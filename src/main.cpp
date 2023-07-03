#include <array>
#include <Arduino.h>
#include <M5Unified.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <ArduinoOSCWiFi.h>
#include <driver/i2s.h>

#include "IMUManager.h"
#include "DisplayManager.h"


// ====== Global ======
IMUManager imuManager;
Preferences preferences;
DisplayManager displayManager;

const int micSamplingRate = 16000; // サンプリング周波数 16 kHz
const int micSampleSize = 512; // 平均を取るサンプル数
const int micBufferSize = micSampleSize * 2;
const int clkPin = 0;
const int dataPin = 34;

const TickType_t healthCheckInterval = pdMS_TO_TICKS(30000);         // 30   s
const TickType_t imuUpdateInterval = pdMS_TO_TICKS(10);              // 10   ms (100  Hz)
const TickType_t oscSendInterval_60fps = pdMS_TO_TICKS(16.6);        // 16.6 ms (60   Hz)
const TickType_t oscSendInterval_30fps = pdMS_TO_TICKS(33.3);        // 33.3 ms (30   Hz)
const TickType_t oscSendInterval_15fps = pdMS_TO_TICKS(66.6);        // 66.6 ms (15   Hz)
const TickType_t micSamplingInterval = pdMS_TO_TICKS(5);             // 5    ms (200  Hz)

const TickType_t i2sWaitTime = pdMS_TO_TICKS(100);                   // 100 ms

String oscServerIp;
int oscServerPort;
String clientName;

uint8_t buffer[micBufferSize] = {0};
int16_t *adcBuffer = nullptr;
auto totalPower = 0.0f;
auto numPower = 0;


// ====== TaskHandler ======
TaskHandle_t healthCheckTaskHandle = nullptr;
TaskHandle_t imuTaskHandle = nullptr;
TaskHandle_t micTaskHandle = nullptr;
TaskHandle_t sendImuOscTaskHandle = nullptr;
TaskHandle_t sendMicOscTaskHandle = nullptr;

// ====== Task ======
[[noreturn]] void healthCheckTask(void *pvParameters);

[[noreturn]] void imuTask(void *pvParameters);

[[noreturn]] void micTask(void *pvParameters);

[[noreturn]] void sendImuOscTask(void *pvParameters);

[[noreturn]] void sendMicOscTask(void *pvParameters);


// ====== Semaphore ======
volatile SemaphoreHandle_t imuSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t micSemaphore = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t displaySemaphore = xSemaphoreCreateBinary();


// ====== Function ======
void i2sInit();

float calcDecibel(float value);

void processSignal();

void connectWiFi();

bool readOscPreference();


void setup() {
    M5.begin();
    M5.Power.begin();
    displayManager.showInitScreen();
    Serial.begin(115200);

    // ====== Audio ======
    M5.Speaker.end();
    M5.Mic.begin();

    // ====== LCD ======
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(0, 0);

    // ====== I2S ======
    i2sInit();

    // ====== OSC ======
    if (readOscPreference()) {
        Serial.println("Read OSC preference successfully.");
    } else {
        Serial.println("Failed to read OSC preference.");
    }

    // ====== WiFi ======
    connectWiFi();

    // ====== IMU ======
    M5.Imu.init();
    imuManager.setup(false);

    // ====== Task ======
    imuSemaphore = xSemaphoreCreateBinary();
    micSemaphore = xSemaphoreCreateBinary();
    displaySemaphore = xSemaphoreCreateBinary();
    if (imuSemaphore == nullptr || micSemaphore == nullptr || displaySemaphore == nullptr) {
        Serial.println("Failed to create semaphore.");
        delay(1000);
        ESP.restart();
        delay(1000);
    }
    xSemaphoreGive(imuSemaphore);
    xSemaphoreGive(micSemaphore);
    xSemaphoreGive(displaySemaphore);


    xTaskCreatePinnedToCore(
            healthCheckTask,
            "Health Check Task",
            4096,
            nullptr,
            1,
            &healthCheckTaskHandle,
            APP_CPU_NUM
    );

    xTaskCreatePinnedToCore(
            imuTask,
            "IMU Task",
            4096,
            nullptr,
            2,
            &imuTaskHandle,
            APP_CPU_NUM
    );

    xTaskCreatePinnedToCore(
            sendImuOscTask,
            "IMU OSC Task",
            4096,
            nullptr,
            2,
            &sendImuOscTaskHandle,
            APP_CPU_NUM
    );

    xTaskCreatePinnedToCore(
            sendMicOscTask,
            "MIC OSC Task",
            4096,
            nullptr,
            4,
            &sendMicOscTaskHandle,
            APP_CPU_NUM
    );


    xTaskCreatePinnedToCore(
            micTask,
            "MIC Task",
            2048,
            nullptr,
            3,
            &micTaskHandle,
            APP_CPU_NUM
    );
}

bool readOscPreference() {
    try {
        preferences.begin("osc", true);
        oscServerIp = preferences.getString("oscServerIp", "192.168.100.10");
        oscServerPort = preferences.getInt("oscServerPort", 9000);
        clientName = preferences.getString("clientName", "ccbt1");
        preferences.end();
    } catch (std::exception &e) {
        Serial.println(e.what());
        return false;
    }

    return true;
}

void connectWiFi() {
    displayManager.showWiFiConnectingScreen();

    WiFiManager wm;
    WiFiManagerParameter oscServerIpParam("oscServerIp",
                                          "OSC Server IP",
                                          oscServerIp.c_str(),
                                          15);
    WiFiManagerParameter oscServerPortParam("oscServerPort",
                                            "OSC Server Port",
                                            std::to_string(oscServerPort).c_str(),
                                            5);
    WiFiManagerParameter clientNameParam("clientName",
                                         "Client Name",
                                         clientName.c_str(),
                                         15);

    WiFiClass::mode(WIFI_STA);

    wm.setConnectRetries(3);
    wm.setConnectTimeout(30);
    wm.setConfigPortalTimeout(180);
    wm.setMinimumSignalQuality();
    wm.setSTAStaticIPConfig(
            IPAddress(192, 168, 100, 11),
            IPAddress(192, 168, 100, 1),
            IPAddress(255, 255, 255, 0)
    );
    wm.setSaveConfigCallback([]() {
        displayManager.showWiFiSettingSavedScreen();
    });

    wm.addParameter(&oscServerIpParam);
    wm.addParameter(&oscServerPortParam);
    wm.addParameter(&clientNameParam);

    auto ssid = wm.getDefaultAPName();
    if (wm.autoConnect(ssid.c_str())) {
        Serial.println("WiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("MAC: ");
        Serial.println(WiFi.macAddress());

        displayManager.showWiFiSettingSuccessScreen();
    } else {
        displayManager.showWiFiSettingFailedScreen();
        delay(2000);
        ESP.restart();
        delay(1000);
    }

    preferences.begin("osc", false);
    oscServerIp = oscServerIpParam.getValue();
    oscServerPort = strtol(oscServerPortParam.getValue(), nullptr, 10);
    clientName = clientNameParam.getValue();
    preferences.putString("oscServerIp", oscServerIp);
    preferences.putInt("oscServerPort", oscServerPort);
    preferences.putString("clientName", clientName);
    preferences.end();

    displayManager.showStatusScreen(clientName.c_str(), oscServerPort);
}

void loop() {
    M5.update();
    if (M5.BtnA.wasReleaseFor(3000)) {
        xSemaphoreTake(displaySemaphore, portMAX_DELAY);

        // リセット確認画面
        DisplayManager::showResetConfirmScreen();

        // 5秒間ボタンが押されなければreturn false
        unsigned long startMillis = millis();
        while (true) {
            M5.update();
            if (M5.BtnA.wasPressed()) {
                Serial.println("Reset WiFi Setting confirmed.");
                preferences.begin("osc", false);
                preferences.clear();
                preferences.end();

                delay(1000);
                WiFi.disconnect(true, true);
                delay(500);
                ESP.restart();
                delay(1000);
            }

            if (millis() - startMillis > 5000) {
                Serial.println("Reset WiFi Setting canceled.");
                xSemaphoreGive(displaySemaphore);
                break;
            }

            delay(5);
        }
        ESP.restart();
        delay(1000);
    }
}

// ====== Task ======
[[noreturn]] void healthCheckTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static auto connectionFailedCount = 0;
    static auto reconnectCount = 0;
    String batteryAddress = "/" + clientName + "/status/battery";

    while (true) {
        // WiFiの疎通確認
        // auto reconnect で対応しきれない場合に再接続を試みる

        xTaskDelayUntil(&xLastWakeTime,
                        healthCheckInterval);

        xSemaphoreTake(displaySemaphore, portMAX_DELAY);

        DisplayManager::showStatusScreen(clientName.c_str(), oscServerPort);

        xSemaphoreGive(displaySemaphore);

        if (WiFiClass::status() == WL_CONNECTED) {
            Serial.println("WiFi is connected and reachable");
            connectionFailedCount = 0;
            reconnectCount = 0;
        } else {
            Serial.println("WiFi is not connected or not reachable");
            connectionFailedCount++;
        }

        if (reconnectCount >= 10) {
            Serial.println("Reconnect failed");
            ESP.restart();
            delay(1000);
        }

        // 10回疎通確認に失敗した場合は再接続を試みる
        if (connectionFailedCount >= 10) {
            Serial.println("Try to reconnect");
            WiFi.disconnect();
            WiFi.reconnect();
            connectionFailedCount = 0;
            reconnectCount++;
        }

        auto isCharging = M5.Power.isCharging();
        auto getBatteryLevel = M5.Power.getBatteryLevel();

        // バッテリー状態の確認低バッテリーの場合はOSCで通知する
        OscWiFi.send(oscServerIp.c_str(),
                     oscServerPort,
                     batteryAddress.c_str(),
                     getBatteryLevel,
                     (bool) isCharging);
    }

    vTaskDelete(healthCheckTaskHandle);
}

[[noreturn]] void imuTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        xTaskDelayUntil(&xLastWakeTime,
                        imuUpdateInterval);
        xSemaphoreTake(imuSemaphore, portMAX_DELAY);
        imuManager.update();
        xSemaphoreGive(imuSemaphore);
    }

    vTaskDelete(imuTaskHandle);
}


[[noreturn]] void micTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    size_t readBytes;

    while (true) {
        xTaskDelayUntil(&xLastWakeTime,
                        micSamplingInterval);

        xSemaphoreTake(micSemaphore, portMAX_DELAY);

        i2s_read(I2S_NUM_0,
                 (char *) buffer,
                 micBufferSize,
                 &readBytes,
                 i2sWaitTime);
        adcBuffer = (int16_t *) buffer;
        processSignal();
        xSemaphoreGive(micSemaphore);
    }
    vTaskDelete(micTaskHandle);
}


[[noreturn]] void sendImuOscTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // 使用するOSCアドレス
    String accAddress = "/" + clientName + "/imu/acc";
    String gyroAddress = "/" + clientName + "/imu/gyro";
    String rotationAddress = "/" + clientName + "/imu/rotation";
    while (true) {
        xTaskDelayUntil(&xLastWakeTime,
                        oscSendInterval_60fps);

        xSemaphoreTake(imuSemaphore, portMAX_DELAY);

        auto acc = imuManager.getAcc();
        auto gyro = imuManager.getGyro();
        auto rotation = imuManager.getRotation();

        xSemaphoreGive(imuSemaphore);

        // ACC
        OscWiFi.send(oscServerIp,
                     oscServerPort,
                     accAddress, acc[0], acc[1], acc[2]);

        // GYRO
        OscWiFi.send(oscServerIp,
                     oscServerPort,
                     gyroAddress, gyro[0], gyro[1], gyro[2]);

        // ROLL & PITCH
        OscWiFi.send(oscServerIp,
                     oscServerPort,
                     rotationAddress, rotation[0], rotation[1]);

    }

    vTaskDelete(sendImuOscTaskHandle);
}

[[noreturn]] void sendMicOscTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // 使用するOSCアドレス
    String micAddress = "/" + clientName + "/mic/volume";
    auto db = 0.0f;
    auto power = 0.0f;
    while (true) {
        xTaskDelayUntil(&xLastWakeTime,
                        oscSendInterval_30fps);

        xSemaphoreTake(micSemaphore, portMAX_DELAY);
        // 蓄積されたデータの平均を取り、dBに変換する
        if (numPower > 0) {
            power = totalPower / numPower;
            db = calcDecibel(power);
            totalPower = 0;
            numPower = 0;
        }
        xSemaphoreGive(micSemaphore);

        // MIC
        OscWiFi.send(oscServerIp,
                     oscServerPort,
                     micAddress, power, db);
    }

    vTaskDelete(sendMicOscTaskHandle);
}

void i2sInit() {
    i2s_config_t i2s_config = {
            .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
            .sample_rate =  micSamplingRate,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 2,
            .dma_buf_len = 128,
    };

    i2s_pin_config_t pinConfig;
    pinConfig.bck_io_num = I2S_PIN_NO_CHANGE;
    pinConfig.ws_io_num = clkPin;
    pinConfig.data_out_num = I2S_PIN_NO_CHANGE;
    pinConfig.data_in_num = dataPin;

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, nullptr);
    i2s_set_pin(I2S_NUM_0, &pinConfig);
    i2s_set_clk(I2S_NUM_0, micSamplingRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

float calcDecibel(float value) {
    // https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/SPM1423HM4H-B_datasheet_en.pdf
    return 8.6859 * log(value) + 25.6699;
}


void processSignal() {
    // https://gist.github.com/tomoto/6a1b67d9e963f9932a43c984171d80fb
    // Author: Tomoto Mizuma (Jul 23, 2021.)
    // Code Changed by: Daiki Miura (June 30, 2023.)

    static float filteredBase = 0.0f;

    // 平均を取ってゼロ点を自動的に補正する
    auto base = 0.0f;
    for (int n = 0; n < micSampleSize; n++)
        base += adcBuffer[n];

    base /= micSampleSize;

    // フィルタをかけて変化を緩やかにする
    const auto alpha = 0.98f;
    filteredBase = filteredBase * alpha + base * (1 - alpha);

    // 二乗平均平方根を取る
    auto power = 0.0;
    for (int n = 0; n < micSampleSize; n++)
        power += sq(adcBuffer[n] - filteredBase);

    power = sqrt(power / micSampleSize);
    totalPower += power;
    numPower++;
}


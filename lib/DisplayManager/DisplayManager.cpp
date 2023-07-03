//
// Created by Daiki Miura on 2023/06/24.
//

#include "DisplayManager.h"

DisplayManager::DisplayManager() = default;

void DisplayManager::showInitScreen() const {
    M5.Display.setFont(&fonts::Font0);
    M5.Display.clearDisplay();
    M5.Display.setCursor(0, 0);
    M5.Display.setTextColor(defaultFontColor, defaultBackgroundColor);
    M5.Display.println("Initializing...");
}

void DisplayManager::showWiFiConnectingScreen() {
    M5.Display.startWrite();
    M5.Display.clear(TFT_BLUE);
    M5.Display.setTextColor(WHITE, TFT_BLUE);
    M5.Display.setCursor(0, 0);
    M5.Display.println("Connecting to WiFi...");
    M5.Display.endWrite();
}

void DisplayManager::showWiFiSettingSavedScreen() {
    M5.Display.startWrite();
    M5.Display.setCursor(0, 8);
    M5.Display.println("> WiFi setting saved.");
    M5.Display.endWrite();
}

void DisplayManager::showWiFiSettingSuccessScreen() {
    M5.Display.startWrite();
    M5.Display.clear(TFT_GREEN);
    M5.Display.setTextColor(WHITE, TFT_GREEN);
    M5.Display.setCursor(0, 0);
    M5.Display.setFont(&fonts::Font4);
    M5.Display.println("WiFi Connected!");
    M5.Display.endWrite();
}

void DisplayManager::showWiFiSettingFailedScreen() {
    M5.Display.startWrite();
    M5.Display.clear(TFT_RED);
    M5.Display.setTextColor(WHITE, TFT_RED);
    M5.Display.setCursor(0, 0);
    M5.Display.setFont(&fonts::Font4);
    M5.Display.println("WiFi Connection Failed! Restarting...");
    M5.Display.endWrite();
}

void DisplayManager::showStatusScreen(const char *ClientName, int oscPort) {
    M5.Display.startWrite();
    M5.Display.clear(TFT_BLACK);
    M5.Display.setTextColor(WHITE, TFT_BLACK);
    M5.Display.setCursor(0, 0);
    M5.Display.setFont(&fonts::Font0);

    // IP Address
    M5.Display.printf("IP:   %s", WiFi.localIP().toString().c_str());
    M5.Display.setCursor(0, 9);
    // MAC Address
    M5.Display.printf("MAC:  %s", WiFi.macAddress().c_str());
    M5.Display.setCursor(0, 18);
    // OSC Client Name
    M5.Display.printf("Name: %s", ClientName);
    M5.Display.setCursor(0, 27);
    // OSC Port
    M5.Display.printf("Port: %d (Send to)", oscPort);
    M5.Display.setCursor(0, 36);
    // Battery Level
    M5.Display.printf("BAT:  %d%%", M5.Power.getBatteryLevel());
    M5.Display.setCursor(0, 45);
    // Battery
    auto isCharging = M5.Power.isCharging();
    M5.Display.printf("Charging: %s", isCharging ? "Yes" : "No");
    M5.Display.setCursor(0, 54);
    M5.Display.endWrite();
}

void DisplayManager::showResetConfirmScreen() {
    M5.Display.startWrite();
    M5.Display.clear(TFT_RED);
    M5.Display.setTextColor(WHITE, TFT_RED);
    M5.Display.setCursor(0, 0);
    M5.Display.setFont(&fonts::Font0);

    M5.Display.println("Reset WiFi Setting?");
    M5.Display.setCursor(0, 10);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.println("A: Yes (Press M5 Btn)");
    M5.Display.endWrite();
}

void DisplayManager::println(const char *str) {
    M5.Display.println(str);
}
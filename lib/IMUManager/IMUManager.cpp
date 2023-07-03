/// \file IMUManager.cpp
/// \brief IMUの値を管理するクラス
/// \author Daiki Miura
/// \date 2021/06/21

#include <array>
#include "IMUManager.h"

IMUManager::IMUManager() = default;

void IMUManager::setup() {
    preferences.begin("imu_calibration", true);
    bool calibrated = preferences.getBool("calibrated", false);
#ifdef DEBUG
    Serial.print("[DEBUG] calibrated: ");
    Serial.println(calibrated);
#endif
    preferences.end();

    if (!calibrated) {
#ifdef DEBUG
        Serial.println("[DEBUG] calibration start");
#endif
        calibration();
        saveCalibration();

        preferences.begin("imu_calibration", false);
        preferences.putBool("calibrated", true);
        preferences.end();
#ifdef DEBUG
        Serial.println("[DEBUG] calibration end");
#endif
    } else {
        loadCalibration();
    }
}

void IMUManager::setup(bool forceCalibration) {
    if (forceCalibration) {
        calibration();
        saveCalibration();

        preferences.begin("imu_calibration", false);  // write-enabled
        preferences.putBool("calibrated", true);
        preferences.end();
    } else {
        setup();
    }

    readImu();
    applyCalibration();
    kalmanX.setAngle(getRoll());
    kalmanY.setAngle(getPitch());
    lastMs = micros();
}

void IMUManager::update() {
    readImu();
    applyCalibration();

    float dt = (micros() - lastMs) / 1000000.0f;
    lastMs = micros();
    float roll = getRoll();
    float pitch = getPitch();

    kalAngleX = kalmanX.getAngle(roll, gyro[0], dt);
    kalAngleY = kalmanY.getAngle(pitch, gyro[1], dt);
}

void IMUManager::draw() {
    tick++;
    if (tick % 20 == 0) {
        tick = 0;
        M5.Lcd.setCursor(0, 15);
        M5.Lcd.printf("%7.2f %7.2f %7.2f", gyro[0], gyro[1], gyro[2]);
        M5.Lcd.setCursor(140, 15);
        M5.Lcd.print("o/s");
        M5.Lcd.setCursor(0, 30);
        M5.Lcd.printf("%7.2f %7.2f %7.2f", acc[0] * 1000, acc[1] * 1000, acc[2] * 1000);
        M5.Lcd.setCursor(145, 30);
        M5.Lcd.print("mg");
        M5.Lcd.setCursor(0, 45);
        M5.Lcd.printf("%7.2f %7.2f", kalAngleX, kalAngleY);
        M5.Lcd.setCursor(140, 45);
        M5.Lcd.print("deg");
    }
}

float IMUManager::getRoll() {
    return atan(acc[1] / sqrt((acc[0] * acc[0]) + (acc[2] * acc[2]))) * RAD_TO_DEG;
}

float IMUManager::getPitch() {
    return atan(-acc[0] / sqrt((acc[1] * acc[1]) + (acc[2] * acc[2]))) * RAD_TO_DEG;
}

std::array<float, 3> IMUManager::getAcc() {
    return acc;
}

std::array<float, 3> IMUManager::getGyro() {
    return gyro;
}

std::array<float, 2> IMUManager::getRotation() {
    return {
            kalAngleX,
            kalAngleY
    };
}

void IMUManager::calibration() {
    std::array<float, 3> gyroSum{};
    std::array<float, 3> accSum{};

    for (int i = 0; i < 500; i++) {
        readImu();
        gyroSum[0] += gyro[0];
        gyroSum[1] += gyro[1];
        gyroSum[2] += gyro[2];
        accSum[0] += acc[0];
        accSum[1] += acc[1];
        accSum[2] += acc[2];
        delay(2);
    }

    gyroOffset[0] = gyroSum[0] / 500;
    gyroOffset[1] = gyroSum[1] / 500;
    gyroOffset[2] = gyroSum[2] / 500;
    accOffset[0] = accSum[0] / 500;
    accOffset[1] = accSum[1] / 500;
    accOffset[2] = accSum[2] / 500 - 1.0f;  // Subtract gravitational acceleration 1G
}

void IMUManager::readImu() {
    M5.Imu.getGyroData(&gyro[0], &gyro[1], &gyro[2]);
    M5.Imu.getAccelData(&acc[0], &acc[1], &acc[2]);
}

void IMUManager::applyCalibration() {
    gyro[0] -= gyroOffset[0];
    gyro[1] -= gyroOffset[1];
    gyro[2] -= gyroOffset[2];
    acc[0] -= accOffset[0];
    acc[1] -= accOffset[1];
    acc[2] -= accOffset[2];
}

void IMUManager::loadCalibration() {
    preferences.begin("imu_calibration", true);  // read-only
    preferences.getBytes("gyroOffset", gyroOffset.data(), sizeof(gyroOffset));
    preferences.getBytes("accOffset", accOffset.data(), sizeof(accOffset));
    preferences.end();
}

void IMUManager::saveCalibration() {
    preferences.begin("imu_calibration", false);  // write-enabled
    preferences.putBytes("gyroOffset", gyroOffset.data(), sizeof(gyroOffset));
    preferences.putBytes("accOffset", accOffset.data(), sizeof(accOffset));
    preferences.end();
}




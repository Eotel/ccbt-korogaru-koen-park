/// \file IMUManager.h
/// \brief IMUの値を管理するクラス
/// \author Daiki Miura
/// \date 2021/06/21


#ifndef CCBT_KOROGARU_KOEN_PARK_IMUMANAGER_H
#define CCBT_KOROGARU_KOEN_PARK_IMUMANAGER_H

#include <array>
#include <Preferences.h>
#include <M5Unified.h>
#include "Kalman.h"


class IMUManager {
public:
    IMUManager();

    void setup();

    void setup(bool forceCalibration);

    void update();

    void draw();


    std::array<float, 3> getAcc();

    std::array<float, 3> getGyro();

    std::array<float, 2> getRotation();


private:
    Preferences preferences;
    std::array<float, 3> acc{};
    std::array<float, 3> accOffset{};
    std::array<float, 3> gyro{};
    std::array<float, 3> gyroOffset{};

    float getRoll();

    float getPitch();

    float kalAngleX = .0f;
    float kalAngleY = .0f;

    Kalman kalmanX;
    Kalman kalmanY;
    unsigned long lastMs = 0;
    unsigned long tick = 0;

    void calibration();

    void readImu();

    void applyCalibration();

    void loadCalibration();

    void saveCalibration();

};

#endif //CCBT_KOROGARU_KOEN_PARK_IMUMANAGER_H

//
// Created by Daiki Miura on 2023/06/24.
//

#ifndef CCBT_KOROGARU_KOEN_PARK_DISPLAYMANAGER_H
#define CCBT_KOROGARU_KOEN_PARK_DISPLAYMANAGER_H

#include "M5Unified.h"
#include "WiFi.h"


class DisplayManager {
public:
    uint32_t defaultFontColor = WHITE;
    uint32_t defaultBackgroundColor = BLACK;

    DisplayManager();


    static void showWiFiConnectingScreen();

    static void showWiFiSettingSuccessScreen();

    static void showWiFiSettingSavedScreen();

    static void showWiFiSettingFailedScreen();

    static void showResetConfirmScreen();

    static void println(const char *str);

    void showInitScreen() const;

    static void showStatusScreen(const char* oscClientName, int oscPort);

    void showBatteryStatus();
};


#endif //CCBT_KOROGARU_KOEN_PARK_DISPLAYMANAGER_H

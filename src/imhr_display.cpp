#include "imhr_display.h"

namespace imhr
{

    U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

    void displayInit()
    {
        display.begin();
    }

    static void drawCenteredStr(const char *text, int y)
    {
        int width = display.getStrWidth(text);
        display.drawStr((128 - width) / 2, y, text);
    }

    void displayMessage(const char *line1, const char *line2)
    {
        display.clearBuffer();

        display.setFont(u8g2_font_ncenB14_tr);
        drawCenteredStr(line1, 30);

        display.setFont(u8g2_font_ncenB08_tr);
        drawCenteredStr(line2, 50);

        display.sendBuffer();
    }

    static void drawDepartureLine(const char *label, const char *minsStr, int y)
    {
        display.setFont(u8g2_font_logisoso16_tr);
        display.drawStr(0, y, label);

        int strW = display.getStrWidth(minsStr);
        display.drawStr(128 - strW, y, minsStr);
    }

    static void determineMinuteDisplay(char *minuteDisplay, size_t size, int minutes)
    {
        if (minutes < 0)
            snprintf(minuteDisplay, size, "--");
        else if (minutes == 0)
            snprintf(minuteDisplay, size, "sofort");
        else
            snprintf(minuteDisplay, size, "%d min", minutes);
    }

    void displayDeparture(const char *busLine, int busMins,
                          const char *trainPlatform, int trainMins)
    {
        char busLabel[16];
        char trainLabel[16];
        snprintf(busLabel, sizeof(busLabel), "%s", busLine);

        if (trainPlatform && strlen(trainPlatform) > 0)
            snprintf(trainLabel, sizeof(trainLabel), trainPlatform);
        else
            snprintf(trainLabel, sizeof(trainLabel), "Zug");

        char busMinsStr[12];
        char trainMinsStr[12];
        determineMinuteDisplay(busMinsStr, sizeof(busMinsStr), busMins);
        determineMinuteDisplay(trainMinsStr, sizeof(trainMinsStr), trainMins);

        display.clearBuffer();
        drawDepartureLine(busLabel, busMinsStr, 20);
        drawDepartureLine(trainLabel, trainMinsStr, 48);
        display.sendBuffer();
    }

}

#include "imhr_display.h"

namespace imhr
{

    U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

    void displayInit()
    {
        display.begin();
    }

    void displayMessage(const char *line1, const char *line2)
    {
        display.clearBuffer();
        display.setFont(u8g2_font_ncenB08_tr);
        display.drawStr(0, 12, line1);
        if (line2)
            display.drawStr(0, 28, line2);
        display.sendBuffer();
    }

    static void drawDepartureLine(const char *label, const char *minsStr, int y)
    {
        display.setFont(u8g2_font_logisoso16_tr);
        display.drawStr(0, y, label);

        int strW = display.getStrWidth(minsStr);
        display.drawStr(128 - strW, y, minsStr);
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

        if (busMins < 0)
            snprintf(busMinsStr, sizeof(busMinsStr), "--");
        else if (busMins == 0)
            snprintf(busMinsStr, sizeof(busMinsStr), "sofort");
        else
            snprintf(busMinsStr, sizeof(busMinsStr), "%d min", busMins);

        if (trainMins < 0)
            snprintf(trainMinsStr, sizeof(trainMinsStr), "--");
        else if (trainMins == 0)
            snprintf(trainMinsStr, sizeof(trainMinsStr), "sofort");
        else
            snprintf(trainMinsStr, sizeof(trainMinsStr), "%d min", trainMins);

        display.clearBuffer();
        drawDepartureLine(busLabel, busMinsStr, 20);
        drawDepartureLine(trainLabel, trainMinsStr, 48);
        display.sendBuffer();
    }

}

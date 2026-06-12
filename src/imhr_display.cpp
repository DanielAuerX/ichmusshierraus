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

    void displayDeparture(const char *line, int mins)
    {
        String secondLine = (mins == 0) ? "sofort" : String(mins) + " min";
        if (mins == 0) {}
        char line1[32];
        char line2[32];
        snprintf(line1, sizeof(line1), "Bus %s", line);
        snprintf(line2, sizeof(line2), secondLine.c_str());

        display.clearBuffer();
        display.setFont(u8g2_font_logisoso28_tr);
        display.drawStr(0, 30, line1);
        display.drawStr(0, 62, line2);
        display.sendBuffer();
    }
}

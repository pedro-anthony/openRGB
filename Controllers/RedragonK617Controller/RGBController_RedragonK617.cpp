/*---------------------------------------------------------*\
| RGBController_RedragonK617.cpp                             |
|                                                           |
|   OpenRGB interface for Redragon K617 Fizz                 |
|   Pedro Antonio                             05 Oct 2026  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "LogManager.h"
#include "RGBControllerKeyNames.h"
#include "RGBController_RedragonK617.h"

/**------------------------------------------------------------------*\
    @name Redragon K617 Fizz
    @category Keyboard
    @type USB
    @save :x:
    @direct :x:
    @effects :x:
    @detectors DetectRedragonK617
    @comment Experimental static and per-key lighting; detector disabled by default.
\*-------------------------------------------------------------------*/

RGBController_RedragonK617::RGBController_RedragonK617(RedragonK617Controller* controller_ptr)
{
    controller          = controller_ptr;
    name                = "Redragon K617 Fizz";
    vendor              = "Redragon";
    type                = DEVICE_TYPE_KEYBOARD;
    description         = "Experimental K617 static and per-key lighting";
    version             = "USB release 1.01";
    location            = controller->GetLocation();

    mode Static;
    Static.name         = "Static";
    Static.value        = 1;
    Static.flags        = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
    Static.color_mode   = MODE_COLORS_MODE_SPECIFIC;
    Static.colors_min   = 1;
    Static.colors_max   = 1;
    Static.colors.resize(1);
    modes.push_back(Static);

    mode Custom;
    Custom.name         = "Custom";
    Custom.value        = 21;
    Custom.flags        = MODE_FLAG_HAS_PER_LED_COLOR;
    Custom.color_mode   = MODE_COLORS_PER_LED;
    modes.push_back(Custom);

    SetupZones();
}

RGBController_RedragonK617::~RGBController_RedragonK617()
{
    Shutdown();
    delete controller;
}

void RGBController_RedragonK617::SetupZones()
{
    zone keyboard;
    keyboard.name       = "Keyboard";
    keyboard.type       = ZONE_TYPE_MATRIX;
    keyboard.leds_min   = REDRAGON_K617_LED_COUNT;
    keyboard.leds_max   = REDRAGON_K617_LED_COUNT;
    keyboard.leds_count = REDRAGON_K617_LED_COUNT;
    const unsigned int NA = 0xFFFFFFFF;
    /* ABNT2: preserve the original LED order and append CFG keys 62/63. */
    unsigned int map[5][15] =
    {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, NA },
        { 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, NA, NA },
        { 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 27, 40, NA },
        { 41, 61, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 62, 52, NA },
        { 53, 54, 55, NA, NA, 56, NA, NA, NA, 57, 58, NA, 59, 60, NA },
    };
    keyboard.matrix_map.Set(5, 15, &map[0][0]);
    zones.push_back(keyboard);

    const char* names[] =
    {
        KEY_EN_ESCAPE, KEY_EN_1, KEY_EN_2, KEY_EN_3, KEY_EN_4, KEY_EN_5, KEY_EN_6, KEY_EN_7, KEY_EN_8, KEY_EN_9, KEY_EN_0, KEY_EN_MINUS, KEY_EN_EQUALS, KEY_EN_BACKSPACE,
        KEY_EN_TAB, KEY_EN_Q, KEY_EN_W, KEY_EN_E, KEY_EN_R, KEY_EN_T, KEY_EN_Y, KEY_EN_U, KEY_EN_I, KEY_EN_O, KEY_EN_P, KEY_NORD_ACUTE_GRAVE, KEY_EN_LEFT_BRACKET, KEY_EN_RIGHT_BRACKET,
        KEY_EN_CAPS_LOCK, KEY_EN_A, KEY_EN_S, KEY_EN_D, KEY_EN_F, KEY_EN_G, KEY_EN_H, KEY_EN_J, KEY_EN_K, KEY_EN_L, KEY_FR_CEDILLA_C, KEY_BR_TILDE, KEY_EN_ISO_ENTER,
        KEY_EN_LEFT_SHIFT, KEY_EN_Z, KEY_EN_X, KEY_EN_C, KEY_EN_V, KEY_EN_B, KEY_EN_N, KEY_EN_M, KEY_EN_COMMA, KEY_EN_PERIOD, KEY_EN_SEMICOLON, KEY_EN_RIGHT_SHIFT,
        KEY_EN_LEFT_CONTROL, KEY_EN_LEFT_WINDOWS, KEY_EN_LEFT_ALT, KEY_EN_SPACE, KEY_EN_RIGHT_ALT, KEY_EN_RIGHT_FUNCTION, KEY_EN_MENU, KEY_EN_RIGHT_CONTROL,
        KEY_EN_ISO_BACK_SLASH, KEY_EN_FORWARD_SLASH,
    };
    for(unsigned int i = 0; i < REDRAGON_K617_LED_COUNT; i++)
    {
        led key;
        key.name = names[i];
        key.value = RedragonK617Controller::key_indices[i];
        leds.push_back(key);
    }
    SetupColors();
}

void RGBController_RedragonK617::DeviceUpdateLEDs()
{
    bool success;
    if(modes[active_mode].value == 21)
    {
        unsigned char rgb[REDRAGON_K617_LED_COUNT * 3];
        for(unsigned int i = 0; i < REDRAGON_K617_LED_COUNT; i++)
        {
            rgb[3 * i]     = RGBGetRValue(colors[i]);
            rgb[3 * i + 1] = RGBGetGValue(colors[i]);
            rgb[3 * i + 2] = RGBGetBValue(colors[i]);
        }
        success = controller->SetKeyColors(rgb, REDRAGON_K617_LED_COUNT);
    }
    else
    {
        RGBColor color = modes[active_mode].colors[0];
        success = controller->SetStaticColor(RGBGetRValue(color), RGBGetGValue(color), RGBGetBValue(color));
    }
    if(!success)
    {
        LOG_ERROR("[Redragon K617] %s; remaining configuration writes aborted", controller->GetLastError().c_str());
    }
}

void RGBController_RedragonK617::DeviceUpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_RedragonK617::DeviceUpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_RedragonK617::DeviceUpdateMode()
{
    DeviceUpdateLEDs();
}

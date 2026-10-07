/*---------------------------------------------------------*\
| RedragonK617Controller.h                                  |
|                                                           |
|   Experimental static lighting for Redragon K617 Fizz     |
|   Pedro Antonio                             05 Oct 2026  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <hidapi.h>
#include <string>

#define REDRAGON_K617_REPORT_SIZE     1032
#define REDRAGON_K617_CONFIG_SIZE      262
#define REDRAGON_K617_LED_COUNT         63

class RedragonK617Controller
{
public:
    // ponytail: one-color linear estimate; use measured curves if other mixes disagree.
    RedragonK617Controller(hid_device* dev_handle, const char* path, double blue_scale = 32.0 / 81.0);
    ~RedragonK617Controller();

    static bool MatchesDevice(const hid_device_info* info);
    static bool MatchesDescriptor(hid_device* dev_handle);
    std::string GetLocation();
    const std::string& GetLastError() const { return(last_error); }
    static constexpr unsigned char key_indices[REDRAGON_K617_LED_COUNT] = { 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 75, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 76, 84, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 97, 105, 106, 107, 110, 113, 114, 117, 118, 85, 96 };
    bool        SetKeyColors(const unsigned char* rgb, unsigned int count);
    bool        SetStaticColor(unsigned char red, unsigned char green, unsigned char blue);

private:
    bool ApplyColorReport(const unsigned char* color, bool custom);
    hid_device* dev;
    std::string location;
    std::string last_error;
    double blue_scale;
};

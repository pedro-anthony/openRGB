/*---------------------------------------------------------*\
| RedragonK617Controller.cpp                                |
|                                                           |
|   Experimental static lighting for Redragon K617 Fizz     |
|   Pedro Antonio                             05 Oct 2026  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cwchar>
#include <thread>
#include "RedragonK617Controller.h"

/*---------------------------------------------------------*\
| Interface 1 descriptor from the K617 capture (frame 21115).|
| Exact matching is deliberately conservative while the     |
| detector is experimental and disabled by default.          |
\*---------------------------------------------------------*/
static const unsigned char k617_report_descriptor[] =
{
    0x06, 0x01, 0x00, 0x09, 0x80, 0xA1, 0x01, 0x85, 0x01, 0x19, 0x81, 0x29, 0x83, 0x15, 0x00, 0x25,
    0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05, 0x81, 0x01, 0xC0, 0x05, 0x0C,
    0x09, 0x01, 0xA1, 0x01, 0x85, 0x02, 0x19, 0x00, 0x2A, 0xFF, 0x02, 0x15, 0x00, 0x26, 0xFF, 0x7F,
    0x95, 0x01, 0x75, 0x10, 0x81, 0x00, 0xC0, 0x06, 0x00, 0xFF, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x03,
    0x15, 0x00, 0x26, 0xFF, 0x00, 0x09, 0x2F, 0x75, 0x08, 0x95, 0x03, 0x81, 0x02, 0xC0, 0x05, 0x01,
    0x09, 0x06, 0xA1, 0x01, 0x85, 0x04, 0x05, 0x07, 0x19, 0x04, 0x29, 0x70, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x78, 0x81, 0x02, 0xC0, 0x06, 0x00, 0xFF, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x05,
    0x15, 0x00, 0x26, 0xFF, 0x00, 0x19, 0x01, 0x29, 0x02, 0x75, 0x08, 0x95, 0x05, 0xB1, 0x02, 0xC0,
    0x06, 0x00, 0xFF, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x06, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x19, 0x01,
    0x29, 0x02, 0x75, 0x08, 0x96, 0x07, 0x04, 0xB1, 0x02, 0xC0, 0x05, 0x01, 0x09, 0x02, 0xA1, 0x01,
    0x85, 0x07, 0x09, 0x01, 0xA1, 0x00, 0x05, 0x09, 0x15, 0x00, 0x25, 0x01, 0x19, 0x01, 0x29, 0x05,
    0x75, 0x01, 0x95, 0x05, 0x81, 0x02, 0x95, 0x03, 0x81, 0x01, 0x05, 0x01, 0x16, 0x00, 0x80, 0x26,
    0xFF, 0x7F, 0x09, 0x30, 0x09, 0x31, 0x75, 0x10, 0x95, 0x02, 0x81, 0x06, 0x15, 0x81, 0x25, 0x7F,
    0x09, 0x38, 0x75, 0x08, 0x95, 0x01, 0x81, 0x06, 0x05, 0x0C, 0x0A, 0x38, 0x02, 0x95, 0x01, 0x81,
    0x06, 0xC0, 0xC0,
};

/*---------------------------------------------------------*\
| Static-color mask from frame 44387. These bytes are fixed  |
| in all four captured red/green/blue/red reports.            |
\*---------------------------------------------------------*/
static const unsigned short static_color_mask[] =
{
    0x022, 0x024, 0x026, 0x027, 0x029, 0x02B, 0x02D, 0x02E, 0x02F, 0x030, 0x031, 0x032,
    0x037, 0x039, 0x03B, 0x03C, 0x03E, 0x040, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047,
    0x04C, 0x04E, 0x050, 0x051, 0x053, 0x055, 0x057, 0x058, 0x059, 0x05A, 0x05B, 0x05C,
    0x061, 0x063, 0x065, 0x066, 0x068, 0x06A, 0x06C, 0x06D, 0x06E, 0x06F, 0x070, 0x071,
    0x076, 0x078, 0x07A, 0x07B, 0x07D, 0x07F, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086,
    0x08B, 0x08D, 0x08F, 0x090, 0x092, 0x094, 0x096, 0x097, 0x098, 0x099, 0x09A, 0x09B,
    0x0A0, 0x0A2, 0x0A4, 0x0A5, 0x0A7, 0x0A9, 0x0AB, 0x0AC, 0x0AD, 0x0AE, 0x0AF, 0x0B0,
    0x0B5, 0x0B7, 0x0B9, 0x0BA, 0x0BC, 0x0BE, 0x0C0, 0x0C1, 0x0C2, 0x0C3, 0x0C4, 0x0C5,
    0x0CA, 0x0CC, 0x0CE, 0x0CF, 0x0D1, 0x0D3, 0x0D5, 0x0D6, 0x0D7, 0x0D8, 0x0D9, 0x0DA,
    0x0DF, 0x0E1, 0x0E3, 0x0E4, 0x0E6, 0x0E8, 0x0EA, 0x0EB, 0x0EC, 0x0ED, 0x0EE, 0x0EF,
    0x0F4, 0x0F6, 0x0F8, 0x0F9, 0x0FB, 0x0FD, 0x0FF, 0x100, 0x101, 0x102, 0x103, 0x104,
    0x109, 0x10B, 0x10D, 0x10E, 0x110, 0x112, 0x114, 0x115, 0x116, 0x117, 0x118, 0x119,
    0x11E, 0x120, 0x122, 0x123, 0x125, 0x127, 0x129, 0x12A, 0x12B, 0x12C, 0x12D, 0x12E,
    0x133, 0x135, 0x137, 0x138, 0x13A, 0x13C, 0x13E, 0x13F, 0x140, 0x141, 0x142, 0x143,
    0x148, 0x14A, 0x14C, 0x14D, 0x14F, 0x151, 0x153, 0x154, 0x155, 0x156, 0x157, 0x158,
    0x15D, 0x15F, 0x161, 0x162, 0x164, 0x166, 0x168, 0x169, 0x16A, 0x16B, 0x16C, 0x16D,
    0x172, 0x174, 0x176, 0x177, 0x179, 0x17B, 0x17D, 0x17E, 0x17F, 0x180, 0x181, 0x182,
    0x187, 0x189, 0x18B, 0x18C, 0x18E, 0x190, 0x192, 0x193, 0x194, 0x195, 0x196, 0x197,
    0x19C, 0x19E, 0x1A0, 0x1A1, 0x1A3, 0x1A5, 0x1A7, 0x1A8, 0x1A9, 0x1AA, 0x1AB, 0x1AC,
    0x1B1, 0x1B3, 0x1B5, 0x1B6, 0x1B8, 0x1BA, 0x1BC, 0x1BD, 0x1BE, 0x1BF, 0x1C0, 0x1C1,
    0x1C6, 0x1C8, 0x1CA, 0x1CB, 0x1CD, 0x1CF, 0x1D1, 0x1D2, 0x1D3, 0x1D4, 0x1D5, 0x1D6,
    0x1DB, 0x1DD, 0x1DF, 0x1E0, 0x1E2, 0x1E4, 0x1E6, 0x1E7, 0x1E8, 0x1E9, 0x1EA,
};

RedragonK617Controller::RedragonK617Controller(hid_device* dev_handle, const char* path, double blue_scale)
{
    dev      = dev_handle;
    location = path;
    this->blue_scale = std::isfinite(blue_scale) && blue_scale >= 0.0 && blue_scale <= 1.0
                     ? blue_scale : 32.0 / 81.0;
}

RedragonK617Controller::~RedragonK617Controller()
{
    hid_close(dev);
}

bool RedragonK617Controller::MatchesDevice(const hid_device_info* info)
{
    if(!info->path || info->release_number != 0x0101
    || !info->manufacturer_string || !info->product_string
    || std::wcscmp(info->manufacturer_string, L"BY Tech") != 0
    || std::wcscmp(info->product_string, L"Gaming Keyboard") != 0)
    {
        return(false);
    }

    /* Linux HIDAPI enumerates multiple collections with the same path. */
    for(const hid_device_info* next = info->next; next; next = next->next)
    {
        if(next->path && next->usage_page == 0xFF00 && next->usage == 1
        && std::strcmp(info->path, next->path) == 0)
        {
            return(false);
        }
    }
    return(true);
}

bool RedragonK617Controller::MatchesDescriptor(hid_device* dev_handle)
{
    unsigned char descriptor[sizeof(k617_report_descriptor) + 1];
    int size = hid_get_report_descriptor(dev_handle, descriptor, sizeof(descriptor));
    return(size == sizeof(k617_report_descriptor)
        && std::memcmp(descriptor, k617_report_descriptor, sizeof(k617_report_descriptor)) == 0);
}

std::string RedragonK617Controller::GetLocation()
{
    return("HID: " + location);
}

bool RedragonK617Controller::ApplyColorReport(const unsigned char* color, bool custom)
{
    /*-----------------------------------------------------*\
    | Read the live configuration as the OEM app does.       |
    | Never overwrite it after a failed or unexpected read. |
    \*-----------------------------------------------------*/
    last_error.clear();
    auto transfer_failed = [this](const char* stage, int actual, int expected)
    {
        last_error = std::string(stage) + ": returned " + std::to_string(actual)
                   + " bytes, expected " + std::to_string(expected);
        if(actual < 0)
        {
            const wchar_t* error = hid_error(dev);
            char detail[512] = {};
            if(error && std::snprintf(detail, sizeof(detail), "%ls", error) > 0)
            {
                last_error += std::string("; HID: ") + detail;
            }
        }
        return(false);
    };

    const unsigned char request[] = {0x05, 0x83, 0xB6, 0x00, 0x00, 0x00};
    unsigned char config[REDRAGON_K617_REPORT_SIZE] = {0x06};

    int size = hid_send_feature_report(dev, request, sizeof(request));
    if(size != sizeof(request))
    {
        return(transfer_failed("Profile read request (report 05)", size, sizeof(request)));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    /* OEM requests the full feature report; the profile reply is shorter. */
    size = hid_get_feature_report(dev, config, sizeof(config));
    if(size != REDRAGON_K617_CONFIG_SIZE)
    {
        return(transfer_failed("Profile response (report 06)", size, REDRAGON_K617_CONFIG_SIZE));
    }

    if(config[0] != 0x06 || config[1] != 0x83 || config[2] != 0xB6 || config[3] != 0x00
    || config[14] != 0x5A || config[15] != 0xA5 || config[16] != 0x03 || config[17] != 0x03
    || config[137] != 0x5A || config[138] != 0xA5 || config[139] != 0x03 || config[140] != 0x03)
    {
        char detail[256];
        std::snprintf(detail, sizeof(detail),
                      "Profile response rejected: header %02X %02X %02X %02X; markers %02X %02X %02X %02X / %02X %02X %02X %02X",
                      config[0], config[1], config[2], config[3],
                      config[14], config[15], config[16], config[17],
                      config[137], config[138], config[139], config[140]);
        last_error = detail;
        return(false);
    }

    size = hid_send_feature_report(dev, color, REDRAGON_K617_REPORT_SIZE);
    if(size != REDRAGON_K617_REPORT_SIZE)
    {
        return(transfer_failed("Color table write (report 06)", size, REDRAGON_K617_REPORT_SIZE));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    /*-----------------------------------------------------*\
    | Convert the read response to a write and select the mode.|
    | Preserve other mode settings and brightness.           |
    | The OEM capture pads this 262-byte block to 1032 bytes. |
    | Key bindings are left untouched.    |
    \*-----------------------------------------------------*/
    config[1]  = 0x03;
    config[20] = custom ? 0x01 : 0x00;
    config[21] = custom ? 0x15 : 0x01;

    size = hid_send_feature_report(dev, config, sizeof(config));
    if(size != sizeof(config))
    {
        return(transfer_failed("Lighting mode write (report 06)", size, sizeof(config)));
    }
    return(true);
}

bool RedragonK617Controller::SetStaticColor(unsigned char red, unsigned char green, unsigned char blue)
{
    unsigned char color[REDRAGON_K617_REPORT_SIZE] = {0x06, 0x08, 0xB8, 0x00, 0x40};

    for(unsigned int i = 0; i < sizeof(static_color_mask) / sizeof(static_color_mask[0]); i++)
    {
        color[static_color_mask[i]] = 0xFF;
    }

    color[29] = red;
    color[30] = green;
    color[31] = static_cast<unsigned char>(std::lround(blue * blue_scale));

    return(ApplyColorReport(color, false));
}

bool RedragonK617Controller::SetKeyColors(const unsigned char* rgb, unsigned int count)
{
    if(!rgb || count != REDRAGON_K617_LED_COUNT)
    {
        last_error = "Custom color update requires exactly 63 RGB colors";
        return(false);
    }

    unsigned char color[REDRAGON_K617_REPORT_SIZE] = {0x06, 0x09, 0xBC, 0x00, 0x40};
    /* Fixed tail reproduced from both captures, outside the custom B/G/R planes. */
    const unsigned short tail_mask[] = {660, 661, 662, 663, 664, 680, 681, 682, 683, 684,
                                       702, 703, 704, 706, 722, 728, 743, 745, 748};
    for(unsigned short offset : tail_mask)
    {
        color[offset] = 0xFF;
    }
    for(unsigned int i = 0; i < count; i++)
    {
        unsigned int offset = 8 + key_indices[i];
        color[offset]       = static_cast<unsigned char>(std::lround(rgb[3 * i + 2] * blue_scale));
        color[offset + 126] = rgb[3 * i + 1];
        color[offset + 252] = rgb[3 * i];
    }
    return(ApplyColorReport(color, true));
}

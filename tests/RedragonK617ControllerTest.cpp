/*---------------------------------------------------------*\
| RedragonK617ControllerTest.cpp                             |
|                                                           |
|   Offline capture and failed-transfer checks              |
|   Pedro Antonio                             05 Oct 2026  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include "RedragonK617Controller.h"

static std::vector<unsigned char> descriptor;
static std::vector<unsigned char> response;
static std::vector<std::vector<unsigned char>> writes;
static int failed_write = -1;
static int read_length  = REDRAGON_K617_CONFIG_SIZE;

int hid_get_report_descriptor(hid_device*, unsigned char* data, size_t size)
{
    size_t count = std::min(size, descriptor.size());
    std::memcpy(data, descriptor.data(), count);
    return((int)count);
}

int hid_send_feature_report(hid_device*, const unsigned char* data, size_t size)
{
    writes.push_back(std::vector<unsigned char>(data, data + size));
    return((int)writes.size() == failed_write ? -1 : (int)size);
}

int hid_get_feature_report(hid_device*, unsigned char* data, size_t size)
{
    assert(size == REDRAGON_K617_REPORT_SIZE);
    assert(data[0] == 0x06);
    std::memcpy(data, response.data(), std::min(size, response.size()));
    return(read_length);
}

const wchar_t* hid_error(hid_device*)
{
    return(L"mock transfer failure");
}

void hid_close(hid_device*)
{
}

static std::vector<unsigned char> ReadFixture(const char* name)
{
    std::ifstream input(std::string("tests/data/redragon-k617/") + name, std::ios::binary);
    assert(input.good());
    return(std::vector<unsigned char>(std::istreambuf_iterator<char>(input), {}));
}

int main()
{
    hid_device_info info = {};
    char path[] = "offline";
    wchar_t manufacturer[] = L"BY Tech";
    wchar_t product[] = L"Gaming Keyboard";
    info.path = path;
    info.manufacturer_string = manufacturer;
    info.product_string = product;
    info.release_number = 0x0101;
    info.usage_page = 0xFF00;
    info.usage = 1;
    assert(RedragonK617Controller::MatchesDevice(&info));
    hid_device_info duplicate = info;
    info.next = &duplicate;
    assert(!RedragonK617Controller::MatchesDevice(&info));
    assert(RedragonK617Controller::MatchesDevice(&duplicate));
    char other_path[] = "another-keyboard";
    duplicate.path = other_path;
    assert(RedragonK617Controller::MatchesDevice(&info));
    info.release_number = 0x0100;
    assert(!RedragonK617Controller::MatchesDevice(&info));
    info.release_number = 0x0101;
    info.product_string = nullptr;
    assert(!RedragonK617Controller::MatchesDevice(&info));

    descriptor = ReadFixture("descriptor.bin");
    hid_device* dev = (hid_device*)1;
    assert(RedragonK617Controller::MatchesDescriptor(dev));
    assert(writes.empty());
    descriptor[0] ^= 1;
    assert(!RedragonK617Controller::MatchesDescriptor(dev));
    descriptor[0] ^= 1;
    descriptor.push_back(0);
    assert(!RedragonK617Controller::MatchesDescriptor(dev));

    RedragonK617Controller controller(dev, "offline");
    std::vector<unsigned char> color = ReadFixture("static-red.bin");
    std::vector<unsigned char> mode  = ReadFixture("mode-static.bin");
    assert(color.size() == REDRAGON_K617_REPORT_SIZE);
    assert(mode.size() == REDRAGON_K617_REPORT_SIZE);

    /* Read-shaped mode data must reconstruct the captured static write. */
    response = mode;
    response[1]  = 0x83;
    response[20] = 0x01;
    response[21] = 0x15;
    assert(controller.SetStaticColor(255, 0, 0));
    assert(writes.size() == 3);
    assert(writes[0] == std::vector<unsigned char>({0x05, 0x83, 0xB6, 0, 0, 0}));
    assert(writes[1] == color);
    assert(writes[2] == mode);

    /* Preserve a real captured profile, including unrelated settings. */
    response = ReadFixture("profile-response.bin");
    writes.clear();
    assert(controller.SetStaticColor(0, 255, 0));
    color[29] = 0;
    color[30] = 255;
    assert(writes[1] == color);
    for(unsigned int i = 0; i < response.size(); i++)
    {
        if(i != 1 && i != 20 && i != 21)
        {
            assert(writes[2][i] == response[i]);
        }
    }
    for(unsigned int i = response.size(); i < writes[2].size(); i++)
    {
        assert(writes[2][i] == 0);
    }

    /* Failed/short/malformed reads must never write a configuration. */
    const int bad_lengths[] = {-1, 0, 6, REDRAGON_K617_CONFIG_SIZE - 1};
    for(unsigned int i = 0; i < sizeof(bad_lengths) / sizeof(bad_lengths[0]); i++)
    {
        writes.clear();
        read_length = bad_lengths[i];
        assert(!controller.SetStaticColor(0, 0, 255));
        assert(writes.size() == 1);
        assert(controller.GetLastError().find("Profile response (report 06)") == 0);
    }
    read_length = REDRAGON_K617_CONFIG_SIZE;
    response[14] = 0;
    writes.clear();
    assert(!controller.SetStaticColor(0, 0, 255));
    assert(writes.size() == 1);
    assert(controller.GetLastError().find("Profile response rejected:") == 0);
    response[14] = 0x5A;

    for(int i = 1; i <= 3; i++)
    {
        writes.clear();
        failed_write = i;
        assert(!controller.SetStaticColor(0, 0, 255));
        assert((int)writes.size() == i);
        const char* stages[] = {"Profile read request", "Color table write", "Lighting mode write"};
        assert(controller.GetLastError().find(stages[i - 1]) == 0);
        assert(controller.GetLastError().find("mock transfer failure") != std::string::npos);
    }
    failed_write = -1;
    writes.clear();
    unsigned char rgb[REDRAGON_K617_LED_COUNT * 3] = {};
    const unsigned int selected[] = {16, 29, 30, 31, 0, 60};
    const unsigned char values[][3] = {{18,52,86}, {101,67,33}, {34,68,102},
                                       {102,51,34}, {19,87,36}, {36,104,19}};
    for(unsigned int i = 0; i < 6; i++)
    {
        std::copy(values[i], values[i] + 3, rgb + 3 * selected[i]);
    }
    mode = ReadFixture("mode-custom.bin");
    response = mode;
    response[1] = 0x83;
    response[20] = 0;
    response[21] = 1;
    assert(controller.SetKeyColors(rgb, REDRAGON_K617_LED_COUNT));
    assert(writes.size() == 3);
    assert(writes[1] == ReadFixture("custom-six-keys.bin"));
    assert(writes[2] == mode);
    assert(controller.GetLastError().empty());

    bool indices[126] = {};
    for(unsigned char index : RedragonK617Controller::key_indices)
    {
        assert(index < 126 && !indices[index]);
        indices[index] = true;
    }
    writes.clear();
    assert(!controller.SetKeyColors(rgb, REDRAGON_K617_LED_COUNT - 1));
    assert(!controller.SetKeyColors(nullptr, REDRAGON_K617_LED_COUNT));
    assert(writes.empty());
    for(int i = 1; i <= 3; i++)
    {
        writes.clear();
        failed_write = i;
        assert(!controller.SetKeyColors(rgb, REDRAGON_K617_LED_COUNT));
        assert((int)writes.size() == i);
    }
    failed_write = -1;
    read_length = 6;
    writes.clear();
    assert(!controller.SetKeyColors(rgb, REDRAGON_K617_LED_COUNT));
    assert(writes.size() == 1);

    // ABNT-only keys use CFG indices 85 and 96 in all three planes.
    read_length = REDRAGON_K617_CONFIG_SIZE;
    writes.clear();
    rgb[61 * 3] = 0x12;
    rgb[61 * 3 + 1] = 0x34;
    rgb[61 * 3 + 2] = 0x56;
    rgb[62 * 3] = 0x65;
    rgb[62 * 3 + 1] = 0x43;
    rgb[62 * 3 + 2] = 0x21;
    assert(controller.SetKeyColors(rgb, REDRAGON_K617_LED_COUNT));
    std::vector<unsigned char> expected = ReadFixture("custom-six-keys.bin");
    expected[8 + 85] = 0x56;
    expected[8 + 85 + 126] = 0x34;
    expected[8 + 85 + 252] = 0x12;
    expected[8 + 96] = 0x21;
    expected[8 + 96 + 126] = 0x43;
    expected[8 + 96 + 252] = 0x65;
    assert(writes[1] == expected);

}

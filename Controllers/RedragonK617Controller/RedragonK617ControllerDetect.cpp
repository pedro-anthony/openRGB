/*---------------------------------------------------------*\
| RedragonK617ControllerDetect.cpp                           |
|                                                           |
|   Opt-in detection for captured Redragon K617 revision     |
|   Pedro Antonio                             05 Oct 2026  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "DetectionManager.h"
#include "LogManager.h"
#include "RGBController_RedragonK617.h"
#include "ResourceManager.h"
#include "SettingsManager.h"

DetectedControllers DetectRedragonK617(hid_device_info* info, const std::string& /*name*/)
{
    DetectedControllers detected_controllers;

    /*-----------------------------------------------------*\
    | Shared VID/PID: require the captured revision and     |
    | descriptor before allowing any configuration writes. |
    | This fingerprint is not a unique model identifier.   |
    \*-----------------------------------------------------*/
    if(!RedragonK617Controller::MatchesDevice(info))
    {
        return(detected_controllers);
    }

    hid_device* dev = hid_open_path(info->path);

    if(!dev)
    {
        LOG_ERROR("[Redragon K617] Could not open %s", info->path);
        return(detected_controllers);
    }

    if(!RedragonK617Controller::MatchesDescriptor(dev))
    {
        hid_close(dev);
        return(detected_controllers);
    }

    double blue_scale = 32.0 / 81.0;
    json settings = ResourceManager::get()->GetSettingsManager()->GetSettings("RedragonK617Settings");
    if(settings.contains("BlueScale") && settings["BlueScale"].is_number())
    {
        blue_scale = settings["BlueScale"].get<double>();
    }
    RedragonK617Controller* controller = new RedragonK617Controller(dev, info->path, blue_scale);
    detected_controllers.push_back(new RGBController_RedragonK617(controller));
    return(detected_controllers);
}

/*---------------------------------------------------------*\
| Disabled until identification and hardware tests are      |
| complete; do not re-enable the FL ESPORTS F11 detector.    |
\*---------------------------------------------------------*/
REGISTER_HID_DETECTOR_IPU_DISABLED("Redragon K617 Fizz (experimental)", DetectRedragonK617, 0x258A, 0x0049, 1, 0xFF00, 1);

/*---------------------------------------------------------*\
| RGBController_RedragonK617.h                               |
|                                                           |
|   OpenRGB interface for Redragon K617 Fizz                 |
|   Pedro Antonio                             05 Oct 2026  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include "RGBController.h"
#include "RedragonK617Controller.h"

class RGBController_RedragonK617 : public RGBController
{
public:
    RGBController_RedragonK617(RedragonK617Controller* controller_ptr);
    ~RGBController_RedragonK617();

    void SetupZones();
    void DeviceUpdateLEDs();
    void DeviceUpdateZoneLEDs(int zone);
    void DeviceUpdateSingleLED(int led);
    void DeviceUpdateMode();

private:
    RedragonK617Controller* controller;
};

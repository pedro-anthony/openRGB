# OpenRGB with Redragon K617 Fizz ABNT support

An unofficial personal fork of [OpenRGB](https://gitlab.com/CalcProgrammer1/OpenRGB) with Static and per-key RGB support for the **Redragon K617 Fizz, 63-key ABNT layout**.

Modified on **2026-10-05** by Pedro Antonio. Based on upstream commit `b9783787b60c444f9558d1fef4c276af6555ee49`. This fork is not an official OpenRGB release or a Redragon product. The original project introduction and notices are preserved in [README.upstream.md](README.upstream.md).

## What works

- **Static:** one color across the whole keyboard.
- **Custom:** independent colors for all 63 ABNT keys.
- A keyboard matrix for OpenRGB's visual LED selection, including the extra `\|` and `/?` keys and vertically aligned `[` and `]` keys.
- Existing OpenRGB profile and command-line functionality.

The owner verified detection, Static RGB, Custom RGB, several individual keys, and the corrected ABNT mappings on Linux. Offline tests also compare generated reports against captured OEM reports and exercise failed-transfer handling. This is not an exhaustive test of every key, platform or hardware revision.

## Supported keyboard revision

| Property | Captured value |
|---|---|
| USB VID:PID | `258a:0049` |
| Manufacturer / product | `BY Tech` / `Gaming Keyboard` |
| USB device release | `1.01` |
| HID interface | `1` |
| Vendor usage page / usage | `FF00` / `01` |
| Layout | ABNT, 63 keys |

Detection also requires an exact match to the captured 227-byte HID report descriptor. The USB release is not an established firmware version. VID/PID and descriptor matching do not prove that another keyboard is compatible: these IDs are shared by other devices. The detector therefore remains **disabled by default**. The existing disabled Sinowealth detector has not been re-enabled.

The ANSI variant and Windows/macOS behavior have not been tested. This fork currently uses the ABNT layout and does not automatically select a regional layout.

## Build on Linux

Install the dependencies described in [OpenRGB's compilation guide](Documentation/Compiling.md). The tested build uses Qt 5, HIDAPI's hidraw backend, libusb and mbedTLS 3.

From the repository root:

```sh
mkdir -p build
cd build
qmake ../OpenRGB.pro
make -j3
```

On the development machine, the installed translation compiler was Qt 6. The following qmake command was used instead:

```sh
qmake ../OpenRGB.pro QT_TOOL.lrelease.binary=/usr/bin/lrelease6
```

This still builds the application against Qt 5. Use the ordinary qmake command when the Qt 5 translation tools are installed.

## Give your user hardware access

Follow [OpenRGB's udev instructions](Documentation/UdevRules.md). For this self-compiled build:

```sh
sudo ./build/openrgb --generate-udev-rules /etc/udev/rules.d/60-openrgb.rules
sudo udevadm control --reload-rules
```

Unplug/replug the keyboard afterward. Avoid having separate `60-openrgb.rules` files in both `/etc/udev/rules.d` and `/usr/lib/udev/rules.d`; a distribution package may already provide the latter.

## Enable the keyboard and set colors

Close other RGB applications and release the keyboard from any VirtualBox guest before using OpenRGB on the host.

```sh
./build/openrgb --gui --noautoconnect
```

1. Open **Settings → Supported Devices** and enable **Redragon K617 Fizz (experimental)**, then click **Save**. Restart or rescan devices to detect it.
2. Select the keyboard and choose **Custom** mode.
3. Click **Toggle LED View**.
4. Click a key or drag-select a group. Hold **Ctrl** to add keys to the selection.
5. Choose a color and click **Apply Colors To Selection**.
6. Save an OpenRGB profile if you want to reuse the color layout.

For a whole-keyboard color:

```sh
./build/openrgb --noautoconnect --device 'Redragon K617 Fizz' --mode Static --color FF0000
```

The local setup also enables the existing **ASUS Aura Motherboard** and **Redragon M711 Cobra** detectors. These are upstream controllers, not new drivers in this fork. The detected hardware was an ASUS TUF GAMING A520M-PLUS II (`0b05:19af`) and an M711 Cobra (`04d9:fc30`); their lighting response has not been verified as part of this fork's testing. Enable motherboard lighting in BIOS if needed.

## Offline test

The test uses mocked HID functions and does not access a physical keyboard. Run it from the repository root:

```sh
g++ -std=c++17 -Wall -Wextra -Werror \
    -IControllers/RedragonK617Controller -I/usr/include/hidapi \
    tests/RedragonK617ControllerTest.cpp \
    Controllers/RedragonK617Controller/RedragonK617Controller.cpp \
    -o build/check-k617
./build/check-k617
```

It checks exact static and six-key custom packets, ABNT-only key positions in the RGB planes, live-profile preservation, conservative detection, duplicate HID collections, invalid inputs and failed transfers. The HIDAPI include path may differ on other systems.

The small fixtures under `tests/data/redragon-k617` contain the HID descriptor and lighting/configuration feature reports. They do not contain keyboard input reports. Full USB captures and personal OpenRGB settings are not included in this repository.

## Protocol and limits

Custom colors use blue, green and red planes spaced 126 bytes apart in the `06 09 BC` feature report. The key indices come from the OEM CFG, with six named-key captures establishing the mapping. Each update reads and validates the live profile before changing its lighting-mode fields, preserving unrelated profile settings. Invalid responses abort the update.

Only Static and Custom are exposed. Built-in animations, brightness adjustment, Direct streaming and explicit save-to-device are not implemented. Existing brightness is preserved; a zero brightness setting can make a valid color update invisible. Persistence of OEM writes across unplugging remains unverified.

Custom reports reproduce the fixed tail observed in both captures rather than reading that tail from the live device. Effects on other OEM presets remain unverified. This controller is intended for occasional color configuration, not continuous software-driven animation.

## License and upstream

The existing [GPL license](LICENSE), source license headers and dependency-specific licenses are retained. The new controller and test files use `GPL-2.0-or-later`. Original OpenRGB code and contributor attribution remain in the repository history.

This repository publishes the modified source. If distributing a compiled version, provide its complete corresponding source and build/install scripts under the applicable GPL terms.

- [Original OpenRGB repository](https://gitlab.com/CalcProgrammer1/OpenRGB)
- [Original README](README.upstream.md)
- [Related K617 support issue](https://gitlab.com/CalcProgrammer1/OpenRGB/-/issues/2172)

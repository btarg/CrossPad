# CrossPad

Cross-platform Node.js bindings for creating virtual gamepads without ViGEm.

CrossPad uses a Linux `uinput` backend and a Windows `WinUHid` backend, with an
Xbox One-style controller profile available on both platforms.

## Build from source

Run these commands from the repository root.

### All platforms

Install:

```powershell
npm install
```

Build the native Node.js addon:

```powershell
npm run build
```

`npm install` builds the addon automatically when no matching prebuilt addon is
present. Run `npm run build` again after changing native source files.

### Windows

The Windows build also builds the WinUHid runtime libraries and UMDF2 driver.
Install these prerequisites first:

- Node.js 18 or newer
- Visual Studio 2022 Build Tools with Desktop C++ and MSBuild, or a compatible
  VS2026 Build Tools installation
- Windows SDK
- Windows Driver Kit (WDK) and UMDF tools
- Python 3
- x64 Spectre-mitigated libraries if MSBuild reports `MSB8040`

The local install step also needs `devcon.exe` from the WDK in one of the
standard Windows Driver Kit tool directories. If `devcon.exe` is unavailable,
use the CI-safe workflow below and install the packaged driver separately.

Build the native addon, WinUHid libraries, and driver, then install the
development driver and verify the device:

```powershell
npm run build:windows
```

This command may request administrator approval. The local WDK test
certificate and driver are for development only.

To build and package the Windows artifacts without installing the device
(the CI-safe workflow), run:

```powershell
npm run build
npm run build:winuhid -- -InstallDriver:$false -SkipDeviceVerification
npm run stage:windows
npm run verify:build
```

`stage:windows` copies the native addon into `prebuilds\win32-x64`.
`verify:build` checks the addon, WinUHid files, driver package, INF structure,
package import, and npm package contents.

### Linux

On Debian or Ubuntu, install the native build prerequisites:

```sh
sudo apt update
sudo apt install build-essential python3
```

Load `uinput` and grant the current user access:

```sh
sudo modprobe uinput
sudo usermod -aG input "$USER"
```

Log out and back in after changing group membership, then build:

```sh
npm install
npm run build
```

If `/dev/uinput` is unavailable, check that the module is loaded and that the
process has read/write access.

### Run the example

```powershell
npm start
```

On Windows, `npm run build:windows` installs the development driver before
starting the example. For a packaged install, use the `installDriver()` helper
described in [Windows driver setup](#windows-driver-setup).

## Install from npm

```powershell
npm install crosspad
```

Published packages include prebuilt native addons and the Windows WinUHid
runtime files. Windows still requires the WinUHid driver to be installed once.
This library includes TypeScript declarations, so no separate `@types` package
is required.

## Xbox One-style controller

Use the explicit Xbox One factory:

```js
const { createXboxOneController } = require('crosspad');

const pad = createXboxOneController();
pad.updateMode = 'manual';
pad.connect();

pad.button.A.setValue(true);
pad.axis.leftX.setValue(-1);
pad.axis.leftTrigger.setValue(0.75);
pad.update();

pad.button.A.setValue(false);
pad.disconnect();
```

The same API is available from TypeScript:

```ts
import {
  createXboxOneController,
  type X360Controller
} from 'crosspad';

const pad: X360Controller = createXboxOneController();
pad.button.A.setValue(true);
pad.axis.leftX.setValue(-1);
pad.update();
```

The included [hello-world example](examples/hello-world.js) creates this
profile, rotates both analog sticks, moves the D-pad, and cycles A/B/X/Y:

```sh
npm start
```

On Windows, the example calls `installDriver()` before connecting.
This will start the UAC prompt for administrator approval if the driver is not already installed.
It will throw an error if the process is not elevated or the driver cannot be installed.

### Installing the Windows driver from Node.js

```js
const { installDriver } = require('crosspad');

try {
  installDriver();
} catch (error) {
  console.error(error.message);
}
```

This is the one-time Windows setup step. It uses UAC for the driver install,
so the user will be asked for admin approval if the driver is not already
installed.

This check runs before the UAC launcher. When the correct driver and device
are already available, `installDriver()` returns without displaying a UAC
prompt.
The call throws if the process is not elevated, the package is missing driver
files, Windows rejects the driver signature, or installation cannot be
completed.

The installer requires `devcon.exe` from the Windows SDK or WDK because the
`pnputil.exe` version shipped on some Windows installations does not support
the `/add-device` command. It searches the system PATH and standard Windows
SDK/WDK tool directories.
When an update is required, it removes the existing WinUHid device instance
before recreating it, matching the upstream local setup script.

For example, run the application normally. Windows should display the standard
administrator-consent prompt. This avoids PowerShell and does not use
`ExecutionPolicy Bypass`:

```text
node app.js
```

Applications should normally run this once during their own setup flow and
then call `createXboxOneController()`:

```js
const {
  installDriver,
  createXboxOneController
} = require('crosspad');

installDriver();

const pad = createXboxOneController();
pad.connect();
```

To remove the virtual device and all matching `winuhiddriver.inf` packages
from the Windows driver store, use the corresponding uninstall helper:

```js
const { uninstallDriver } = require('crosspad');

uninstallDriver(); // Opens the Windows administrator-consent prompt.
```

`uninstallDriver()` removes existing WinUHid device instances with `devcon`
and removes every matching published driver package with
`pnputil /delete-driver /uninstall`. It is Windows x64 only, requires
administrator approval, and should be used when no application is connected
to a virtual controller.

The packaged `scripts/install-driver.js` entry point can also be run directly.
It performs the same preflight and requests UAC elevation itself:

```powershell
node scripts\install-driver.js
node scripts\install-driver.js --uninstall
```

Windows derives the UAC consent dialog application name from the executable
being elevated, so the standard dialog may identify the process as `Node.js`.
The elevated console changes its title to `CrossPad driver installer` or
`CrossPad driver uninstaller` and prints a prominent operation banner before
making any changes.

The package must contain a production-trusted WinUHid driver package for
end-user installation. The development certificate produced by the local WDK
build is only for development and must not be silently trusted or distributed
as a production driver.

Both driver helpers are synchronous. They return normally when the requested
operation succeeds, including when `installDriver()` finds that the correct
driver is already installed. They throw an `Error` when setup fails, the user
cancels UAC, `devcon.exe` is unavailable, the driver package is missing, or
Windows cannot verify the device:

```js
const {
  installDriver,
  uninstallDriver
} = require('crosspad');

try {
  installDriver();
  console.log('WinUHid driver is ready.');
} catch (error) {
  console.error('WinUHid installation failed:', error.message);
}

try {
  uninstallDriver();
  console.log('WinUHid driver was removed.');
} catch (error) {
  console.error('WinUHid uninstallation failed:', error.message);
}
```

Applications can use the thrown error to disable controller functionality,
display setup instructions, or distinguish a cancelled UAC prompt:

```js
try {
  installDriver();
} catch (error) {
  if (error.message.includes('cancelled')) {
    console.log('Driver installation was cancelled.');
  } else {
    console.error(error.message);
  }
}
```

`createXboxOneController()` uses the MIT-licensed upstream WinUHidDevs Xbox
One preset on Windows. On Linux it creates an evdev/uinput device named
`Xbox One Controller` with vendor/product ID `045e:02ff`.

The default factory remains available:

```js
const { createX360Controller } = require('crosspad');
const pad = createX360Controller();
```

The default uses the generic HID/uinput profile and is not the Xbox One
preset.

## JavaScript API

### Factories

```js
const {
  createX360Controller,
  createXboxOneController
} = require('crosspad');
```

Both factories return a controller with:

```js
pad.connect();
pad.disconnect();
pad.update();
pad.resetInputs();
```

Set `updateMode` to `auto` (the default) to submit every input change, or to
`manual` to batch changes and call `update()` yourself:

```js
pad.updateMode = 'manual';
pad.axis.leftX.setValue(-1);
pad.axis.leftY.setValue(0.5);
pad.button.A.setValue(true);
pad.update();
```

Available buttons:

```text
START BACK LEFT_THUMB RIGHT_THUMB LEFT_SHOULDER RIGHT_SHOULDER
GUIDE A B X Y
```

Available axes:

```text
leftX leftY rightX rightY
leftTrigger rightTrigger
dpadHorz dpadVert
```

Stick values use `-1` to `1`. Trigger values use `0` to `1`. D-pad axes use
negative, zero, and positive values.

Always disconnect the controller during shutdown:

```js
process.once('SIGINT', () => {
  pad.resetInputs();
  pad.disconnect();
  process.exit(0);
});
```

## Linux setup

The Linux backend uses `/dev/uinput` directly.

If `/dev/uinput` is unavailable, check that the module is loaded and that the
process has read/write access. A uinput device is an evdev device; it is not
Linux XInput.

### SDL on Linux

SDL's joystick API should enumerate the device. SDL's Gamepad API is
mapping-driven, however, and individual games may require a mapping for the
device GUID. A uinput device cannot implement Microsoft's proprietary XInput
protocol.

For an SDL application that accepts custom mappings, the CrossPad Xbox One
layout is:

```text
a:b7,b:b8,x:b9,y:b10,back:b1,guide:b6,start:b0,leftstick:b2,rightstick:b3,leftshoulder:b4,rightshoulder:b5,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,dpup:-a7,dpdown:+a7,dpleft:-a6,dpright:+a6
```

Confirm the complete device GUID and element indices with the SDL version and
test utility used by the target game. Games that do not accept custom
mappings must provide their own mapping.

## Windows driver setup

CrossPad includes the MIT-licensed [WinUHid project](https://github.com/cgutman/WinUHid).
The Windows backend needs both the runtime DLLs and the UMDF2 driver installed
on the machine.

For local debugging, you can also override the bundled WinUHid libraries:

```powershell
$env:WINUHID_DLL = 'C:\path\to\WinUHid.dll'
$env:WINUHID_DEVS_DLL = 'C:\path\to\WinUHidDevs.dll'
npm start
```

If a controller fails to connect on Windows, make sure the driver is installed,
the `\\.\WinUHid` interface exists, and the system is not blocking the
signature or driver install.

## Compatibility limitations

The Xbox One profile is an Xbox One-style HID/GameInput device. It is not a
true XUSB bus device and does not guarantee compatibility with applications
that exclusively call XInput. A VID/PID or HID descriptor cannot turn a
generic HID device into an XUSB device.

The Windows generic backend deliberately uses a neutral identity and a
minimal joystick descriptor. This avoids pretending that a generic HID report
is an authentic Microsoft XUSB device.

It has been tested and verified to work with SDL3 applications and the browser Gamepad API (e.g. Hardware Tester).

## WinUHid attribution and license

CrossPad includes and builds portions of
[cgutman/WinUHid](https://github.com/cgutman/WinUHid), including its
`WinUHidDevs` Xbox One preset. WinUHid is distributed under the MIT License.
The original license and copyright notice are preserved in
[vendor/WinUHid/LICENSE](vendor/WinUHid/LICENSE).

The WinUHid source and preset are used directly from the upstream MIT-licensed
repository.

CrossPad itself is distributed under the MIT License.

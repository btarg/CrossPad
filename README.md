# CrossPad

Cross-platform Node.js bindings for creating virtual gamepads. CrossPad
provides a Linux `uinput` backend and a Windows WinUHid backend, with an
Xbox One-style controller profile available on both platforms.

CrossPad does not use ViGEm. The JavaScript API is intentionally small and
works with the same button and axis state on each supported platform.

The package includes first-party TypeScript declarations for the public API,
so no separate `@types` package is required.

## Requirements

### All platforms

- Node.js 18 or newer
- A C++17 compiler supported by `node-gyp`
- Python 3

Install dependencies and build the native addon:

```powershell
pnpm install
pnpm run build
```

`pnpm install` runs the native build through the package's install script.
Run `pnpm run build` after changing native source files.

## Installing from npm

Published releases include prebuilt native addons for Linux x64 and Windows
x64, plus the Windows WinUHid user libraries. A consumer does not need a
compiler for those platforms:

```sh
pnpm add crosspad
```

The package selects the native addon for the current `process.platform` and
`process.arch`. The Windows UMDF2 driver still must be installed separately
with the signed driver package and administrator privileges; shipping a DLL
alone cannot install a Windows device driver. The package exposes an explicit
`installDriver()` helper for this setup step.

The [GitHub Actions workflow](.github/workflows/package.yml) builds both
platforms, generates the Windows driver artifacts, verifies the package, and
publishes version tags when `NPM_TOKEN` is configured. Generated artifacts are
not committed to Git.

To test the exact package locally, create a tarball and install it in a
separate Node.js 18+ project:

```sh
mkdir /tmp/crosspad-package-test
cd /tmp/crosspad-package-test
pnpm init
pnpm install /absolute/path/to/crosspad-0.1.0.tgz
node -e "const p=require('crosspad'); console.log(Object.keys(p))"
```

Create the tarball with `pnpm pack`. Inspect its contents without creating a
tarball with:

```powershell
pnpm pack --dry-run
```

The package contains a platform-specific addon under `prebuilds/` and, on
Windows, the WinUHid runtime DLLs and driver package.

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

On Windows, the example calls `installDriver()` before connecting. Run it from
an elevated terminal so it can verify the packaged driver version and install
or update the driver only when necessary.

The example wraps driver setup in `try/catch`, prints the error, and stops
before creating the controller if installation or verification fails. This
demonstrates the error-handling pattern applications should use.

### Installing the Windows driver from Node.js

On Windows x64, install the packaged WinUHid driver before connecting the
first controller:

```js
const { installDriver } = require('crosspad');

installDriver(); // Requires an already elevated process.
```

`installDriver()` is intentionally explicit and only works on Windows x64.
It runs a Node.js installer that queries Windows with `pnputil.exe`, then
creates and installs the packaged INF for the root-enumerated
`Root\WinUHid` device with `devcon.exe`, and
verifies the `\\.\WinUHid` device interface. The calling process must already
be running on Windows with a user account allowed to approve UAC. The native
Windows launcher starts the installer with the standard UAC `runas` verb, so
the main Node.js process does not need to be elevated beforehand.
Before installing, it compares the packaged INF `DriverVer` value with the
installed driver matched by the `Root\WinUHid` hardware ID and skips the
installation when they match. The device's instance ID may be something like
`ROOT\SYSTEM\0007`, so the installer does not assume that it is
`ROOT\WinUHid`. If Windows does not expose a readable version, it safely
proceeds with the installation.

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

Install the native build prerequisites on Debian or Ubuntu:

```sh
sudo apt update
sudo apt install build-essential python3
```

Load the uinput kernel module and grant the current user access:

```sh
sudo modprobe uinput
sudo usermod -aG input "$USER"
```

Log out and back in after changing group membership, then build and run:

```sh
npm install
npm start
```

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

## Windows setup

The Windows backend uses the MIT-licensed [WinUHid project](https://github.com/cgutman/WinUHid):

- `WinUHid.dll` is the user-mode WinUHid client library.
- `WinUHidDevs.dll` contains the upstream Xbox One preset.
- `WinUHidDriver.dll` is the UMDF2 driver installed by the setup script.

WinUHid is user-mode from an implementation perspective, but it is still a
Windows device driver and requires installation into the driver store. A DLL
copied beside the Node application cannot replace that installation.

Install these prerequisites:

- Visual Studio 2022 Build Tools with the C++ build tools, or a tested
  Visual Studio 2026 Build Tools installation
- Windows SDK
- Windows Driver Kit (Sometimes also called Windows Driver Kit Build Tools)
- x64 Spectre-mitigated libraries if MSBuild reports `MSB8040`

Visual Studio 2026 can be used if the installed WDK supports it. The vendored
WinUHid user-library projects currently request the VS2022 `v143` toolset, so
keep the VS2022 C++ toolset installed or migrate those projects to the
VS2026 toolset and test the complete driver build. MSBuild discovery is
automatic; the script uses `PATH`, `vswhere`, and known installation paths.

Build and install the local WinUHid driver:

```powershell
pnpm run build:windows
```

To build the driver without installing it, use the CI-safe flow:

```powershell
pnpm run build
pnpm run build:winuhid -- -InstallDriver:$false -SkipDeviceVerification
pnpm run stage:windows
pnpm run verify:build
```

The verification checks that these artifacts exist and are packaged correctly:

```text
vendor/WinUHid/bin/win32-x64/WinUHid.dll
vendor/WinUHid/bin/win32-x64/WinUHidDevs.dll
vendor/WinUHid/package/WinUHidDriver.dll
vendor/WinUHid/package/WinUHidDriver.inf
vendor/WinUHid/package/winuhiddriver.cat
```

The Node.js installer requires administrator privileges because it creates the
root-enumerated WinUHid device. It also verifies the `\\.\WinUHid` interface.
The development build uses a WDK test certificate and is intended only for
local development. Production distribution requires a production-trusted
driver signature; do not silently trust a developer certificate on a user's
machine.

The bundled DLLs are selected automatically. To use local builds instead:

```powershell
$env:WINUHID_DLL = 'C:\path\to\WinUHid.dll'
$env:WINUHID_DEVS_DLL = 'C:\path\to\WinUHidDevs.dll'
pnpm start
```

`stage:windows` copies the addon from `build\Release\` into `prebuilds\`.
`verify:build` checks file presence, INF structure, package import, and npm
package contents. Both commands return a non-zero exit code on failure and are
used by CI.

If `connect()` reports Windows error 2, the driver package is not installed,
the device interface is unavailable, or Windows rejected the driver
signature. Check Device Manager and the setup output.

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

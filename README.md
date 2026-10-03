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

```sh
npm install
npm run build
```

`npm install` runs the native build through the package's install script.
Run `npm run build` when rebuilding after changing native source files.
pnpm is optional and can be used instead if you prefer it.

## Installing from npm

Published releases include prebuilt native addons for Linux x64 and Windows
x64, plus the Windows WinUHid user libraries. A consumer does not need a
compiler for those platforms:

```sh
npm install crosspad
```

The package selects the native addon for the current `process.platform` and
`process.arch`. The Windows UMDF2 driver still must be installed separately
with the signed driver package and administrator privileges; shipping a DLL
alone cannot install a Windows device driver. The package exposes an explicit
`installDriver()` helper for this setup step.

The repository's [GitHub Actions workflow](.github/workflows/package.yml)
rebuilds both platform addons, builds and packages the WinUHid user libraries
and driver on Windows, checks the npm tarball, and publishes version tags of the form `v1.2.3` when
the `NPM_TOKEN` repository secret is configured. Pull requests build and
inspect the package without publishing it.

Generated Windows driver files are not committed to Git. The Windows CI job
builds `WinUHidDriver.dll`, its INF, and catalog, uploads them with the other
runtime artifacts, and the package job assembles the final npm tarball from
those artifacts.

To test the exact package locally before publishing:

```sh
npm pack
```

This creates a tarball in the repository directory. Install that tarball into
a separate temporary project using Node.js 18 or newer:

```sh
mkdir /tmp/crosspad-package-test
cd /tmp/crosspad-package-test
npm init -y
npm install /absolute/path/to/crosspad-0.1.0.tgz
node -e "const p=require('crosspad'); console.log(Object.keys(p))"
```

The install should use the prebuilt addon without compiling. The package
loader selects:

```text
prebuilds/linux-x64/virtual_x360.node
prebuilds/win32-x64/virtual_x360.node
```

On Windows, the published package also contains:

```text
vendor/WinUHid/bin/win32-x64/WinUHid.dll
vendor/WinUHid/bin/win32-x64/WinUHidDevs.dll
```

You can inspect the exact tarball contents without creating it permanently:

```sh
npm pack --dry-run
```

After a GitHub Actions run completes, download the `linux-package-files` or
`windows-package-files` artifact from the workflow run's **Artifacts** section.
The release `package` job combines those files with the checked-out source and
verifies the final npm tarball. A tag build publishes the same package when
`NPM_TOKEN` is configured.

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

The package must contain a production-trusted WinUHid driver package for
end-user installation. The development certificate produced by the local WDK
build is only for development and must not be silently trusted or distributed
as a production driver.

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

- Visual Studio 2022 Build Tools with the C++ build tools
- Windows SDK
- Windows Driver Kit (WDK), including the UMDF tools
- x64 Spectre-mitigated libraries if MSBuild reports `MSB8040`

Build the addon and build/install the local WinUHid driver:

```powershell
npm run build:windows
```

Or build only the WinUHid libraries and driver:

```powershell
npm run build:winuhid
```

The script builds and bundles:

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

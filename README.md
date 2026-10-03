# Native X360 controller

This is a small Node.js native addon that creates a virtual Xbox 360-style
gamepad without ViGEm. It deliberately keeps the JavaScript API close to
`node-ViGEmClient` while putting the OS-specific device implementation behind
the addon boundary.

## Linux

The included backend uses `/dev/uinput` directly. Install the kernel uinput
module and give the running user access to `/dev/uinput` (for example through
the `uinput` group), then:

```sh
pnpm install
pnpm start
```

The device is an Xbox-style Linux input device. `inputtino` can replace the
direct uinput implementation later without changing the JavaScript API.

## Windows / WinUHid

The addon now includes a WinUHid backend. It loads `WinUHid.dll` at runtime,
creates a generic HID joystick report, and submits the same button/stick/trigger
state as the Linux backend. The Windows report intentionally uses a minimal
conventional joystick layout rather than the Gamepad usage that caused
Windows Gaming Input crashes.
The D-pad is represented as four additional HID buttons, allowing SDL3 menu
navigation without relying on a Windows hat-switch parser.
This does not link ViGEm.

An opt-in Xbox One HID backend is also included. It uses the MIT-licensed
upstream `WinUHidDevs` Xbox One preset rather than a locally recreated
descriptor:

```js
const { createXboxOneController } = require('native-x360-pad');
const pad = createXboxOneController();
pad.connect();
```

The preset uses the upstream Xbox One HID descriptor, report layout, hat
switch encoding, and device identity (`045e:02ff`). This is an Xbox One-style
HID/GameInput device, not an XUSB bus device, so applications that require
exclusive XInput support may still not detect it. The generic backend remains
the default for `createX360Controller()`.

On Linux, `createXboxOneController()` creates an evdev/uinput device named
`Xbox One Controller` with vendor/product `045e:02ff` and the standard gamepad
button and axis events. SDL's joystick API should enumerate it, but SDL's
Gamepad API is mapping-driven: a game must contain or load a mapping for that
device GUID. A uinput device cannot create Microsoft's XInput protocol, so
there is no library-side way to guarantee detection by every SDL game.

For SDL applications you control, load an explicit mapping after opening SDL:

```text
030000005e040000ff02000000007800,Xbox One Controller,a:b7,b:b8,x:b9,y:b10,back:b1,guide:b6,start:b0,leftstick:b2,rightstick:b3,leftshoulder:b4,rightshoulder:b5,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,dpup:-a7,dpdown:+a7,dpleft:-a6,dpright:+a6,
```

The exact GUID and element indices must be confirmed with the target SDL
version's joystick test utility. Games that do not accept custom mappings must
ship their own mapping or use SDL's joystick API instead of assuming every
device is an Xbox controller.

WinUHid still requires its separately built and signed **UMDF2** driver package
to be installed. UMDF2 is user-mode, but the package is still a system driver
and cannot be replaced by a DLL copied next to the Node application. Build and
install the WinUHid driver and user library from
[WinUHid](https://github.com/cgutman/WinUHid), then make `WinUHid.dll`
available on `PATH` or beside the Node executable.

The addon also accepts an explicit DLL path. This is useful when testing a
local build:

```powershell
$env:WINUHID_DLL = 'C:\path\to\WinUHid\x64\Release\WinUHid.dll'
pnpm build
pnpm start
```

The MIT-licensed user-mode and UMDF2 driver source is vendored under
[`vendor/WinUHid`](vendor/WinUHid). The Windows setup command automatically
builds the DLL and driver, requests administrator elevation, installs the
driver package, and verifies the device interface:

```powershell
pnpm build:winuhid
```

The package then loads
`vendor/WinUHid/bin/win32-x64/WinUHid.dll` automatically. The DLL is
redistributed under the MIT license; see
[`vendor/WinUHid/LICENSE`](vendor/WinUHid/LICENSE).
`pnpm build:winuhid` also builds and bundles
`vendor/WinUHid/bin/win32-x64/WinUHidDevs.dll`, which contains the upstream
MIT-licensed Xbox One preset.

The command requires the WDK UMDF toolset and a valid driver signature. It is
intentionally not part of `pnpm install`; ordinary dependency installation
must not silently modify Windows drivers. Use `pnpm build:windows` to build the
Node addon and perform the complete Windows setup in one command.

If the driver build reports `MSB8040`, modify the **Visual Studio 2022 Build
Tools** installation and add **Libs for Spectre** under **Individual
components**. For an x64 build, the required files are the x64 Spectre-mitigated
libraries. The Visual Studio component ID is
`Microsoft.VisualStudio.Component.VC.SpectreMitigation`.

With WDK 10.0.26100, the build script passes
`SkipPackageVerification=true` because that WDK release may omit the x86
`InfVerif.dll` loaded by Visual Studio's package-verification task. Driver
compilation, catalog generation, and test signing still run; the script then
checks that the generated driver files exist before attempting installation.

If `connect()` reports Windows error 2 after setup, the driver package was not
installed successfully or Windows rejected its signature. Check Device
Manager and the `pnputil` output from the setup command.

The local build uses the WDK's test certificate. The elevated setup step
imports the generated `WinUHidDriver.cer` into the local machine Root and
Trusted Publisher stores before installing the driver with `devcon`. This is only suitable for
development on your own machine. A distributed package must use a
production-trusted driver signature and should not silently install a
developer certificate.

The development INF also grants built-in users read/write access to the
WinUHid control device, so the Node.js process does not need to run elevated.

### XInput limitation

An Xbox 360 controller uses Microsoft's proprietary XUSB protocol; it is not a
standard HID controller. The Windows backend intentionally uses a
standards-based gamepad identity rather than spoofing Microsoft's VID/PID.
This allows SDL3's generic HID backend and browser Gamepad API to enumerate the
device, but it does not make it an XInput controller. A generic WinUHid device
cannot become an XInput controller merely by changing its VID/PID or HID
descriptor.

Useful references:

- [MS-XUSBI protocol specification](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-xusbi/c79474e7-3968-43d1-8d2f-175d47bef43e)
- [Microsoft: DirectInput and XUSB devices](https://learn.microsoft.com/en-us/windows/win32/xinput/directinput-and-xusb-devices)
- [Xbox 360 HID descriptor](https://gist.github.com/fendent/5709856)

## JavaScript API

```js
const { createX360Controller } = require('native-x360-pad');
const pad = createX360Controller();
pad.updateMode = 'manual';
pad.connect();
pad.button.A.setValue(true);
pad.axis.leftX.setValue(-1);
pad.update();
pad.disconnect();
```

Buttons are `START, BACK, LEFT_THUMB, RIGHT_THUMB, LEFT_SHOULDER,
RIGHT_SHOULDER, GUIDE, A, B, X, Y`. Axes are `leftX, leftY, rightX, rightY,
leftTrigger, rightTrigger, dpadHorz, dpadVert`.

Do not launch the Xbox One backend while Chromium-based applications are open
until it has been validated on the target Windows installation. HID
enumeration bugs in an application or Windows Gaming Input can affect every
process that enumerates the new device. Disconnect the controller and remove
the WinUHid device with the driver tooling if enumeration causes instability.

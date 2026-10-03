#include <node_api.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef __linux__
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

#ifdef _WIN32
struct WinUhidApi;
struct WinUhidXOneApi;
#endif

struct State {
  bool connected = false;
  bool xbox_one = false;
  std::array<bool, 11> buttons{};
  std::array<double, 8> axes{};
#ifdef __linux__
  int fd = -1;
#endif
#ifdef _WIN32
  HMODULE winuhid = nullptr;
  HMODULE winuhid_devs = nullptr;
  void* win_device = nullptr;
  void* xbox_one_device = nullptr;
  WinUhidApi* win_api = nullptr;
  WinUhidXOneApi* xbox_one_api = nullptr;
#endif
};

#ifdef _WIN32
#pragma pack(push, 1)
struct WinUhidConfig {
  ULONG supported_events;
  USHORT vendor_id;
  USHORT product_id;
  USHORT version;
  USHORT descriptor_length;
  const void* descriptor;
  GUID container_id;
  const wchar_t* instance_id;
  const wchar_t* hardware_ids;
  UINT read_report_period_us;
};
#pragma pack(pop)

using WinUhidDevice = void*;
using WinUhidCreateDevice = WinUhidDevice (*)(const WinUhidConfig*);
using WinUhidStartDevice = BOOL (*)(WinUhidDevice, void*, void*);
using WinUhidSubmitInputReport = BOOL (*)(WinUhidDevice, const void*, DWORD);
using WinUhidStopDevice = void (*)(WinUhidDevice);
using WinUhidDestroyDevice = void (*)(WinUhidDevice);

struct WinUhidApi {
  WinUhidCreateDevice create = nullptr;
  WinUhidStartDevice start = nullptr;
  WinUhidSubmitInputReport submit = nullptr;
  WinUhidStopDevice stop = nullptr;
  WinUhidDestroyDevice destroy = nullptr;
};

#pragma pack(push, 1)
struct WinUhidXOneInputReport {
  USHORT left_stick_x;
  USHORT left_stick_y;
  USHORT right_stick_x;
  USHORT right_stick_y;
  USHORT left_trigger : 10;
  USHORT right_trigger : 10;
  UCHAR button_a : 1;
  UCHAR button_b : 1;
  UCHAR button_x : 1;
  UCHAR button_y : 1;
  UCHAR button_lb : 1;
  UCHAR button_rb : 1;
  UCHAR button_back : 1;
  UCHAR button_menu : 1;
  UCHAR button_ls : 1;
  UCHAR button_rs : 1;
  UCHAR reserved3 : 6;
  UCHAR hat : 4;
  UCHAR reserved4 : 4;
  UCHAR button_home : 1;
  UCHAR reserved5 : 7;
  UCHAR battery_level;
};
#pragma pack(pop)

using WinUhidXOneGamepad = void*;
using WinUhidXOneCreate = WinUhidXOneGamepad (*)(const void*, void*, void*);
using WinUhidXOneInitializeInputReport = void (*)(WinUhidXOneInputReport*);
using WinUhidXOneSetHatState = void (*)(WinUhidXOneInputReport*, int, int);
using WinUhidXOneReportInput = BOOL (*)(WinUhidXOneGamepad, const WinUhidXOneInputReport*);
using WinUhidXOneDestroy = void (*)(WinUhidXOneGamepad);

struct WinUhidXOneApi {
  WinUhidXOneCreate create = nullptr;
  WinUhidXOneInitializeInputReport initialize = nullptr;
  WinUhidXOneSetHatState set_hat = nullptr;
  WinUhidXOneReportInput report = nullptr;
  WinUhidXOneDestroy destroy = nullptr;
};

const unsigned char kGenericJoystickHidDescriptor[] = {
    0x05, 0x01, 0x09, 0x04, 0xA1, 0x01,
    0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0F,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x0F, 0x81, 0x02,
    0x75, 0x01, 0x95, 0x01, 0x81, 0x01,
    0x75, 0x08, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x95, 0x02,
    0x05, 0x01, 0x09, 0x32, 0x09, 0x35, 0x81, 0x02,
    0x75, 0x10, 0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F, 0x95, 0x04,
    0x09, 0x30, 0x09, 0x31, 0x09, 0x33, 0x09, 0x34,
    0x81, 0x02,
    0xC0};

std::string WinError(const char* operation) {
  const DWORD error = GetLastError();
  std::string message = std::string(operation) + " failed (Windows error " +
                         std::to_string(error) + ")";
  if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
    message +=
        ": the WinUHid UMDF2 driver is not installed or its \\\\.\\WinUHid "
        "device interface is unavailable";
  } else if (error == ERROR_ACCESS_DENIED) {
    message += ": access denied; check the installed driver and permissions";
  }
  return message;
}

WinUhidApi LoadWinUhid(State* state) {
  const char* configured_path = std::getenv("WINUHID_DLL");
  if (configured_path && configured_path[0] != '\0') {
    int length = MultiByteToWideChar(CP_UTF8, 0, configured_path, -1, nullptr, 0);
    std::wstring wide_path(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, configured_path, -1, wide_path.data(), length);
    state->winuhid = LoadLibraryW(wide_path.c_str());
  } else {
    state->winuhid = LoadLibraryW(L"WinUHid.dll");
  }
  if (!state->winuhid) {
    throw std::runtime_error(
        "WinUHid.dll could not be loaded. Build WinUHid\\WinUHid.vcxproj, "
        "install the WinUHid UMDF2 driver package, and set WINUHID_DLL to the "
        "built DLL path.");
  }
  WinUhidApi api;
  api.create = reinterpret_cast<WinUhidCreateDevice>(GetProcAddress(state->winuhid, "WinUHidCreateDevice"));
  api.start = reinterpret_cast<WinUhidStartDevice>(GetProcAddress(state->winuhid, "WinUHidStartDevice"));
  api.submit = reinterpret_cast<WinUhidSubmitInputReport>(GetProcAddress(state->winuhid, "WinUHidSubmitInputReport"));
  api.stop = reinterpret_cast<WinUhidStopDevice>(GetProcAddress(state->winuhid, "WinUHidStopDevice"));
  api.destroy = reinterpret_cast<WinUhidDestroyDevice>(GetProcAddress(state->winuhid, "WinUHidDestroyDevice"));
  if (!api.create || !api.start || !api.submit || !api.stop || !api.destroy) {
    FreeLibrary(state->winuhid);
    state->winuhid = nullptr;
    throw std::runtime_error("WinUHid.dll does not export the required API");
  }
  return api;
}

void ConfigureWindowsDevice(State* state) {
  if (state->xbox_one) {
    // WinUHidDevs.dll imports the base WinUHid.dll. Load the base library
    // first so the preset can resolve that dependency from the bundled path.
    state->win_api = new WinUhidApi(LoadWinUhid(state));
    const char* configured_path = std::getenv("WINUHID_DEVS_DLL");
    if (configured_path && configured_path[0] != '\0') {
      int length = MultiByteToWideChar(CP_UTF8, 0, configured_path, -1, nullptr, 0);
      std::wstring wide_path(length, L'\0');
      MultiByteToWideChar(CP_UTF8, 0, configured_path, -1, wide_path.data(), length);
      state->winuhid_devs =
          LoadLibraryExW(wide_path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    } else {
      state->winuhid_devs = LoadLibraryW(L"WinUHidDevs.dll");
    }
    if (!state->winuhid_devs) {
      throw std::runtime_error(WinError("WinUHidDevs.dll load"));
    }
    auto* api = new WinUhidXOneApi();
    api->create = reinterpret_cast<WinUhidXOneCreate>(
        GetProcAddress(state->winuhid_devs, "WinUHidXOneCreate"));
    api->initialize = reinterpret_cast<WinUhidXOneInitializeInputReport>(
        GetProcAddress(state->winuhid_devs, "WinUHidXOneInitializeInputReport"));
    api->set_hat = reinterpret_cast<WinUhidXOneSetHatState>(
        GetProcAddress(state->winuhid_devs, "WinUHidXOneSetHatState"));
    api->report = reinterpret_cast<WinUhidXOneReportInput>(
        GetProcAddress(state->winuhid_devs, "WinUHidXOneReportInput"));
    api->destroy = reinterpret_cast<WinUhidXOneDestroy>(
        GetProcAddress(state->winuhid_devs, "WinUHidXOneDestroy"));
    if (!api->create || !api->initialize || !api->set_hat || !api->report ||
        !api->destroy) {
      delete api;
      FreeLibrary(state->winuhid_devs);
      state->winuhid_devs = nullptr;
      throw std::runtime_error(
          "WinUHidDevs.dll does not export the Xbox One preset API");
    }
    state->xbox_one_api = api;
    state->xbox_one_device = api->create(nullptr, nullptr, nullptr);
    if (!state->xbox_one_device) {
      const std::string error = WinError("WinUHidXOneCreate");
      delete api;
      state->xbox_one_api = nullptr;
      FreeLibrary(state->winuhid_devs);
      state->winuhid_devs = nullptr;
      throw std::runtime_error(error);
    }
    return;
  }
  auto* api = new WinUhidApi(LoadWinUhid(state));
  state->win_api = api;
  WinUhidConfig config{};
  // Do not impersonate Microsoft's VID/PID: SDL routes those IDs through
  // its XUSB-specific backend, which cannot consume a generic HID report.
  config.vendor_id = 0x1209;
  config.product_id = 0x0001;
  config.version = 1;
  config.descriptor_length = sizeof(kGenericJoystickHidDescriptor);
  config.descriptor = kGenericJoystickHidDescriptor;
  config.read_report_period_us = 1000;
  state->win_device = api->create(&config);
  if (!state->win_device) {
    delete api;
    state->win_api = nullptr;
    throw std::runtime_error(WinError("WinUHidCreateDevice"));
  }
  if (!api->start(state->win_device, nullptr, nullptr)) {
    api->destroy(state->win_device);
    state->win_device = nullptr;
    delete api;
    state->win_api = nullptr;
    throw std::runtime_error(WinError("WinUHidStartDevice"));
  }
}

void SubmitWindowsReport(State* state) {
  if (state->xbox_one) {
    auto* api = state->xbox_one_api;
    WinUhidXOneInputReport report{};
    api->initialize(&report);
    auto stick = [](double value) -> USHORT {
      const double normalized = std::clamp(value, -1.0, 1.0);
      return static_cast<USHORT>(std::lround((normalized + 1.0) * 32767.0));
    };
    auto trigger = [](double value) -> USHORT {
      return static_cast<USHORT>(
          std::lround(std::clamp(value, 0.0, 1.0) * 1023.0));
    };
    report.left_stick_x = stick(state->axes[0]);
    report.left_stick_y = stick(state->axes[1]);
    report.right_stick_x = stick(state->axes[2]);
    report.right_stick_y = stick(state->axes[3]);
    report.left_trigger = trigger(state->axes[4]);
    report.right_trigger = trigger(state->axes[5]);
    report.button_menu = state->buttons[0];
    report.button_back = state->buttons[1];
    report.button_ls = state->buttons[2];
    report.button_rs = state->buttons[3];
    report.button_lb = state->buttons[4];
    report.button_rb = state->buttons[5];
    report.button_home = state->buttons[6];
    report.button_a = state->buttons[7];
    report.button_b = state->buttons[8];
    report.button_x = state->buttons[9];
    report.button_y = state->buttons[10];
    api->set_hat(&report,
                 state->axes[6] < -0.5 ? -1 : state->axes[6] > 0.5 ? 1 : 0,
                 state->axes[7] < -0.5 ? -1 : state->axes[7] > 0.5 ? 1 : 0);
    if (!api->report(state->xbox_one_device, &report)) {
      throw std::runtime_error(WinError("WinUHidXOneReportInput"));
    }
    return;
  }
  unsigned char report[12]{};
  unsigned short buttons = 0;
  for (size_t i = 0; i < state->buttons.size(); ++i) {
    if (state->buttons[i]) buttons |= static_cast<unsigned short>(1u << i);
  }
  const bool right = state->axes[6] > 0.5;
  const bool left = state->axes[6] < -0.5;
  const bool down = state->axes[7] > 0.5;
  const bool up = state->axes[7] < -0.5;
  if (up) buttons |= 1u << 11;
  if (down) buttons |= 1u << 12;
  if (left) buttons |= 1u << 13;
  if (right) buttons |= 1u << 14;
  report[0] = static_cast<unsigned char>(buttons & 0xff);
  report[1] = static_cast<unsigned char>((buttons >> 8) & 0xff);
  report[2] = static_cast<unsigned char>(
      std::lround(std::clamp(state->axes[4], 0.0, 1.0) * 255.0));
  report[3] = static_cast<unsigned char>(
      std::lround(std::clamp(state->axes[5], 0.0, 1.0) * 255.0));
  auto stick = [&](size_t index, size_t offset) {
    const auto value = static_cast<short>(std::lround(std::clamp(state->axes[index], -1.0, 1.0) * 32767.0));
    std::memcpy(report + offset, &value, sizeof(value));
  };
  stick(0, 4);
  stick(1, 6);
  stick(2, 8);
  stick(3, 10);
  if (!state->win_api->submit(state->win_device, report, sizeof(report))) {
    throw std::runtime_error(WinError("WinUHidSubmitInputReport"));
  }
}
#endif

constexpr const char* kButtonNames[] = {
    "START", "BACK", "LEFT_THUMB", "RIGHT_THUMB", "LEFT_SHOULDER",
    "RIGHT_SHOULDER", "GUIDE", "A", "B", "X", "Y"};
constexpr const char* kAxisNames[] = {
    "leftX", "leftY", "rightX", "rightY",
    "leftTrigger", "rightTrigger", "dpadHorz", "dpadVert"};

void Throw(napi_env env, const std::string& message) {
  napi_throw_error(env, nullptr, message.c_str());
}

bool GetState(napi_env env, napi_callback_info info, State** state,
              size_t argc = 0, napi_value* argv = nullptr) {
  napi_value this_arg;
  size_t actual_argc = argc;
  napi_status status = napi_get_cb_info(env, info, &actual_argc, argv, &this_arg, nullptr);
  if (status != napi_ok) {
    Throw(env, "Unable to read native controller arguments");
    return false;
  }
  status = napi_unwrap(env, this_arg, reinterpret_cast<void**>(state));
  if (status != napi_ok || *state == nullptr) {
    Throw(env, "Controller has already been destroyed");
    return false;
  }
  return true;
}

#ifdef __linux__
void Emit(State* state, unsigned short type, unsigned short code, int value) {
  input_event event{};
  event.type = type;
  event.code = code;
  event.value = value;
  if (write(state->fd, &event, sizeof(event)) != static_cast<ssize_t>(sizeof(event))) {
    throw std::runtime_error("write(/dev/uinput) failed");
  }
}

void Sync(State* state) { Emit(state, EV_SYN, SYN_REPORT, 0); }

int AxisValue(size_t index, double value) {
  if (index >= 6) {
    return value >= 0.5 ? 1 : 0;
  }
  if (index == 4 || index == 5) {
    return static_cast<int>(std::lround(std::clamp(value, 0.0, 1.0) * 255.0));
  }
  return static_cast<int>(std::lround(std::clamp(value, -1.0, 1.0) * 32767.0));
}

void ConfigureLinuxDevice(State* state) {
  state->fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (state->fd < 0) {
    throw std::runtime_error("Cannot open /dev/uinput; grant access to uinput or run with appropriate permissions");
  }

  if (ioctl(state->fd, UI_SET_EVBIT, EV_KEY) < 0 ||
      ioctl(state->fd, UI_SET_EVBIT, EV_ABS) < 0) {
    close(state->fd);
    state->fd = -1;
    throw std::runtime_error("Cannot configure /dev/uinput");
  }

  const int keys[] = {BTN_START, BTN_SELECT, BTN_THUMBL, BTN_THUMBR,
                      BTN_TL, BTN_TR, BTN_MODE, BTN_A, BTN_B, BTN_X, BTN_Y};
  for (int key : keys) {
    if (ioctl(state->fd, UI_SET_KEYBIT, key) < 0) {
      close(state->fd);
      state->fd = -1;
      throw std::runtime_error("Cannot configure virtual controller buttons");
    }
  }
  const int axes[] = {ABS_X, ABS_Y, ABS_RX, ABS_RY, ABS_Z, ABS_RZ, ABS_HAT0X, ABS_HAT0Y};
  for (int axis : axes) {
    if (ioctl(state->fd, UI_SET_ABSBIT, axis) < 0) {
      close(state->fd);
      state->fd = -1;
      throw std::runtime_error("Cannot configure virtual controller axes");
    }
  }

  uinput_user_dev device{};
  const char* device_name =
      state->xbox_one ? "Xbox One Controller" : "Native X360 Controller";
  std::strncpy(device.name, device_name, UINPUT_MAX_NAME_SIZE - 1);
  device.id.bustype = BUS_USB;
  device.id.vendor = 0x045e;
  device.id.product = state->xbox_one ? 0x02ff : 0x028e;
  device.id.version = 1;
  for (int i = 0; i < 4; ++i) {
    device.absmin[axes[i]] = -32768;
    device.absmax[axes[i]] = 32767;
  }
  device.absmin[ABS_Z] = device.absmin[ABS_RZ] = 0;
  device.absmax[ABS_Z] = device.absmax[ABS_RZ] = 255;
  device.absmin[ABS_HAT0X] = device.absmin[ABS_HAT0Y] = -1;
  device.absmax[ABS_HAT0X] = device.absmax[ABS_HAT0Y] = 1;
  if (write(state->fd, &device, sizeof(device)) != static_cast<ssize_t>(sizeof(device)) ||
      ioctl(state->fd, UI_DEV_CREATE) < 0) {
    close(state->fd);
    state->fd = -1;
    throw std::runtime_error("Cannot create virtual controller");
  }
}
#endif

void Finalize(napi_env, void* data, void*) {
  auto* state = static_cast<State*>(data);
#ifdef __linux__
  if (state->fd >= 0) {
    ioctl(state->fd, UI_DEV_DESTROY);
    close(state->fd);
  }
#endif
#ifdef _WIN32
  if (state->xbox_one_device) {
    state->xbox_one_api->destroy(state->xbox_one_device);
  }
  if (state->win_device) {
    state->win_api->stop(state->win_device);
    state->win_api->destroy(state->win_device);
  }
  if (state->winuhid) FreeLibrary(state->winuhid);
  if (state->winuhid_devs) FreeLibrary(state->winuhid_devs);
  delete state->win_api;
  delete state->xbox_one_api;
#endif
  delete state;
}

napi_value Connect(napi_env env, napi_callback_info info) {
  State* state;
  if (!GetState(env, info, &state)) return nullptr;
  if (state->connected) return nullptr;
#ifdef __linux__
  try {
    ConfigureLinuxDevice(state);
  } catch (const std::exception& error) {
    Throw(env, error.what());
    return nullptr;
  }
#elif defined(_WIN32)
  try {
    ConfigureWindowsDevice(state);
  } catch (const std::exception& error) {
    if (state->winuhid) {
      FreeLibrary(state->winuhid);
      state->winuhid = nullptr;
    }
    delete state->win_api;
    state->win_api = nullptr;
    delete state->xbox_one_api;
    state->xbox_one_api = nullptr;
    if (state->winuhid_devs) {
      FreeLibrary(state->winuhid_devs);
      state->winuhid_devs = nullptr;
    }
    Throw(env, error.what());
    return nullptr;
  }
#else
  Throw(env, "No virtual controller backend is available on this platform");
  return nullptr;
#endif
  state->connected = true;
  return nullptr;
}

napi_value Disconnect(napi_env env, napi_callback_info info) {
  State* state;
  if (!GetState(env, info, &state)) return nullptr;
#ifdef __linux__
  if (state->fd >= 0) {
    ioctl(state->fd, UI_DEV_DESTROY);
    close(state->fd);
    state->fd = -1;
  }
#endif
#ifdef _WIN32
  if (state->xbox_one_device) {
    state->xbox_one_api->destroy(state->xbox_one_device);
    state->xbox_one_device = nullptr;
  }
  if (state->win_device) {
    state->win_api->stop(state->win_device);
    state->win_api->destroy(state->win_device);
    state->win_device = nullptr;
  }
#endif
  state->connected = false;
  return nullptr;
}

napi_value Update(napi_env env, napi_callback_info info) {
  State* state;
  if (!GetState(env, info, &state)) return nullptr;
  if (!state->connected) {
    Throw(env, "Controller is not connected");
    return nullptr;
  }
#ifdef __linux__
  try {
    const int keys[] = {BTN_START, BTN_SELECT, BTN_THUMBL, BTN_THUMBR,
                        BTN_TL, BTN_TR, BTN_MODE, BTN_A, BTN_B, BTN_X, BTN_Y};
    for (size_t i = 0; i < state->buttons.size(); ++i) Emit(state, EV_KEY, keys[i], state->buttons[i]);
    const int axes[] = {ABS_X, ABS_Y, ABS_RX, ABS_RY, ABS_Z, ABS_RZ, ABS_HAT0X, ABS_HAT0Y};
    for (size_t i = 0; i < state->axes.size(); ++i) Emit(state, EV_ABS, axes[i], AxisValue(i, state->axes[i]));
    Sync(state);
  } catch (const std::exception& error) {
    Throw(env, error.what());
    return nullptr;
  }
#endif
#ifdef _WIN32
  try {
    SubmitWindowsReport(state);
  } catch (const std::exception& error) {
    Throw(env, error.what());
    return nullptr;
  }
#endif
  return nullptr;
}

napi_value SetButton(napi_env env, napi_callback_info info) {
  napi_value argv[2];
  State* state;
  if (!GetState(env, info, &state, 2, argv)) return nullptr;
  uint32_t index;
  bool value;
  if (napi_get_value_uint32(env, argv[0], &index) != napi_ok ||
      napi_get_value_bool(env, argv[1], &value) != napi_ok || index >= state->buttons.size()) {
    Throw(env, "setButton expects a valid button index and boolean value");
    return nullptr;
  }
  state->buttons[index] = value;
  return nullptr;
}

napi_value SetAxis(napi_env env, napi_callback_info info) {
  napi_value argv[2];
  State* state;
  if (!GetState(env, info, &state, 2, argv)) return nullptr;
  uint32_t index;
  double value;
  if (napi_get_value_uint32(env, argv[0], &index) != napi_ok ||
      napi_get_value_double(env, argv[1], &value) != napi_ok || index >= state->axes.size() ||
      !std::isfinite(value)) {
    Throw(env, "setAxis expects a valid axis index and finite number");
    return nullptr;
  }
  state->axes[index] = value;
  return nullptr;
}

napi_value CreateController(napi_env env, napi_callback_info info) {
  napi_value object;
  if (napi_create_object(env, &object) != napi_ok) return nullptr;
  auto* state = new State();
  napi_value argv[1];
  size_t argc = 1;
  napi_value this_arg;
  if (napi_get_cb_info(env, info, &argc, argv, &this_arg, nullptr) == napi_ok &&
      argc == 1) {
    char backend[32]{};
    size_t length = 0;
    if (napi_get_value_string_utf8(env, argv[0], backend, sizeof(backend),
                                   &length) != napi_ok) {
      delete state;
      Throw(env, "Controller backend must be a string");
      return nullptr;
    }
    if (std::string(backend, length) == "xbox-one") {
      state->xbox_one = true;
    } else if (std::string(backend, length) != "generic") {
      delete state;
      Throw(env, "Unknown controller backend");
      return nullptr;
    }
  }
  if (napi_wrap(env, object, state, Finalize, nullptr, nullptr) != napi_ok) {
    delete state;
    Throw(env, "Unable to create controller");
    return nullptr;
  }
  napi_property_descriptor methods[] = {
      {"connect", nullptr, Connect, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"disconnect", nullptr, Disconnect, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"update", nullptr, Update, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"setButton", nullptr, SetButton, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"setAxis", nullptr, SetAxis, nullptr, nullptr, nullptr, napi_default, nullptr}};
  napi_define_properties(env, object, sizeof(methods) / sizeof(methods[0]), methods);
  return object;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptor = {
      "createX360Controller", nullptr, CreateController, nullptr, nullptr, nullptr, napi_default, nullptr};
  napi_define_properties(env, exports, 1, &descriptor);
  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

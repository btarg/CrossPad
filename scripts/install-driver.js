'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

if (process.platform !== 'win32') {
  throw new Error('The WinUHid driver can only be installed on Windows.');
}
if (process.arch !== 'x64') {
  throw new Error(`The WinUHid driver installer supports Windows x64, not ${process.arch}.`);
}

const driverPackage = path.join(__dirname, '..', 'vendor', 'WinUHid', 'package');
const inf = path.join(driverPackage, 'WinUHidDriver.inf');
const catalog = path.join(driverPackage, 'winuhiddriver.cat');
const driver = path.join(driverPackage, 'WinUHidDriver.dll');

for (const file of [inf, catalog, driver]) {
  if (!fs.existsSync(file)) {
    throw new Error(`The packaged WinUHid driver file is missing: ${file}`);
  }
}

function readPackagedDriverVersion() {
  const contents = fs.readFileSync(inf, 'utf8');
  const match = contents.match(/^\s*DriverVer\s*=\s*([^,\r\n]+)\s*,\s*([^\r\n;]+)/im);
  if (!match) {
    throw new Error(`The packaged driver INF does not contain a DriverVer entry: ${inf}`);
  }
  return `${match[1].trim()},${match[2].trim()}`;
}

function queryInstalledDriver() {
  const result = spawnSync('pnputil.exe', [
    '/enum-devices',
    '/instanceid',
    'Root\\WinUHid',
    '/drivers'
  ], { encoding: 'utf8' });

  if (result.error || result.status !== 0) {
    return null;
  }

  const output = `${result.stdout || ''}\n${result.stderr || ''}`;
  const versionMatch = output.match(
    /^\s*Driver\s+(?:version|Versión)\s*:\s*(\d{1,2}[/-]\d{1,2}[/-]\d{2,4})\s+([0-9.]+)\s*$/im
  );
  if (!versionMatch) {
    return null;
  }
  return `${versionMatch[1].replace(/-/g, '/')},${versionMatch[2]}`;
}

function isDeviceReady() {
  try {
    const handle = fs.openSync('\\\\.\\WinUHid', 'r+');
    fs.closeSync(handle);
    return true;
  } catch {
    return false;
  }
}

function runPnpUtil(args) {
  const result = spawnSync('pnputil.exe', args, { stdio: 'inherit' });
  if (result.error) {
    throw result.error;
  }
  if (result.status !== 0) {
    throw new Error(`pnputil.exe failed with exit code ${result.status}.`);
  }
}

const packagedVersion = readPackagedDriverVersion();
const installedVersion = queryInstalledDriver();
if (isDeviceReady() && installedVersion === packagedVersion) {
  console.log(`WinUHid driver ${packagedVersion} is already installed.`);
  process.exit(0);
}

if (isDeviceReady() && installedVersion) {
  console.log(
    `Installed WinUHid driver ${installedVersion} differs from packaged ${packagedVersion}; updating.`
  );
} else {
  console.log('WinUHid driver is not installed or its version could not be verified; installing.');
}

console.log('Creating the root-enumerated WinUHid device...');
const addDevice = spawnSync('pnputil.exe', ['/add-device', 'Root\\WinUHid'], {
  stdio: 'inherit'
});
if (addDevice.error) {
  throw addDevice.error;
}
if (addDevice.status !== 0 && addDevice.status !== 259) {
  throw new Error(`pnputil.exe could not create Root\\WinUHid (exit code ${addDevice.status}).`);
}

console.log(`Installing the WinUHid driver package from ${inf}...`);
runPnpUtil(['/add-driver', inf, '/install']);

let deviceReady = false;
for (let attempt = 0; attempt < 20; attempt += 1) {
  if (isDeviceReady()) {
    deviceReady = true;
    break;
  }
  Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, 250);
}

if (!deviceReady) {
  throw new Error('The driver was installed, but the \\\\.\\WinUHid device interface is unavailable.');
}

console.log('WinUHid driver installation completed successfully.');

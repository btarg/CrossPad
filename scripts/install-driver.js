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
  const bytes = fs.readFileSync(inf);
  let contents;
  if (bytes[0] === 0xff && bytes[1] === 0xfe) {
    contents = bytes.toString('utf16le');
  } else if (bytes[0] === 0xfe && bytes[1] === 0xff) {
    const swapped = Buffer.alloc(bytes.length - 2);
    for (let index = 2; index < bytes.length; index += 2) {
      swapped[index - 2] = bytes[index + 1];
      swapped[index - 1] = bytes[index];
    }
    contents = swapped.toString('utf16le');
  } else {
    contents = bytes.toString('utf8');
  }
  const match = contents.match(/^\s*DriverVer\s*=\s*([^,\r\n]+)\s*,\s*([^\r\n;]+)/im);
  if (!match) {
    throw new Error(`The packaged driver INF does not contain a DriverVer entry: ${inf}`);
  }
  return `${match[1].trim()},${match[2].trim()}`;
}

function queryInstalledDriver() {
  const result = spawnSync('pnputil.exe', [
    '/enum-devices',
    '/deviceid',
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

function findDevcon() {
  const candidates = [];
  const where = spawnSync('where.exe', ['devcon.exe'], { encoding: 'utf8' });
  if (!where.error && where.status === 0) {
    candidates.push(...where.stdout.split(/\r?\n/).map((value) => value.trim()).filter(Boolean));
  }

  const toolsRoot = path.join(process.env.ProgramFiles || 'C:\\Program Files', 'Windows Kits', '10', 'Tools');
  if (fs.existsSync(toolsRoot)) {
    for (const version of fs.readdirSync(toolsRoot)) {
      candidates.push(path.join(toolsRoot, version, 'x64', 'devcon.exe'));
    }
  }

  const legacyRoot = path.join(
    process.env['ProgramFiles(x86)'] || 'C:\\Program Files (x86)',
    'Windows Kits',
    '10',
    'Tools'
  );
  if (fs.existsSync(legacyRoot)) {
    for (const version of fs.readdirSync(legacyRoot)) {
      candidates.push(path.join(legacyRoot, version, 'x64', 'devcon.exe'));
    }
  }

  return candidates.find((candidate) => fs.existsSync(candidate)) || null;
}

function createRootDevice() {
  const devcon = findDevcon();
  if (!devcon) {
    throw new Error(
      'devcon.exe was not found. Install the Windows SDK/WDK tools or create Root\\WinUHid manually before installing the driver.'
    );
  }

  console.log('Removing any existing WinUHid device instance...');
  const remove = spawnSync(devcon, ['remove', '*WinUHid*'], {
    stdio: 'inherit'
  });
  if (remove.error) {
    throw remove.error;
  }
  if (remove.status !== 0) {
    console.warn(`No existing WinUHid device was removed (devcon exit code ${remove.status}).`);
  }

  console.log(`Creating the root-enumerated WinUHid device with ${devcon}...`);
  const result = spawnSync(devcon, ['install', inf, 'Root\\WinUHid'], {
    stdio: 'inherit'
  });
  if (result.error) {
    throw result.error;
  }
  if (result.status !== 0) {
    throw new Error(`devcon could not create Root\\WinUHid (exit code ${result.status}).`);
  }
}

const packagedVersion = readPackagedDriverVersion();
const installedVersion = queryInstalledDriver();
const driverIsCurrent = isDeviceReady() && installedVersion === packagedVersion;

if (process.argv.includes('--check')) {
  if (driverIsCurrent) {
    console.log(`WinUHid driver ${packagedVersion} is already installed.`);
    process.exit(0);
  }
  process.exit(1);
}

if (driverIsCurrent) {
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

createRootDevice();

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

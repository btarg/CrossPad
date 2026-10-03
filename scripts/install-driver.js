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
const elevated = process.argv.includes('--elevated');
const uninstallRequested = process.argv.includes('--uninstall');

if (elevated) {
  process.title = uninstallRequested
    ? 'CrossPad driver uninstaller'
    : 'CrossPad driver installer';
  const operation = uninstallRequested ? 'UNINSTALLING' : 'INSTALLING';
  console.log('');
  console.log('============================================================');
  console.log(`                 CROSSPAD DRIVER ${operation}`);
  console.log('============================================================');
  console.log('This elevated window is modifying the WinUHid device driver.');
  console.log('Do not close this window until the operation has completed.');
  console.log('============================================================');
  console.log('');
}

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

function elevate(mode) {
  const candidates = [
    path.join(__dirname, '..', 'prebuilds', 'win32-x64', 'virtual_x360.node'),
    path.join(__dirname, '..', 'build', 'Release', 'virtual_x360.node')
  ];
  const nativePath = candidates.find((candidate) => fs.existsSync(candidate));
  if (!nativePath) {
    throw new Error(
      'The native addon is required to request UAC elevation. Build the package before running the driver installer directly.'
    );
  }

  const native = require(nativePath);
  const elevateDriver = mode === '--uninstall'
    ? native.uninstallDriver
    : native.installDriver;
  if (typeof elevateDriver !== 'function') {
    throw new Error(
      'The native addon does not include UAC driver installation support. Rebuild the addon with the current package.'
    );
  }

  const status = elevateDriver(process.execPath, __filename, `${mode} --elevated`);
  if (status !== 0) {
    throw new Error(`WinUHid driver operation failed with exit code ${status}.`);
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

function uninstallDriver() {
  const devcon = findDevcon();
  if (!devcon) {
    throw new Error(
      'devcon.exe was not found. Install the Windows SDK/WDK tools before uninstalling WinUHid.'
    );
  }

  const devices = queryWinUhidDevices();
  for (const instanceId of devices) {
    console.log(`Removing WinUHid device ${instanceId}...`);
    const remove = spawnSync(devcon, ['remove', instanceId], {
      stdio: 'inherit'
    });
    if (remove.error) {
      throw remove.error;
    }
    if (remove.status !== 0) {
      throw new Error(`devcon could not remove ${instanceId} (exit code ${remove.status}).`);
    }
  }

  const packages = queryWinUhidPackages();
  if (packages.size === 0) {
    console.log('No WinUHid driver-store packages were found.');
  }

  for (const packageName of packages) {
    console.log(`Removing driver-store package ${packageName}...`);
    const result = spawnSync('pnputil.exe', [
      '/delete-driver',
      packageName,
      '/uninstall'
    ], { stdio: 'inherit' });
    if (result.error) {
      throw result.error;
    }
    if (result.status !== 0) {
      throw new Error(
        `pnputil.exe could not remove ${packageName} (exit code ${result.status}).`
      );
    }
  }

  const remaining = queryWinUhidPackages();
  if (remaining.size > 0) {
    throw new Error(
      `WinUHid driver packages remain installed: ${[...remaining].join(', ')}`
    );
  }
  console.log('WinUHid driver uninstall completed successfully.');
}

function queryWinUhidDevices() {
  const result = spawnSync('pnputil.exe', [
    '/enum-devices',
    '/deviceid',
    'Root\\WinUHid'
  ], { encoding: 'utf8' });
  if (result.error || result.status !== 0) {
    return [];
  }
  const output = `${result.stdout || ''}\n${result.stderr || ''}`;
  return [...output.matchAll(/^\s*Instance ID:\s*(.+?)\s*$/gim)]
    .map((match) => match[1])
    .filter((instanceId) => /^Root\\[^\\]+\\[^\\]+$/i.test(instanceId));
}

function queryWinUhidPackages() {
  const result = spawnSync('pnputil.exe', [
    '/enum-drivers',
    '/class',
    'System',
    '/format',
    'xml'
  ], {
    encoding: 'utf8'
  });
  if (result.error || result.status !== 0) {
    throw new Error('pnputil.exe could not enumerate driver packages in XML format.');
  }

  const output = `${result.stdout || ''}\n${result.stderr || ''}`;
  const packages = new Set();
  for (const match of output.matchAll(
    /<Driver\b[^>]*\bDriverName="(oem\d+\.inf)"[^>]*>([\s\S]*?)<\/Driver>/gi
  )) {
    if (/<OriginalName>\s*winuhiddriver\.inf\s*<\/OriginalName>/i.test(match[2])) {
      packages.add(match[1]);
    }
  }
  return packages;
}

if (uninstallRequested) {
  if (!elevated) {
    elevate('--uninstall');
    process.exit(0);
  }
  uninstallDriver();
  process.exit(0);
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

if (!elevated) {
  elevate('--install');
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

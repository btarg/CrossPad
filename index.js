'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const bundledWinUhid = path.join(__dirname, 'vendor', 'WinUHid', 'bin', 'win32-x64', 'WinUHid.dll');
const bundledWinUhidDevs = path.join(__dirname, 'vendor', 'WinUHid', 'bin', 'win32-x64', 'WinUHidDevs.dll');
if (!process.env.WINUHID_DLL && fs.existsSync(bundledWinUhid)) {
  process.env.WINUHID_DLL = bundledWinUhid;
}
if (!process.env.WINUHID_DEVS_DLL && fs.existsSync(bundledWinUhidDevs)) {
  process.env.WINUHID_DEVS_DLL = bundledWinUhidDevs;
}

const platformPrebuild = path.join(
  __dirname,
  'prebuilds',
  `${process.platform}-${process.arch}`,
  'virtual_x360.node'
);
const localBuild = path.join(__dirname, 'build', 'Release', 'virtual_x360.node');
const native = require(fs.existsSync(platformPrebuild) ? platformPrebuild : localBuild);

const buttonNames = [
  'START', 'BACK', 'LEFT_THUMB', 'RIGHT_THUMB', 'LEFT_SHOULDER',
  'RIGHT_SHOULDER', 'GUIDE', 'A', 'B', 'X', 'Y'
];
const axisNames = [
  'leftX', 'leftY', 'rightX', 'rightY',
  'leftTrigger', 'rightTrigger', 'dpadHorz', 'dpadVert'
];

class X360Controller {
  constructor(backend = 'generic') {
    this._native = native.createX360Controller(backend);
    this.updateMode = 'auto';
    this.button = Object.fromEntries(buttonNames.map((name, index) => [
      name,
      {
        name,
        value: false,
        setValue: (value) => {
          this._native.setButton(index, Boolean(value));
          this.button[name].value = Boolean(value);
          if (this.updateMode === 'auto') this.update();
        }
      }
    ]));
    this.axis = Object.fromEntries(axisNames.map((name, index) => [
      name,
      {
        name,
        value: 0,
        setValue: (value) => {
          if (typeof value !== 'number' || !Number.isFinite(value)) {
            throw new TypeError(`${name} must be a finite number`);
          }
          this._native.setAxis(index, value);
          this.axis[name].value = value;
          if (this.updateMode === 'auto') this.update();
        }
      }
    ]));
  }

  connect() {
    this._native.connect();
    return null;
  }

  disconnect() {
    this._native.disconnect();
    return null;
  }

  update() {
    this._native.update();
  }

  resetInputs() {
    for (const button of Object.values(this.button)) button.setValue(false);
    for (const axis of Object.values(this.axis)) axis.setValue(0);
  }
}

function installDriver() {
  if (process.platform !== 'win32') {
    throw new Error('The WinUHid driver can only be installed on Windows.');
  }
  if (process.arch !== 'x64') {
    throw new Error(`The WinUHid driver installer supports Windows x64, not ${process.arch}.`);
  }

  const installer = path.join(__dirname, 'scripts', 'install-driver.js');
  if (!fs.existsSync(installer)) {
    throw new Error(`The packaged WinUHid driver installer was not found: ${installer}`);
  }

  const result = spawnSync(process.execPath, [installer], { stdio: 'inherit' });

  if (result.error) {
    throw result.error;
  }
  if (result.status !== 0) {
    throw new Error(`WinUHid driver installation failed with exit code ${result.status}.`);
  }
}

module.exports = {
  X360Controller,
  createX360Controller: () => new X360Controller('generic'),
  createXboxOneController: () => new X360Controller('xbox-one'),
  installDriver
};

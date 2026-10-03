'use strict';

const { createXboxOneController, installDriver } = require('..');

if (process.platform === 'win32') {
  console.log('Checking the installed WinUHid driver...');
  try {
    installDriver();
    console.log('WinUHid driver is ready.');
  } catch (error) {
    console.error('WinUHid driver setup failed:', error.message);
    process.exit(1);
  }
}

const controller = createXboxOneController();
const buttons = ['A', 'B', 'X', 'Y'];

controller.updateMode = 'manual';
controller.connect();
console.log('Native virtual Xbox One-style controller connected.');
console.log('Rotating both sticks sinusoidally and pressing A, B, X and Y. Press Ctrl+C to stop.');

let button = 0;
let t = 0;
const timer = setInterval(() => {
  controller.axis.leftX.setValue(Math.sin(t));
  controller.axis.leftY.setValue(Math.cos(t));
  controller.axis.rightX.setValue(-Math.sin(t));
  controller.axis.rightY.setValue(Math.cos(t));
  controller.axis.dpadHorz.setValue(Math.sin(t));
  controller.axis.dpadVert.setValue(Math.cos(t));

  const name = buttons[button];
  controller.button[name].setValue(true);
  controller.update();
  setTimeout(() => {
    controller.button[name].setValue(false);
    controller.update();
  }, 100);
  console.log(`Pressed ${name}`);
  t += 0.1;
  button = (button + 1) % buttons.length;
}, 100);

process.once('SIGINT', () => {
  clearInterval(timer);
  controller.resetInputs();
  controller.disconnect();
  process.exit(0);
});

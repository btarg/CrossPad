'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const prebuild = path.join(
  __dirname,
  '..',
  'prebuilds',
  `${process.platform}-${process.arch}`,
  'virtual_x360.node'
);

if (fs.existsSync(prebuild)) {
  console.log(`Using prebuilt native addon: ${prebuild}`);
  process.exit(0);
}

if (!fs.existsSync(path.join(__dirname, '..', 'binding.gyp'))) {
  throw new Error(
    `No prebuilt CrossPad addon is available for ${process.platform}-${process.arch}.`
  );
}

const result = spawnSync(process.execPath, [
  require.resolve('node-gyp/bin/node-gyp.js'),
  'rebuild'
], { stdio: 'inherit' });

if (result.error) throw result.error;
process.exit(result.status === null ? 1 : result.status);

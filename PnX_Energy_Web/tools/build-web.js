const fs = require('node:fs');
const path = require('node:path');

const root = path.resolve(__dirname, '..');
const files = {
  config: 'miniprogram/lib/config.js',
  protocol: 'miniprogram/lib/protocol.js',
  model: 'miniprogram/lib/model.js',
  demo: 'miniprogram/lib/demo.js',
  controller: 'miniprogram/lib/controller.js',
  'web-ble': 'web/src/web-ble.js',
  main: 'web/src/main.js'
};
const entries = Object.entries(files).map(([name, file]) => {
  const source = fs.readFileSync(path.join(root, file), 'utf8')
    .replace(/require\('\.(?:\/)([a-z-]+)'\)/g, "require('$1')");
  return `${JSON.stringify(name)}:function(require,module,exports){\n${source}\n}`;
});
const output = `(function(){'use strict';\nconst factories={${entries.join(',\n')}};\nconst cache={};\nfunction require(name){if(!factories[name])throw new Error('Missing module '+name);if(!cache[name]){const module={exports:{}};cache[name]=module;factories[name](require,module,module.exports);}return cache[name].exports;}\nrequire('main');\n})();\n`;
fs.writeFileSync(path.join(root, 'web', 'app.js'), output);
console.log('web/app.js built from shared protocol/controller/model.');

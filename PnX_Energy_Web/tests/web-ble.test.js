const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const config = require('../miniprogram/lib/config');

function loadWebBle(navigator) {
  const source = fs.readFileSync(path.join(__dirname, '..', 'web', 'src', 'web-ble.js'), 'utf8');
  const module = { exports: {} };
  vm.runInNewContext(`(function(require,module,exports){${source}\n})`,
    { navigator, setTimeout, Uint8Array, Promise, Error }).call(null,
    () => config, module, module.exports);
  return module.exports.WebBle;
}

test('网页BLE选择设备、20字节分包、通知及主动断开', async () => {
  const writes = [], listeners = {};
  const characteristic = {
    properties: { write: true, notify: true },
    async startNotifications() {}, async stopNotifications() {},
    addEventListener(name, fn) { listeners[name] = fn; },
    removeEventListener(name) { delete listeners[name]; },
    async writeValueWithResponse(chunk) { writes.push(Array.from(chunk)); }
  };
  const device = {
    gatt: {
      connected: false,
      async connect() { this.connected = true; return { getPrimaryService: async () => ({ getCharacteristic: async () => characteristic }) }; },
      disconnect() { this.connected = false; }
    },
    addEventListener(name, fn) { listeners[name] = fn; },
    removeEventListener(name) { delete listeners[name]; }
  };
  let requested = false;
  const navigator = { bluetooth: { requestDevice(options) {
    requested = true;
    assert.equal(options.acceptAllDevices, true);
    assert.deepEqual(Array.from(options.optionalServices), [config.service]);
    return Promise.resolve(device);
  } } };
  const WebBle = loadWebBle(navigator);
  const transport = new WebBle();
  let received = null, disconnected = 0;
  transport.onData = value => { received = Array.from(new Uint8Array(value)); };
  transport.onDisconnect = () => disconnected++;
  const opening = transport.connect();
  assert.equal(requested, true, '浏览器设备选择器必须在点击调用链中同步触发');
  await opening;
  await transport.send(new Uint8Array(45).fill(7).buffer);
  assert.deepEqual(writes.map(x => x.length), [20, 20, 5]);
  const packet = new Uint8Array([1, 2, 3]);
  listeners.characteristicvaluechanged({ target: { value: new DataView(packet.buffer) } });
  assert.deepEqual(received, [1, 2, 3]);
  await transport.disconnect();
  assert.equal(disconnected, 0);
  assert.equal(device.gatt.connected, false);
  await transport.connect();
  assert.equal(device.gatt.connected, true);
  listeners.gattserverdisconnected({ target: {} });
  assert.equal(disconnected, 0, '旧设备的迟到断线事件不应断开新会话');
  await transport.disconnect();
});

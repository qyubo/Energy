const config = require('config');

class WebBle {
  constructor() {
    this.onData = () => {};
    this.onDisconnect = () => {};
    this.device = null;
    this.characteristic = null;
    this.epoch = 0;
    this.intentional = false;
    this.onValue = event => {
      const view = event.target.value;
      this.onData(view.buffer.slice(view.byteOffset, view.byteOffset + view.byteLength));
    };
    this.onGattDisconnect = event => {
      if (event.target !== this.device) return;
      this.characteristic = null;
      if (!this.intentional) { this.epoch++; this.onDisconnect(); }
    };
  }

  async connect() {
    if (!navigator.bluetooth) throw new Error('此浏览器不支持 Web Bluetooth；请用电脑 Chrome/Edge 或安卓 Chrome，并通过 HTTPS/localhost 打开');
    // HC-42 固件可能只广播 FFF0，不广播 FFE0；服务在选择设备后再验证。
    const device = await navigator.bluetooth.requestDevice({
      acceptAllDevices: true,
      optionalServices: [config.service]
    });
    await this.disconnect();
    this.intentional = false;
    this.device = device;
    device.addEventListener('gattserverdisconnected', this.onGattDisconnect);
    try {
      const server = await device.gatt.connect();
      const service = await server.getPrimaryService(config.service);
      const characteristic = await service.getCharacteristic(config.characteristic);
      if (!characteristic.properties.notify && !characteristic.properties.indicate) throw new Error('FFE1 不支持通知');
      if (!characteristic.properties.write && !characteristic.properties.writeWithoutResponse) throw new Error('FFE1 不支持写入');
      this.characteristic = characteristic;
      await characteristic.startNotifications();
      characteristic.addEventListener('characteristicvaluechanged', this.onValue);
    } catch (error) {
      await this.disconnect();
      throw error;
    }
  }

  async send(buffer) {
    if (!this.characteristic || !this.device?.gatt.connected) throw new Error('蓝牙未连接');
    const epoch = this.epoch;
    const data = new Uint8Array(buffer);
    for (let i = 0; i < data.length; i += config.chunkSize) {
      if (epoch !== this.epoch || !this.device?.gatt.connected) throw new Error('发送中蓝牙断开');
      const chunk = data.slice(i, i + config.chunkSize);
      if (this.characteristic.properties.write && this.characteristic.writeValueWithResponse) {
        await this.characteristic.writeValueWithResponse(chunk);
      } else if (this.characteristic.properties.writeWithoutResponse && this.characteristic.writeValueWithoutResponse) {
        await this.characteristic.writeValueWithoutResponse(chunk);
      } else {
        await this.characteristic.writeValue(chunk);
      }
      if (i + config.chunkSize < data.length) await new Promise(resolve => setTimeout(resolve, config.chunkGapMs));
    }
  }

  async disconnect() {
    this.intentional = true;
    this.epoch++;
    if (this.characteristic) {
      this.characteristic.removeEventListener('characteristicvaluechanged', this.onValue);
      try { await this.characteristic.stopNotifications(); } catch (_) {}
    }
    this.characteristic = null;
    if (this.device) {
      this.device.removeEventListener('gattserverdisconnected', this.onGattDisconnect);
      if (this.device.gatt.connected) this.device.gatt.disconnect();
    }
    this.device = null;
  }

  async dispose() { await this.disconnect(); }
}

module.exports = { WebBle };

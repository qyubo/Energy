const test = require('node:test');
const assert = require('node:assert/strict');
const { Controller } = require('../miniprogram/lib/controller');
const { Decoder, encode } = require('../miniprogram/lib/protocol');
const { Model } = require('../miniprogram/lib/model');

class FakeTransport {
  constructor(delay = 1) {
    this.model = new Model(() => 0);
    this.delay = delay;
    this.onData = () => {}; this.onDisconnect = () => {};
    this.decoder = new Decoder(m => {
      const reply = this.model.handle(m);
      if (reply) setTimeout(() => this.onData(encode(reply)), this.delay);
    });
  }
  async connect() { this.connected = true; }
  async send(data) { if (!this.connected) throw new Error('断线'); this.decoder.push(data); }
  async disconnect() { this.connected = false; }
  async dispose() { await this.disconnect(); }
}

test('界面只在主板ACK后推进状态', async () => {
  const transport = new FakeTransport(20);
  const controller = new Controller(transport, () => {}, () => {}, { timeout: 200 });
  await controller.connect('fake');
  const oldRound = controller.state.round;
  const pending = controller.action('START', 'SMALL');
  assert.equal(controller.state.round, oldRound);
  assert.equal(await pending, true);
  assert.equal(controller.state.phase, 'RUNNING');
  assert.equal(controller.state.round, oldRound + 1);
  await controller.dispose();
});

test('无ACK时不假装成功并标记状态未知', async () => {
  const transport = new FakeTransport(1);
  const controller = new Controller(transport, () => {}, () => {}, { timeout: 30 });
  await controller.connect('fake');
  transport.send = async () => {};
  const oldState = { ...controller.state };
  assert.equal(await controller.action('START', 'LARGE'), false);
  assert.deepEqual(controller.state, oldState);
  assert.equal(controller.uncertain, true);
  await controller.dispose();
});

test('断开后可用同一控制器建立新会话并再次连接', async () => {
  const transport = new FakeTransport(1);
  const controller = new Controller(transport, () => {}, () => {}, { timeout: 200 });
  await controller.connect('fake');
  const firstSid = controller.sid;
  assert.equal(await controller.action('START', 'SMALL'), true);
  assert.equal(await controller.disconnect(), true);
  await controller.connect('fake');
  assert.notEqual(controller.sid, firstSid);
  assert.equal(controller.seq, 1);
  assert.equal(controller.ready, true);
  assert.equal(controller.state.phase, 'IDLE');
  await controller.dispose();
});

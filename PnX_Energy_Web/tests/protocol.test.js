const test = require('node:test');
const assert = require('node:assert/strict');
const { encode, Decoder, validState, validReply, validCommand } = require('../miniprogram/lib/protocol');

const bytes = value => new Uint8Array(encode(value));

test('解码器支持BLE拆包与粘包', () => {
  const got = [];
  const decoder = new Decoder(x => got.push(x));
  const a = bytes({ a: 1 });
  const b = bytes({ b: 2 });
  decoder.push(a.slice(0, 3));
  decoder.push(new Uint8Array([...a.slice(3), ...b]));
  assert.deepEqual(got, [{ a: 1 }, { b: 2 }]);
});

test('非法字符、超长帧和坏JSON不会产生消息', () => {
  const got = [], errors = [];
  const decoder = new Decoder(x => got.push(x), e => errors.push(e));
  decoder.push(new Uint8Array([1, 10]));
  decoder.push(new TextEncoder().encode('{bad}\n'));
  decoder.push(new TextEncoder().encode('x'.repeat(1025) + '\n'));
  assert.equal(got.length, 0);
  assert.ok(errors.length >= 3);
});

test('协议校验拒绝不可能状态和坏会话', () => {
  const running = { rev: 3, round: 1, mode: 'SMALL', phase: 'RUNNING', mask: 1, target: 2, flashOn: false, flashes: 0 };
  assert.equal(validState(running), true);
  assert.equal(validState({ ...running, target: 1 }), false);
  assert.equal(validReply({ v: 1, sid: 'session_123', seq: 2, ok: true, state: running }, 'session_123'), true);
  assert.equal(validCommand({ v: 1, sid: 'session_123', seq: 1, cmd: 'HELLO' }), true);
  assert.equal(validCommand({ v: 1, sid: 'x', seq: 1, cmd: 'HELLO' }), false);
});

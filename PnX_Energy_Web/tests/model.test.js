const test = require('node:test');
const assert = require('node:assert/strict');
const { Model, lamps } = require('../miniprogram/lib/model');
const config = require('../miniprogram/lib/config');

function command(seq, cmd, extra = {}) { return { v: 1, sid: 'session_123', seq, cmd, ...extra }; }
function setup(mode = 'SMALL') {
  let time = 1000;
  const samples = [0, 0, 0, 0, 0];
  const model = new Model(() => samples.shift() ?? 0, () => time);
  model.handle(command(1, 'HELLO'));
  model.handle(command(2, 'START', { mode, rev: model.state.rev }));
  return { model, advance: n => { time += n; } };
}

test('小符五次击中不重复目标，完成10次闪烁', () => {
  const { model, advance } = setup('SMALL');
  const targets = [];
  for (let seq = 3; seq <= 7; seq++) {
    targets.push(model.state.target);
    const before = model.state.rev;
    const reply = model.handle(command(seq, 'HIT', { rev: before, target: model.state.target }));
    assert.equal(reply.ok, true);
  }
  assert.deepEqual(targets, [1, 2, 3, 4, 5]);
  assert.equal(model.state.phase, 'FLASHING');
  assert.equal(model.state.mask, 31);
  assert.ok(lamps(model.state).every(x => x.command === 4));
  advance(config.flashHalfMs * config.flashCount * 2);
  assert.equal(model.tick(), true);
  assert.equal(model.state.phase, 'COMPLETE');
  assert.equal(model.state.flashes, 10);
});

test('大符双靶、单靶、五格、最后一击和八环结束灯效', () => {
  const { model, advance } = setup('LARGE');
  assert.ok(model.state.otherTarget && model.state.otherTarget !== model.state.target);
  let seq=3;
  const act=cmd=>model.handle(command(seq++,cmd,{rev:model.state.rev,target:model.state.target}));
  act('HIT');
  assert.equal(model.state.otherTarget,0);
  assert.equal(model.state.progress,0);
  act('MISS');
  assert.equal(model.state.progress,1);
  assert.ok(model.state.otherTarget);
  assert.equal(lamps(model.state).filter(x=>x.command===3).length,3);
  for(let level=1;level<=4;level++) {
    act('HIT');
    assert.equal(model.state.otherTarget,0);
    if(level<4) {
      act('HIT');
      assert.equal(model.state.progress,level+1);
      assert.ok(model.state.otherTarget);
    }
  }
  assert.equal(model.state.progress,5);
  assert.equal(lamps(model.state).find(x=>x.id===model.state.target).command,6);
  act('HIT');
  assert.equal(model.state.phase,'FLASHING');
  assert.ok(lamps(model.state).every(x=>x.command===5));
  advance(config.flashHalfMs*config.flashCount*2);
  model.tick();
  assert.equal(model.state.phase,'COMPLETE');
  assert.equal(model.state.flashes,10);
});

test('大符双靶未中重开；单靶未中继续',()=>{
  const {model}=setup('LARGE');
  const first=model.state.round;
  model.handle(command(3,'MISS',{rev:model.state.rev,target:model.state.target}));
  assert.equal(model.state.round,first+1);
  model.handle(command(4,'HIT',{rev:model.state.rev,target:model.state.target}));
  model.handle(command(5,'MISS',{rev:model.state.rev,target:model.state.target}));
  assert.equal(model.state.round,first+1);
  assert.equal(model.state.progress,1);
});

test('未击中与错误压力板都重开一轮', () => {
  const { model } = setup();
  const firstRound = model.state.round;
  model.handle(command(3, 'MISS', { rev: model.state.rev, target: model.state.target }));
  assert.equal(model.state.round, firstRound + 1);
  assert.equal(model.state.mask, 0);
  const target = model.state.target;
  const wrong = target === 1 ? 2 : 1;
  assert.equal(model.sensor(wrong, model.state.rev), true);
  assert.equal(model.state.mask, 0);
  assert.equal(model.state.round, firstRound + 2);
});

test('重复序号幂等，过期状态和错误目标被拒绝', () => {
  const { model } = setup();
  const hit = command(3, 'HIT', { rev: model.state.rev, target: model.state.target });
  const first = model.handle(hit);
  const duplicate = model.handle(hit);
  assert.deepEqual(duplicate, first);
  assert.equal(model.handle(command(4, 'HIT', { rev: 0, target: model.state.target })).error, 'STALE');
});

test('失去手机心跳后看门狗安全停止', () => {
  const { model, advance } = setup('LARGE');
  advance(config.watchdogMs + 1);
  assert.equal(model.tick(), true);
  assert.equal(model.state.phase, 'IDLE');
  assert.ok(lamps(model.state).every(x => x.command === 0));
});

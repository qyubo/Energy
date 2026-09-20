const { Controller } = require('controller');
const { Demo } = require('demo');
const { WebBle } = require('web-ble');

const el = id => document.getElementById(id);
const refs = Object.fromEntries([
  'connection', 'mode-small', 'mode-large', 'connect', 'disconnect', 'demo', 'notice',
  'screen-home', 'screen-test', 'screen-complete', 'screen-connecting', 'mode-title',
  'target-number', 'target-caption', 'round-number', 'progress', 'hit', 'miss', 'exit',
  'complete-mode', 'complete-round', 'complete-flashes', 'complete-title', 'complete-desc',
  'restart', 'warning', 'source-label', 'test-hint'
].map(id => [id, el(id)]));

let controller = null;
let transport = null;
let screen = 'home';
let source = 'none';
let toastTimer = null;

function notice(text, type = '') {
  refs.notice.textContent = text;
  refs.notice.className = `notice visible ${type}`;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => refs.notice.classList.remove('visible'), 3600);
}

function show(name) {
  screen = name;
  for (const key of ['home', 'test', 'complete', 'connecting']) {
    refs[`screen-${key}`].hidden = key !== name;
  }
}

function status(snapshot) {
  const s = snapshot.state;
  const ready = snapshot.ready && snapshot.connected;
  refs.connection.textContent = snapshot.uncertain ? '设备状态待确认' : ready ? '主板已连接' : snapshot.connected ? '等待主板确认' : '未连接';
  refs.connection.className = `connection ${ready ? 'online' : snapshot.uncertain ? 'warn' : ''}`;
  refs.connect.hidden = snapshot.connected || source === 'demo';
  refs.connect.disabled = snapshot.busy;
  refs.disconnect.hidden = !snapshot.connected;
  refs.demo.hidden = snapshot.connected;
  refs['source-label'].hidden = source !== 'demo';
  refs['mode-small'].disabled = !ready || snapshot.busy || snapshot.uncertain || s.phase !== 'IDLE';
  refs['mode-large'].disabled = !ready || snapshot.busy || snapshot.uncertain || s.phase !== 'IDLE';
  refs.hit.disabled = refs.miss.disabled = !ready || snapshot.busy || snapshot.uncertain || s.phase !== 'RUNNING';
  refs.exit.disabled = refs.restart.disabled = snapshot.busy || !ready;
  refs['round-number'].textContent = String(s.round);
  refs['target-number'].textContent = s.target ? (s.mode === 'LARGE' && s.otherTarget ? `${s.target} · ${s.otherTarget}` : String(s.target)) : '—';
  refs['target-caption'].textContent = s.phase === 'FLASHING' ? `正在闪烁 ${s.flashes}/10` :
    s.phase !== 'RUNNING' ? '等待主板' : s.mode === 'LARGE' ?
    `进度 ${s.progress || 0}/5 格 · 点击击中模拟 ${s.target} 号靶` : `${s.target} 号靶面 · 箭头点亮`;
  refs['test-hint'].textContent = s.mode === 'LARGE'
    ? (s.otherTarget ? '当前有两块亮靶：击中任一块继续，未击中则从头开始。网页“击中”模拟主板指定靶。' : s.progress === 5 ? '最后一靶：击中后全红、八环闪烁十次；未击中则从头开始。' : '当前只有一块亮靶：击中或未击中都会进入下一组双靶。')
    : '“未击中”会由主板清零并重新随机点亮第一组。测试时请留意实物灯效。';
  refs['mode-title'].textContent = s.mode === 'SMALL' ? '小符测试' : '大符测试';
  refs.progress.replaceChildren(...Array.from({ length: 5 }, (_, i) => {
    const step = document.createElement('span');
    const done = s.mode === 'LARGE' ? i < (s.progress || 0) : Boolean(s.mask & (1 << i));
    step.className = `step ${done ? 'done' : ''}`;
    step.textContent = done ? '✓' : String(i + 1);
    step.setAttribute('aria-label', `第${i + 1}${s.mode === 'LARGE' ? '格' : '击'}${done ? '已完成' : '未完成'}`);
    return step;
  }));
  refs['complete-mode'].textContent = s.mode === 'SMALL' ? '小符测试' : '大符测试';
  refs['complete-round'].textContent = String(s.round);
  refs['complete-flashes'].textContent = String(s.flashes);
  refs['complete-title'].textContent = s.mode === 'LARGE' ? '大符测试完成' : '小符五组全部击中';
  refs['complete-desc'].textContent = `主板报告十次闪烁完成，当前保持全红和${s.mode === 'LARGE' ? '八环' : '一环'}。`;
  refs.warning.hidden = !snapshot.error;
  refs.warning.textContent = snapshot.error || '';
  if (s.phase === 'COMPLETE') show('complete');
  else if (s.phase === 'IDLE' && screen === 'test') show('home');
  else if (screen === 'connecting' && !snapshot.busy && !ready) show('home');
  else if (screen === 'test' && !snapshot.connected) show('home');
}

function createController(kind) {
  if (controller) controller.dispose().catch(() => {});
  source = kind;
  transport = kind === 'demo' ? new Demo() : new WebBle();
  controller = new Controller(transport, status, text => notice(text, text.includes('失败') ? 'error' : 'success'));
  status({ state: controller.state, ready: false, connected: false, busy: false, uncertain: false, error: '' });
}

async function connect(kind) {
  if (kind === 'real' && !navigator.bluetooth) {
    notice('浏览器不支持 Web Bluetooth；请用电脑 Chrome/Edge 或安卓 Chrome 打开 HTTPS 页面', 'error');
    return;
  }
  if (!controller || source !== kind) createController(kind);
  show('connecting');
  await controller.connect(kind);
  show('home');
  if (controller.ready && kind === 'demo') notice('演示模式：不控制真实硬件');
}

refs.connect.addEventListener('click', () => connect('real'));
refs.demo.addEventListener('click', () => connect('demo'));
refs.disconnect.addEventListener('click', async () => {
  const stopped = await controller.disconnect();
  if (source === 'demo') createController('real');
  show('home');
  notice(stopped ? '已断开并确认停止' : '已断开，但未收到停止确认，请检查主板', stopped ? 'success' : 'error');
});
refs['mode-small'].addEventListener('click', async () => {
  if (await controller.action('START', 'SMALL')) show('test');
});
refs['mode-large'].addEventListener('click', async () => {
  if (await controller.action('START', 'LARGE')) show('test');
});
refs.hit.addEventListener('click', () => controller.action('HIT'));
refs.miss.addEventListener('click', () => controller.action('MISS'));
refs.exit.addEventListener('click', async () => {
  if (await controller.action('STOP')) show('home');
});
refs.restart.addEventListener('click', async () => {
  if (await controller.action('STOP')) show('home');
});

window.addEventListener('pagehide', () => {
  if (controller) controller.background();
});
document.addEventListener('visibilitychange', () => {
  // Switching tabs or apps must not end an active test.
  if (document.visibilityState === 'visible' && controller) controller.foreground();
});

createController('real');
if (!window.isSecureContext) notice('蓝牙功能需要 HTTPS 或本机 localhost 地址', 'error');

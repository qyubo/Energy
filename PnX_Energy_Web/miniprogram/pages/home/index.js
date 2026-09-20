const { Ble, message } = require('../../lib/ble');
const { Demo } = require('../../lib/demo');
const { Controller } = require('../../lib/controller');

const initialView = () => ({
  screen: 'home', source: 'none', devices: [], scanning: false,
  connected: false, ready: false, busy: false, uncertain: false, error: '',
  state: { mode: 'NONE', phase: 'IDLE', mask: 0, target: 0, flashes: 0, round: 0 },
  progress: [0, 1, 2, 3, 4].map(i => ({ index: i + 1, done: false })),
  modeName: '', targetText: '等待主板', targetNumber: '—', progressUnit: '击',
  completeTitle: '测试完成', statusText: '蓝牙未连接',
  connectionClass: 'offline', canChooseMode: false, canChooseLarge: false, canOperate: false
});

Page({
  data: initialView(),

  onLoad() { this.createRealController(); },
  onUnload() {
    if (getApp().controller === this.controller) getApp().controller = null;
    if (this.controller) this.controller.dispose().catch(() => {});
  },

  createController(transport, source) {
    if (this.controller) this.controller.dispose().catch(() => {});
    this.transport = transport;
    this.controller = new Controller(
      transport,
      snapshot => this.applySnapshot(snapshot),
      text => wx.showToast({ title: text, icon: 'none', duration: 1600 })
    );
    getApp().controller = this.controller;
    this.setData({ source });
  },

  createRealController() {
    const transport = new Ble(wx);
    transport.onDevices = devices => this.setData({ devices });
    this.createController(transport, 'real');
  },

  applySnapshot(snapshot) {
    const s = snapshot.state;
    const done = Array.from({ length: 5 }, (_, i) => ({
      index: i + 1, done: s.mode === 'LARGE' ? i < (s.progress || 0) : Boolean(s.mask & (1 << i))
    }));
    const ready = snapshot.ready && snapshot.connected;
    let statusText = ready ? '主板已连接并确认' : snapshot.connected ? '正在确认主板…' : '蓝牙未连接';
    if (snapshot.uncertain) statusText = '设备状态待确认';
    let screen = this.data.screen;
    if (s.phase === 'COMPLETE') screen = 'complete';
    this.setData({
      connected: snapshot.connected, ready: snapshot.ready, busy: snapshot.busy,
      uncertain: snapshot.uncertain, error: snapshot.error, state: s, progress: done,
      statusText, connectionClass: ready ? 'online' : snapshot.connected ? 'waiting' : 'offline',
      canChooseMode: ready && !snapshot.busy && s.phase === 'IDLE' && !snapshot.uncertain,
      canChooseLarge: this.data.source !== 'demo' && ready && !snapshot.busy && s.phase === 'IDLE' && !snapshot.uncertain,
      canOperate: ready && !snapshot.busy && s.phase === 'RUNNING' && !snapshot.uncertain,
      modeName: s.mode === 'SMALL' ? '小符测试' : s.mode === 'LARGE' ? '大符测试' : '',
      targetNumber: s.target ? (s.mode === 'LARGE' && s.otherTarget ? `${s.target} · ${s.otherTarget}` : String(s.target)) : '—',
      targetText: s.phase === 'FLASHING' ? `完成闪烁 ${s.flashes}/10` :
        s.mode === 'LARGE' && s.target ? `进度 ${s.progress || 0}/5 格；手机模拟击中 ${s.target} 号靶` :
        s.target ? `当前目标：${s.target} 号靶` : '等待主板',
      progressUnit: s.mode === 'LARGE' ? '格' : '击',
      completeTitle: s.mode === 'LARGE' ? '大符测试完成' : '小符五组全部击中',
      screen
    });
  },

  async openDevices() {
    if (this.data.source === 'demo') await this.leaveDemo();
    this.setData({ screen: 'devices', scanning: true, devices: [], error: '' });
    try { await this.transport.scan(); }
    catch (e) {
      this.setData({ scanning: false, error: message(e) });
      wx.showToast({ title: message(e), icon: 'none' });
    }
  },

  async refreshDevices() {
    this.setData({ scanning: true, devices: [], error: '' });
    try { await this.transport.scan(); }
    catch (e) { this.setData({ error: message(e) }); }
  },

  closeDevices() { this.transport.stopScan().catch(() => {}); this.setData({ screen: 'home', scanning: false }); },

  async selectDevice(e) {
    const id = e.currentTarget.dataset.id;
    this.setData({ scanning: false });
    await this.controller.connect(id);
    this.setData({ screen: 'home' });
  },

  async disconnectBluetooth() {
    const stopped = await this.controller.disconnect();
    this.setData({ screen: 'home' });
    wx.showToast({ title: stopped ? '已安全断开' : '已断开，停止未确认', icon: 'none' });
  },

  async startDemo() {
    this.createController(new Demo(), 'demo');
    await this.controller.connect('demo');
    wx.showToast({ title: '演示模式不控制硬件', icon: 'none', duration: 2200 });
  },

  async leaveDemo() {
    if (this.controller) await this.controller.dispose();
    this.createRealController();
    this.setData({ screen: 'home', ...initialView(), source: 'real' });
  },

  async chooseMode(e) {
    const mode = e.currentTarget.dataset.mode;
    if (!this.data.canChooseMode) return;
    if (await this.controller.action('START', mode)) this.setData({ screen: 'test' });
  },

  async judge(e) {
    if (!this.data.canOperate) return;
    await this.controller.action(e.currentTarget.dataset.result);
  },

  async exitTest() {
    if (await this.controller.action('STOP')) this.setData({ screen: 'home' });
  },

  async restart() {
    if (await this.controller.action('STOP')) this.setData({ screen: 'home' });
  }
});

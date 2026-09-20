(function(){'use strict';
const factories={"config":function(require,module,exports){
module.exports = {
  service: '0000ffe0-0000-1000-8000-00805f9b34fb',
  characteristic: '0000ffe1-0000-1000-8000-00805f9b34fb',
  chunkSize: 20, chunkGapMs: 25, ackTimeoutMs: 4000,
  heartbeatMs: 1500, watchdogMs: 6000, scanMs: 10000,
  flashHalfMs: 250, flashCount: 10
};

},
"protocol":function(require,module,exports){
// 协议v1：ASCII JSON + LF。接收按字节重组，不能按一次notify解析。
const MAX_LINE = 1024;
const commands = ['HELLO', 'STATUS', 'START', 'HIT', 'MISS', 'STOP'];
function encode(value) {
  const text = JSON.stringify(value);
  if (text.length > MAX_LINE || /[^\x20-\x7e]/.test(text)) throw new Error('协议只接受有界ASCII消息');
  const bytes = new Uint8Array(text.length + 1);
  for (let i=0;i<text.length;i++) bytes[i]=text.charCodeAt(i);
  bytes[text.length]=10;
  return bytes.buffer;
}
class Decoder {
  constructor(onMessage, onError = () => {}) { this.onMessage=onMessage; this.onError=onError; this.reset(); }
  reset() { this.text=''; this.dropping=false; }
  push(buffer) {
    const bytes = buffer instanceof Uint8Array ? buffer : new Uint8Array(buffer);
    for (const byte of bytes) {
      if (byte===10) {
        const line=this.text; const dropped=this.dropping; this.reset();
        if (!dropped && line) {
          try { this.onMessage(JSON.parse(line)); } catch (e) { this.onError(e); }
        }
      } else if (!this.dropping && byte!==13) {
        if (byte<32 || byte>126 || this.text.length>=MAX_LINE) {
          this.dropping=true; this.text=''; this.onError(new Error('非法字符或帧过长'));
        } else this.text+=String.fromCharCode(byte);
      }
    }
  }
}
const integer=(n,min,max)=>Number.isInteger(n)&&n>=min&&n<=max;
function validState(s) {
  if (!s || !integer(s.rev,0,2147483647) || !integer(s.round,0,2147483647) ||
      !integer(s.mask,0,31) || !integer(s.target,0,5) ||
      !['NONE','SMALL','LARGE'].includes(s.mode) ||
      !['IDLE','RUNNING','FLASHING','COMPLETE'].includes(s.phase) ||
      typeof s.flashOn!=='boolean' || !integer(s.flashes,0,10)) return false;
  if (s.phase==='IDLE') return s.mode==='NONE' && s.mask===0 && s.target===0 && s.flashes===0;
  if(s.mode==='NONE') return false;
  if(s.phase==='RUNNING') return s.target>0 && s.mask<31 && !(s.mask & (1<<(s.target-1))) && s.flashes===0;
  return s.mask===31 && s.target===0 && (s.phase!=='COMPLETE' || s.flashes===10);
}
function validReply(m,sid) {
  return m && m.v===1 && m.sid===sid && integer(m.seq,0,2147483647) &&
    typeof m.ok==='boolean' && validState(m.state) &&
    (m.ok || typeof m.error==='string');
}
function validCommand(m) {
  return m && m.v===1 && /^[a-zA-Z0-9_-]{8,48}$/.test(m.sid) &&
    integer(m.seq,1,2147483647) && commands.includes(m.cmd);
}
module.exports={encode,Decoder,validState,validReply,validCommand};


},
"model":function(require,module,exports){
// 可执行主板参考模型。真实设备游戏逻辑必须在C板实现，不在手机推进。
const {validCommand}=require('protocol');
const config=require('config');
function idle(rev=0,round=0) {
  return {rev,round,mode:'NONE',phase:'IDLE',mask:0,target:0,otherTarget:0,progress:0,flashOn:false,flashes:0};
}
class Model {
  constructor(random=Math.random, now=Date.now) {
    this.random=random; this.now=now; this.sid=''; this.lastSeq=0;
    this.lastRequest=''; this.lastReply=null; this.state=idle(); this.lastSeen=0; this.flashAt=0;
  }
  stop() { this.state=idle(this.state.rev+1,this.state.round); }
  choose() {
    const ids=[1,2,3,4,5].filter(id=>!(this.state.mask&(1<<(id-1))));
    this.state.target=ids[Math.min(ids.length-1,Math.floor(Math.max(0,this.random())*ids.length))];
  }
  chooseOther(excluded) {
    const ids=[1,2,3,4,5].filter(id=>id!==excluded);
    return ids[Math.min(ids.length-1,Math.floor(Math.max(0,this.random())*ids.length))];
  }
  bigPair(restart=false) {
    if(restart) this.state.round++;
    this.state.progress=0;
    this.state.mask=0;
    this.state.target=this.chooseOther(0);
    this.state.otherTarget=this.chooseOther(this.state.target);
    this.bigPhase='PAIR';
    this.state.rev++;
  }
  start(mode) {
    this.state={...idle(this.state.rev+1,this.state.round+1),mode,phase:'RUNNING'};
    if(mode==='LARGE') this.bigPair(); else this.choose();
  }
  hit() {
    if(this.state.mode==='LARGE') {
      if(this.bigPhase==='PAIR') {
        this.state.target=this.state.otherTarget;
        this.state.otherTarget=0;
        if(this.state.progress===4) {this.state.progress=5;this.bigPhase='FINAL';}
        else this.bigPhase='SINGLE';
      } else if(this.bigPhase==='SINGLE') {
        this.state.progress++;
        this.state.otherTarget=this.chooseOther(this.state.target);
        this.bigPhase='PAIR';
      } else if(this.bigPhase==='FINAL') {
        this.state.mask=31;this.state.target=0;this.state.otherTarget=0;
        this.state.phase='FLASHING';this.state.flashOn=true;this.flashAt=this.now();
        this.bigPhase='FLASH';
      }
      this.state.rev++;
      return;
    }
    this.state.mask|=1<<(this.state.target-1); this.state.rev++;
    if(this.state.mask===31) {
      this.state.phase='FLASHING'; this.state.target=0; this.state.flashOn=true;
      this.flashAt=this.now();
    } else this.choose();
  }
  sensor(node, expectedRev) {
    if(this.state.phase!=='RUNNING' || expectedRev!==this.state.rev || !Number.isInteger(node) || node<1 || node>5) return false;
    if(node===this.state.target) this.hit(); else this.start(this.state.mode);
    return true;
  }
  reply(seq,ok=true,error) {
    const r={v:1,sid:this.sid,seq,ok,state:{...this.state}};
    if(!ok) r.error=error;
    return r;
  }
  handle(m) {
    if(!validCommand(m)) return null;
    if(m.cmd==='HELLO' && m.sid!==this.sid) {
      // 新会话取得控制权时先回到IDLE；旧会话命令无法进入本会话。
      this.sid=m.sid; this.lastSeq=0; this.lastReply=null; this.stop();
    }
    if(m.sid!==this.sid) return {...this.reply(m.seq,false,'SESSION'),sid:m.sid};
    const signature=JSON.stringify(m);
    if(m.seq===this.lastSeq) {
      if(signature!==this.lastRequest) return this.reply(m.seq,false,'SEQ_CONFLICT');
      this.lastSeen=this.now();
      return JSON.parse(JSON.stringify(this.lastReply));
    }
    if(m.seq<this.lastSeq) return this.reply(m.seq,false,'OLD_SEQ');
    let error='';
    if(['START','HIT','MISS'].includes(m.cmd) && m.rev!==this.state.rev) error='STALE';
    else if(m.cmd==='START') {
      if(!['SMALL','LARGE'].includes(m.mode)) error='MODE';
      else if(this.state.phase!=='IDLE') error='BUSY';
      else this.start(m.mode);
    } else if(m.cmd==='HIT' || m.cmd==='MISS') {
      if(this.state.phase!=='RUNNING') error='PHASE';
      else if(m.target!==this.state.target) error='STALE';
      else if(m.cmd==='HIT') this.hit();
      else if(this.state.mode==='LARGE' && this.bigPhase==='SINGLE') this.hit();
      else if(this.state.mode==='LARGE') this.bigPair(true);
      else this.start(this.state.mode);
    } else if(m.cmd==='STOP') this.stop();
    this.lastSeen=this.now(); this.lastSeq=m.seq; this.lastRequest=signature;
    this.lastReply=this.reply(m.seq,!error,error);
    return JSON.parse(JSON.stringify(this.lastReply));
  }
  tick() {
    if(this.state.phase!=='IDLE' && this.now()-this.lastSeen>config.watchdogMs) { this.stop(); return true; }
    let changed=false;
    while(this.state.phase==='FLASHING' && this.now()-this.flashAt>=config.flashHalfMs) {
      this.flashAt+=config.flashHalfMs;
      if(this.state.flashOn) this.state.flashOn=false;
      else {
        this.state.flashes++; this.state.flashOn=true;
        if(this.state.flashes===config.flashCount) this.state.phase='COMPLETE';
      }
      this.state.rev++; changed=true;
    }
    return changed;
  }
}
function lamps(s) {
  return [1,2,3,4,5].map(id=>{
    let command=0;
    if(s.phase==='FLASHING' || s.phase==='COMPLETE') command=s.flashOn?(s.mode==='LARGE'?5:4):0;
    else if(s.phase==='RUNNING') {
      if(s.mode==='LARGE') {
        if(id===s.target) command=s.progress===5?6:1;
        else if(id===s.otherTarget) command=1;
        else if(s.progress>0) command=3;
      } else if(id===s.target) command=1;
      else if(s.mask&(1<<(id-1))) command=2;
    }
    return {id,command,level:command===3?s.progress:0};
  });
}
module.exports={Model,lamps,idle};

},
"demo":function(require,module,exports){
const {Model}=require('model');
const {Decoder,encode}=require('protocol');
class Demo {
  constructor() {
    this.model=new Model(); this.onData=()=>{};this.onDisconnect=()=>{};this.connected=false;
    this.decoder=new Decoder(m=>{
      const reply=this.model.handle(m);
      if(reply) setTimeout(()=>{if(this.connected)this.onData(encode(reply));},35);
    });
  }
  async connect() {
    this.connected=true;
    this.timer=setInterval(()=>{
      if(this.model.tick()) this.onData(encode(this.model.reply(0)));
    },50);
  }
  async send(bytes) {
    if(!this.connected) throw new Error('演示连接已断开');
    this.decoder.push(bytes);
  }
  async disconnect() { this.connected=false;clearInterval(this.timer);this.model.stop(); }
  async dispose() {await this.disconnect();}
}
module.exports={Demo};


},
"controller":function(require,module,exports){
const {encode,Decoder,validReply}=require('protocol');
const {idle}=require('model');
const config=require('config');
class Controller {
  constructor(transport,onChange=()=>{},onNotice=()=>{},options={}) {
    this.transport=transport; this.onChange=onChange; this.onNotice=onNotice;
    this.timeout=options.timeout||config.ackTimeoutMs;
    this.sid='wx_'+Date.now().toString(36)+'_'+Math.random().toString(36).slice(2,10);
    this.seq=0; this.state=idle();this.ready=false;this.connected=false;this.busy=false;
    this.pending=null;this.hidden=false;this.error='';this.failures=0;this.uncertain=false;
    this.decoder=new Decoder(m=>this.receive(m),()=>{this.error='收到非法协议数据，等待有效主板状态';this.emit();});
    transport.onData=data=>this.decoder.push(data);
    transport.onDisconnect=()=>this.lost();
  }
  emit(){this.onChange({state:{...this.state},ready:this.ready,connected:this.connected,
    busy:this.busy,error:this.error,uncertain:this.uncertain});}
  async connect(id) {
    clearInterval(this.timer);
    this.timer=null;
    this.decoder.reset();
    this.sid='wx_'+Date.now().toString(36)+'_'+Math.random().toString(36).slice(2,10);
    this.seq=0;this.state=idle();this.ready=false;this.connected=false;
    this.failures=0;this.uncertain=false;this.error='';this.hidden=false;
    this.busy=true;this.emit();
    try {
      await this.transport.connect(id);this.connected=true;this.emit();
      await this.request('HELLO');
      this.ready=true;this.error='';this.onNotice('连接成功，主板已就绪');
      this.timer=setInterval(()=>this.poll(),config.heartbeatMs);
    } catch(e) {
      this.ready=false;this.error='连接失败：'+(e.message||'未知错误');
      this.connected=false;await this.transport.disconnect();
      this.onNotice(this.error);
    } finally {this.busy=false;this.emit();}
  }
  receive(m) {
    if(!validReply(m,this.sid)) return;
    const match=this.pending && m.seq===this.pending.seq;
    if(m.seq!==0 && !match) return; // 丢弃已超时、其他命令的迟到回复。
    if(m.state.rev>=this.state.rev) this.state={...m.state};
    if(match) {
      const p=this.pending;this.pending=null;clearTimeout(p.timer);
      if(m.ok) p.resolve(m);else p.reject(new Error('主板拒绝：'+m.error));
    }
    this.emit();
  }
  request(cmd,args={}) {
    if(!this.connected) return Promise.reject(new Error('蓝牙未连接'));
    if(this.pending) return Promise.reject(new Error('上一条命令尚未完成'));
    const packet={v:1,sid:this.sid,seq:++this.seq,cmd,...args};
    return new Promise((resolve,reject)=>{
      const p={seq:packet.seq,resolve,reject};
      p.timer=setTimeout(()=>{
        if(this.pending!==p)return;
        this.pending=null;this.uncertain=true;
        reject(new Error('未收到主板确认；执行结果未知，请同步状态'));
      },this.timeout);
      this.pending=p;
      this.transport.send(encode(packet)).catch(e=>{
        if(this.pending!==p)return;
        clearTimeout(p.timer);this.pending=null;this.uncertain=true;reject(e);
      });
    });
  }
  async action(cmd,mode) {
    if(!this.ready || this.busy || this.pending || (this.uncertain && cmd!=='STOP')) return false;
    this.busy=true;this.error='';this.emit();
    const args=cmd==='START'?{mode,rev:this.state.rev}:
      ['HIT','MISS'].includes(cmd)?{rev:this.state.rev,target:this.state.target}:{};
    try {
      await this.request(cmd,args);this.uncertain=false;
      this.onNotice('命令发送成功');return true;
    } catch(e) {
      this.error=e.message||'发送失败';this.onNotice('命令发送失败');
      return false;
    } finally {this.busy=false;this.emit();}
  }
  async poll() {
    if(!this.ready || !this.connected || this.pending || this.busy || this.hidden) return;
    try {
      await this.request('STATUS');this.failures=0;this.uncertain=false;this.error='';
    } catch(e) {
      this.failures++;this.error=e.message;this.uncertain=true;
      if(this.failures>=2) {await this.transport.disconnect();this.lost();return;}
    }
    this.emit();
  }
  lost() {
    clearInterval(this.timer);
    if(this.pending){clearTimeout(this.pending.timer);this.pending.reject(new Error('连接中断，执行结果未知'));this.pending=null;}
    this.connected=false;this.ready=false;this.uncertain=true;
    this.error='连接已断开，设备状态未知；请重连确认停止';this.emit();
  }
  async background() {
    this.hidden=true;
    if(this.ready && !this.pending && !this.busy) {
      try {await this.request('STOP');} catch(e){this.error='后台停止未获确认，依赖主板看门狗';}
    }
    if(this.connected) await this.transport.disconnect();
    if(this.ready || this.connected)this.lost();
  }
  foreground(){this.hidden=false;this.emit();}
  async disconnect() {
    let stopped=this.state.phase==='IDLE' && !this.uncertain;
    if(this.ready && !this.pending) {
      try {await this.request('STOP');stopped=true;}catch(e){stopped=false;}
    }
    clearInterval(this.timer);await this.transport.disconnect();
    if(this.pending){clearTimeout(this.pending.timer);this.pending.reject(new Error('连接已取消'));this.pending=null;}
    this.ready=false;this.connected=false;
    this.uncertain=!stopped;
    this.error=stopped?'':'未确认停止，请检查主板或重新连接';
    this.emit();return stopped;
  }
  async dispose(){clearInterval(this.timer);await this.disconnect();await this.transport.dispose();}
}
module.exports={Controller};

},
"web-ble":function(require,module,exports){
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

},
"main":function(require,module,exports){
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

}};
const cache={};
function require(name){if(!factories[name])throw new Error('Missing module '+name);if(!cache[name]){const module={exports:{}};cache[name]=module;factories[name](require,module,module.exports);}return cache[name].exports;}
require('main');
})();

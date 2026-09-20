const {encode,Decoder,validReply}=require('./protocol');
const {idle}=require('./model');
const config=require('./config');
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

// 可执行主板参考模型。真实设备游戏逻辑必须在C板实现，不在手机推进。
const {validCommand}=require('./protocol');
const config=require('./config');
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

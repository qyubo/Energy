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


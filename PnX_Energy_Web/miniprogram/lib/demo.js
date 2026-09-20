const {Model}=require('./model');
const {Decoder,encode}=require('./protocol');
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


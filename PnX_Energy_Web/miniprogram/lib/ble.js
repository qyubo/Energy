const config=require('./config');
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
function call(api,name,args={}) {
  return new Promise((resolve,reject)=>{
    if(typeof api[name]!=='function') return reject(new Error('当前环境不支持 '+name+'，请用手机真机'));
    let settled=false;
    const timer=setTimeout(()=>{ if(!settled){settled=true;reject(new Error(name+' 超时'));}},12000);
    api[name]({...args,success:r=>{if(!settled){settled=true;clearTimeout(timer);resolve(r);}},
      fail:e=>{if(!settled){settled=true;clearTimeout(timer);reject(e);}}});
  });
}
function message(e) {
  const map={10000:'蓝牙尚未初始化',10001:'请开启蓝牙并允许微信使用附近设备/蓝牙权限',
    10002:'未找到设备，请靠近并刷新',10003:'连接失败，请确认模块未被其他手机或电脑占用',
    10004:'未找到FFE0服务',10005:'未找到FFE1特征值',10006:'蓝牙连接已断开',
    10007:'特征值不支持收发',10008:'系统蓝牙错误，请断开后重连',10009:'系统不支持BLE',
    10012:'连接超时，请检查模块供电'};
  return map[e.errCode] || e.message || e.errMsg || '蓝牙操作失败';
}
class Ble {
  constructor(api) {
    this.api=api; this.id=''; this.service=''; this.characteristic=''; this.epoch=0;
    this.onData=()=>{}; this.onDisconnect=()=>{}; this.onDevices=()=>{}; this.devices=new Map();
    this.value=e=>{if(e.deviceId===this.id && e.serviceId.toUpperCase()===this.service.toUpperCase() &&
      e.characteristicId.toUpperCase()===this.characteristic.toUpperCase()) this.onData(e.value);};
    this.connection=e=>{
      if(e.deviceId===this.id && !e.connected){this.id='';this.epoch++;this.onDisconnect();}
    };
    this.found=e=>{
      (e.devices||[]).forEach(d=>{
        if(d.deviceId) this.devices.set(d.deviceId,{deviceId:d.deviceId,name:d.name||d.localName||'未命名 BLE 设备',
          RSSI:typeof d.RSSI==='number'?d.RSSI:-100});
      });
      this.onDevices([...this.devices.values()].sort((a,b)=>b.RSSI-a.RSSI));
    };
    api.onBLECharacteristicValueChange(this.value);
    api.onBLEConnectionStateChange(this.connection);
    api.onBluetoothDeviceFound(this.found);
  }
  async scan() {
    await call(this.api,'openBluetoothAdapter',{mode:'central'});
    await this.stopScan();
    this.devices.clear(); this.onDevices([]);
    await call(this.api,'startBluetoothDevicesDiscovery',{allowDuplicatesKey:false});
    this.scanTimer=setTimeout(()=>this.stopScan().catch(()=>{}),config.scanMs);
  }
  async stopScan() {
    clearTimeout(this.scanTimer);
    await call(this.api,'stopBluetoothDevicesDiscovery').catch(()=>{});
  }
  async connect(id) {
    await this.stopScan();
    await this.disconnect();
    const epoch=++this.epoch;
    // 设置id以便连接期间也能检测断线；通知监听早于notify开启。
    this.id=id;
    try {
      await call(this.api,'createBLEConnection',{deviceId:id,timeout:10000});
      const result=await call(this.api,'getBLEDeviceServices',{deviceId:id});
      const service=result.services.find(s=>s.uuid.toUpperCase()===config.service);
      if(!service) throw new Error('模块未提供FFE0服务，请检查型号/固件');
      const found=await call(this.api,'getBLEDeviceCharacteristics',{deviceId:id,serviceId:service.uuid});
      const ch=found.characteristics.find(c=>c.uuid.toUpperCase()===config.characteristic);
      if(!ch || !(ch.properties.write||ch.properties.writeNoResponse) ||
          !(ch.properties.notify||ch.properties.indicate)) throw new Error('FFE1不支持所需的写入与通知');
      this.service=service.uuid; this.characteristic=ch.uuid;
      this.writeType=ch.properties.write?'write':'writeNoResponse';
      await call(this.api,'notifyBLECharacteristicValueChange',{deviceId:id,serviceId:this.service,
        characteristicId:this.characteristic,state:true});
      if(epoch!==this.epoch || this.id!==id) throw new Error('连接已取消');
    } catch(e) {
      await call(this.api,'closeBLEConnection',{deviceId:id}).catch(()=>{});
      if(this.id===id) this.id='';
      throw new Error(message(e));
    }
  }
  async send(buffer) {
    const epoch=this.epoch, id=this.id;
    if(!id) throw new Error('未连接蓝牙');
    const bytes=new Uint8Array(buffer);
    for(let i=0;i<bytes.length;i+=config.chunkSize) {
      if(epoch!==this.epoch || this.id!==id) throw new Error('发送中连接已断开');
      await call(this.api,'writeBLECharacteristicValue',{deviceId:id,serviceId:this.service,
        characteristicId:this.characteristic,writeType:this.writeType,
        value:bytes.slice(i,i+config.chunkSize).buffer});
      if(i+config.chunkSize<bytes.length) await sleep(config.chunkGapMs);
    }
  }
  async disconnect() {
    this.epoch++; const id=this.id; this.id='';
    if(id) await call(this.api,'closeBLEConnection',{deviceId:id}).catch(()=>{});
  }
  async dispose() {
    await this.stopScan(); await this.disconnect();
    this.api.offBLECharacteristicValueChange(this.value);
    this.api.offBLEConnectionStateChange(this.connection);
    this.api.offBluetoothDeviceFound(this.found);
  }
}
module.exports={Ble,message};


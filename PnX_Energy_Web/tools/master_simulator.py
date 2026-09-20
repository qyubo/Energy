"""HC-T + HC-42D 串口主板模拟器。只用于联调，不代替真实固件。"""
import argparse, json, random, sys, time

try:
    import serial
except ImportError:
    print("缺少 pyserial：py -m pip install -r tools/requirements.txt", file=sys.stderr)
    raise SystemExit(2)

class Model:
    def __init__(self):
        self.sid = ""; self.last_seq = 0; self.last_request = ""; self.last_reply = None
        self.rev = 0; self.round = 0; self.mode = "NONE"; self.phase = "IDLE"
        self.mask = 0; self.target = 0; self.flash_on = False; self.flashes = 0
        self.last_seen = time.monotonic(); self.flash_at = self.last_seen

    def state(self):
        return {"rev":self.rev,"round":self.round,"mode":self.mode,"phase":self.phase,
                "mask":self.mask,"target":self.target,"flashOn":self.flash_on,"flashes":self.flashes}

    def stop(self):
        self.rev += 1; self.mode = "NONE"; self.phase = "IDLE"; self.mask = 0
        self.target = 0; self.flash_on = False; self.flashes = 0

    def choose(self):
        choices = [n for n in range(1, 6) if not self.mask & (1 << (n - 1))]
        self.target = random.choice(choices)

    def start(self, mode):
        self.rev += 1; self.round += 1; self.mode = mode; self.phase = "RUNNING"
        self.mask = 0; self.flash_on = False; self.flashes = 0; self.choose()

    def hit(self):
        self.mask |= 1 << (self.target - 1); self.rev += 1
        if self.mask == 31:
            self.phase = "FLASHING"; self.target = 0; self.flash_on = True; self.flash_at = time.monotonic()
        else: self.choose()

    def reply(self, seq, ok=True, error=None, sid=None):
        value = {"v":1,"sid":sid or self.sid,"seq":seq,"ok":ok,"state":self.state()}
        if not ok: value["error"] = error
        return value

    def handle(self, msg):
        if not isinstance(msg, dict) or msg.get("v") != 1 or not isinstance(msg.get("seq"), int): return None
        cmd, sid, seq = msg.get("cmd"), msg.get("sid"), msg["seq"]
        if cmd == "HELLO" and sid != self.sid:
            self.sid = sid; self.last_seq = 0; self.last_reply = None; self.stop()
        if sid != self.sid: return self.reply(seq, False, "SESSION", sid)
        signature = json.dumps(msg, sort_keys=True, separators=(",", ":"))
        if seq == self.last_seq and self.last_reply is not None:
            return self.last_reply if signature == self.last_request else self.reply(seq, False, "SEQ_CONFLICT")
        if seq < self.last_seq: return self.reply(seq, False, "OLD_SEQ")
        error = None
        if cmd in ("START","HIT","MISS") and msg.get("rev") != self.rev: error = "STALE"
        elif cmd == "START":
            if msg.get("mode") not in ("SMALL","LARGE"): error = "MODE"
            elif self.phase != "IDLE": error = "BUSY"
            else: self.start(msg["mode"])
        elif cmd in ("HIT","MISS"):
            if self.phase != "RUNNING": error = "PHASE"
            elif msg.get("target") != self.target: error = "STALE"
            elif cmd == "HIT": self.hit()
            else: self.start(self.mode)
        elif cmd == "STOP": self.stop()
        elif cmd not in ("HELLO","STATUS"): error = "BAD_FRAME"
        self.last_seen = time.monotonic(); self.last_seq = seq; self.last_request = signature
        self.last_reply = self.reply(seq, error is None, error)
        return self.last_reply

    def tick(self):
        now = time.monotonic(); changed = False
        if self.phase != "IDLE" and now - self.last_seen > 6:
            self.stop(); return True
        while self.phase == "FLASHING" and now - self.flash_at >= .25:
            self.flash_at += .25; self.rev += 1; changed = True
            if self.flash_on: self.flash_on = False
            else:
                self.flashes += 1; self.flash_on = True
                if self.flashes == 10: self.phase = "COMPLETE"
        return changed

def send(port, value):
    raw = (json.dumps(value, separators=(",", ":")) + "\n").encode("ascii")
    port.write(raw); print("TX", raw.decode().strip())

def main():
    parser = argparse.ArgumentParser(description="PnX_Energy HC-42 主板模拟器")
    parser.add_argument("--port", required=True, help="例如 COM5")
    parser.add_argument("--baud", type=int, default=9600)
    args = parser.parse_args(); model = Model()
    with serial.Serial(args.port, args.baud, timeout=.05) as port:
        print(f"已打开 {args.port} @ {args.baud} 8N1，Ctrl+C退出")
        buffer = bytearray()
        while True:
            chunk = port.read(256)
            if chunk: buffer.extend(chunk)
            while b"\n" in buffer:
                line, _, buffer = buffer.partition(b"\n")
                try:
                    msg = json.loads(line.decode("ascii").rstrip("\r")); print("RX", msg)
                    reply = model.handle(msg)
                    if reply: send(port, reply)
                except Exception as exc: print("坏帧：", exc, file=sys.stderr)
            if len(buffer) > 1024: buffer.clear(); print("坏帧：超过1024字节", file=sys.stderr)
            if model.tick() and model.sid: send(port, model.reply(0))

if __name__ == "__main__":
    try: main()
    except KeyboardInterrupt: print("\n已退出")

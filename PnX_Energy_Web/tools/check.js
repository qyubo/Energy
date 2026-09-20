const fs = require('node:fs');
const path = require('node:path');
const cp = require('node:child_process');

const root = path.resolve(__dirname, '..');
function walk(dir) {
  return fs.readdirSync(dir, { withFileTypes: true }).flatMap(entry => {
    const full = path.join(dir, entry.name);
    return entry.isDirectory() ? walk(full) : [full];
  });
}
const files = walk(root).filter(f => !f.includes(`${path.sep}.git${path.sep}`));
for (const file of files.filter(f => f.endsWith('.json'))) JSON.parse(fs.readFileSync(file, 'utf8'));
for (const file of files.filter(f => f.endsWith('.js'))) {
  const result = cp.spawnSync(process.execPath, ['--check', file], { encoding: 'utf8' });
  if (result.status !== 0) throw new Error(`${file}\n${result.stderr}`);
}
const app = JSON.parse(fs.readFileSync(path.join(root, 'miniprogram', 'app.json'), 'utf8'));
for (const page of app.pages) {
  for (const ext of ['js', 'json', 'wxml', 'wxss']) {
    const file = path.join(root, 'miniprogram', `${page}.${ext}`);
    if (!fs.existsSync(file)) throw new Error(`缺少页面文件: ${file}`);
  }
}
const config = require(path.join(root, 'miniprogram', 'lib', 'config'));
if (config.service !== '0000ffe0-0000-1000-8000-00805f9b34fb' ||
    config.characteristic !== '0000ffe1-0000-1000-8000-00805f9b34fb') {
  throw new Error('HC-42 UUID配置错误：浏览器要求小写完整 UUID');
}
console.log(`静态检查通过：${files.length} 个文件，${app.pages.length} 个页面。`);

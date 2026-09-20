const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');

const root = path.resolve(__dirname, '..', 'web');
const port = Number(process.env.PNX_PORT || 8080);
const types = { '.html': 'text/html; charset=utf-8', '.css': 'text/css; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8', '.json': 'application/json; charset=utf-8',
  '.svg': 'image/svg+xml' };

http.createServer((request, response) => {
  if (request.method !== 'GET' && request.method !== 'HEAD') {
    response.writeHead(405); response.end(); return;
  }
  let url;
  try { url = new URL(request.url, 'http://localhost'); }
  catch (_) { response.writeHead(400); response.end(); return; }
  const name = url.pathname === '/' ? 'index.html' : url.pathname.slice(1);
  if (name.includes('..') || name.includes('\\') || name.includes('\0')) {
    response.writeHead(403); response.end(); return;
  }
  const file = path.join(root, name);
  fs.stat(file, (error, stat) => {
    if (error || !stat.isFile()) { response.writeHead(404); response.end('Not found'); return; }
    response.writeHead(200, { 'Content-Type': types[path.extname(file)] || 'application/octet-stream',
      'Cache-Control': 'no-store', 'X-Content-Type-Options': 'nosniff' });
    if (request.method === 'HEAD') { response.end(); return; }
    fs.createReadStream(file).pipe(response);
  });
}).listen(port, '127.0.0.1', () => {
  console.log(`PnX Energy 本机网页：http://localhost:${port}`);
  console.log('此窗口保持打开；关闭窗口即停止网页服务。');
});

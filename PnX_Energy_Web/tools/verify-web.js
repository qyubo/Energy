// Optional visual/interaction check: node tools/verify-web.js <playwright-package-dir> <chrome-exe>
const assert = require('node:assert/strict');
const path = require('node:path');
const { chromium } = require(process.argv[2] || 'playwright');

(async () => {
  const browser = await chromium.launch({ headless: true, executablePath: process.argv[3] });
  try {
    const page = await browser.newPage({ viewport: { width: 390, height: 844 }, deviceScaleFactor: 1 });
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    await page.goto('http://localhost:8080', { waitUntil: 'networkidle' });
    assert.equal(await page.title(), 'PnX Energy · 能量机关控制台');
    await page.screenshot({ path: path.resolve(__dirname, '..', 'web', 'preview-mobile.png'), fullPage: true });
    await page.setViewportSize({ width: 1280, height: 900 });
    await page.screenshot({ path: path.resolve(__dirname, '..', 'web', 'preview-desktop.png'), fullPage: true });
    await page.setViewportSize({ width: 390, height: 844 });
    await page.locator('#demo').click();
    await page.locator('#connection.online').waitFor();
    await page.locator('#mode-small').click();
    await page.locator('#screen-test:not([hidden])').waitFor();
    for (let i = 1; i <= 5; i++) {
      await page.locator('#hit').click();
      await page.waitForFunction(count => document.querySelectorAll('#progress .step.done').length === count, i);
    }
    await page.locator('#screen-complete:not([hidden])').waitFor({ timeout: 9000 });
    assert.equal(await page.locator('#complete-flashes').textContent(), '10');
    await page.locator('#restart').click();
    await page.locator('#screen-home:not([hidden])').waitFor();
    await page.locator('#mode-large').click();
    await page.locator('#screen-test:not([hidden])').waitFor();
    await page.locator('#miss').click();
    await page.waitForFunction(() => document.querySelector('#round-number').textContent === '3');
    await page.locator('#exit').click();
    await page.locator('#screen-home:not([hidden])').waitFor();
    await page.locator('#disconnect').click();
    await page.locator('#connect:not([hidden])').waitFor();
    assert.deepEqual(errors, []);
    console.log('网页真浏览器检查通过：移动布局、大小符演示、五击完成、未击中重开、退出和切回真实连接。');
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });

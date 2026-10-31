import { readFile, writeFile, mkdir, readdir } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const defaultRoot = fileURLToPath(new URL('../', import.meta.url));
export async function syncEsp32({ check = false, rcRoot = defaultRoot } = {}) {
  const rc = rcRoot;
  const sketch = path.join(rc, 'ESP32', 'rc_esp32');
  const outputs = [];
  const headers = (await readdir(path.join(rc, 'Common'))).filter(name => name.endsWith('.h')).sort();
  for (const name of headers) {
    const source = await readFile(path.join(rc, 'Common', name));
    outputs.push([name, Buffer.concat([Buffer.from(`// GENERATED — do not edit; source: RC/Common/${name}\n`), source])]);
  }
  const config = await readFile(path.join(rc, 'Common', 'rc_shared_config.h'), 'utf8');
  const port = config.match(/^#define\s+RC_WEBSOCKET_PORT\s+(\d+)U?\s*$/m)?.[1];
  if (!port) throw new Error('Cannot read RC_WEBSOCKET_PORT');
  let embedded = '// GENERATED — do not edit; source: RC/ESP32/web/ (node RC/tools/sync_esp32.mjs)\n#pragma once\n#include <pgmspace.h>\n';
  for (const [name, symbol] of [['index.html', 'WEB_INDEX'], ['style.css', 'WEB_STYLE'], ['controls.js', 'WEB_CONTROLS']]) {
    let source = await readFile(path.join(rc, 'ESP32', 'web', name), 'utf8');
    if (name === 'index.html') source = source.replaceAll('@@RC_WEBSOCKET_PORT@@', port);
    if (source.includes(')RC_ASSET"')) throw new Error(`Raw string delimiter collision: ${name}`);
    embedded += `\nstatic const char ${symbol}[] PROGMEM = R"RC_ASSET(${source})RC_ASSET";\n`;
  }
  outputs.push(['web_ui.h', Buffer.from(embedded)]);
  if (!check) await mkdir(sketch, { recursive: true });
  const stale = [];
  for (const [name, expected] of outputs) {
    const destination = path.join(sketch, name);
    let current;
    try { current = await readFile(destination); } catch (error) { if (error.code !== 'ENOENT') throw error; }
    if (!current?.equals(expected)) {
      if (check) stale.push(name);
      else await writeFile(destination, expected);
    }
  }
  return stale;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const check = process.argv.includes('--check');
  if (process.argv.slice(2).some(argument => argument !== '--check')) throw new Error('Usage: node RC/tools/sync_esp32.mjs [--check]');
  const stale = await syncEsp32({ check });
  for (const name of stale) console.error(`Stale generated file: RC/ESP32/rc_esp32/${name}`);
  if (stale.length) process.exitCode = 1;
  else console.log(check ? 'ESP32 generated files are current.' : 'ESP32 generated files synchronized.');
}

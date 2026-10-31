import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile, writeFile, mkdir, mkdtemp } from 'node:fs/promises';
import path from 'node:path';
import { tmpdir } from 'node:os';
import { syncEsp32 } from '../tools/sync_esp32.mjs';

test('generated files pass the same check used by sync_esp32 --check', async () => {
  assert.deepEqual(await syncEsp32({ check: true }), []);
});
test('sync detects missing/stale files and regeneration is deterministic', async () => {
  const build = path.join(tmpdir(), 'gc-rc-esp32-build');
  await mkdir(build, { recursive: true });
  const fixture = await mkdtemp(path.join(build, 'sync-fixture-'));
  await mkdir(path.join(fixture, 'Common'), { recursive: true });
  await mkdir(path.join(fixture, 'ESP32', 'web'), { recursive: true });
  for (const name of ['rc_protocol.h', 'rc_shared_config.h'])
    await writeFile(path.join(fixture, 'Common', name), await readFile(new URL(`../Common/${name}`, import.meta.url)));
  for (const name of ['index.html', 'style.css', 'controls.js'])
    await writeFile(path.join(fixture, 'ESP32', 'web', name), await readFile(new URL(`../ESP32/web/${name}`, import.meta.url)));
  const missing = await syncEsp32({ check: true, rcRoot: fixture });
  assert.deepEqual(missing, ['rc_protocol.h', 'rc_shared_config.h', 'web_ui.h']);
  await syncEsp32({ rcRoot: fixture });
  assert.deepEqual(await syncEsp32({ check: true, rcRoot: fixture }), []);
  const generated = path.join(fixture, 'ESP32', 'rc_esp32', 'web_ui.h');
  const first = await readFile(generated);
  await syncEsp32({ rcRoot: fixture }); assert.deepEqual(await readFile(generated), first);
  await writeFile(generated, 'stale');
  assert.deepEqual(await syncEsp32({ check: true, rcRoot: fixture }), ['web_ui.h']);
  assert.equal(await readFile(generated, 'utf8'), 'stale'); // Check mode performs no writes.
  await syncEsp32({ rcRoot: fixture }); assert.deepEqual(await readFile(generated), first);
});
test('generated common headers preserve every source byte after the marker', async () => {
  for (const name of ['rc_protocol.h', 'rc_shared_config.h']) {
    const source = await readFile(new URL(`../Common/${name}`, import.meta.url));
    const generated = await readFile(new URL(`../ESP32/rc_esp32/${name}`, import.meta.url));
    const marker = Buffer.from(`// GENERATED — do not edit; source: RC/Common/${name}\n`);
    assert.deepEqual(generated.subarray(0, marker.length), marker); assert.deepEqual(generated.subarray(marker.length), source);
  }
});

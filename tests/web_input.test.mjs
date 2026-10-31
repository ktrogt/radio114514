import test from 'node:test';
import assert from 'node:assert/strict';
import { KeyboardState, encodeMessage, MESSAGE_SIZE, BUTTON, statusBadge } from '../ESP32/web/controls.js';
import { decodeBrowser, mapAxes } from './protocol_helpers.mjs';

function keyboard() { const state = new KeyboardState(); state.focus(); return state; }
test('browser message is 26 bytes with exact little-endian offsets', () => {
  const state = { focused: true, lx: -1000, ly: 1000, rx: -500, ry: 123, lt: 128, rt: 255, buttons: 127, kick: 0x1234, dribbler: 0x5678, reset: 0xabcd, line: 0xffff };
  const buffer = encodeMessage(state, 0x4321), bytes = new Uint8Array(buffer);
  assert.equal(MESSAGE_SIZE, 26);
  assert.deepEqual([...bytes], [0x56, 1, 1, 0, 0x21, 0x43, 0x18, 0xfc, 0xe8, 3, 0x0c, 0xfe, 123, 0, 128, 255, 127, 0, 0x34, 0x12, 0x78, 0x56, 0xcd, 0xab, 255, 255]);
  assert.deepEqual(decodeBrowser(buffer).axes, [-1000, 1000, -500, 123]);
});
test('message sequence and press counters wrap modulo 65536', () => {
  const state = keyboard(); state.counters.kick = 65535; state.keyDown('Space');
  assert.equal(state.counters.kick, 0);
  assert.equal(decodeBrowser(encodeMessage(state.snapshot(), 65536)).sequence, 0);
  state.keyUp('Space'); state.keyDown('Space'); assert.equal(state.counters.kick, 1);
});
test('auto-repeat and duplicate keydowns never repeat a press', () => {
  const state = keyboard();
  assert.equal(state.keyDown('Space'), true); assert.equal(state.keyDown('Space', true), false);
  assert.equal(state.keyDown('Space'), false); assert.equal(state.counters.kick, 1);
  state.keyUp('Space'); state.keyDown('Space'); assert.equal(state.counters.kick, 2);
});
test('short presses between periodic ticks persist in all four counters', () => {
  const state = keyboard();
  for (const key of ['Space', 'KeyF', 'KeyR', 'KeyG']) { state.keyDown(key); state.keyUp(key); }
  const message = decodeBrowser(encodeMessage(state.snapshot(), 1));
  assert.equal(message.buttons, 0); assert.deepEqual(message.counters, [1, 1, 1, 1]);
});
test('blur clears all keys, sends unfocused neutral, and focus restores no keys', () => {
  const state = keyboard();
  for (const key of ['KeyW', 'KeyE', 'ShiftLeft', 'KeyC', 'Space', 'Escape']) state.keyDown(key);
  state.blur(); const lost = decodeBrowser(encodeMessage(state.snapshot(), 2));
  assert.equal(lost.focused, false); assert.deepEqual(lost.axes, [0, 0, 0, 0]); assert.equal(lost.buttons, 0); assert.equal(lost.lt, 0);
  assert.equal(state.keyDown('KeyW'), false); state.focus(); assert.deepEqual(state.snapshot().lx, 0); assert.equal(state.snapshot().ly, 0);
  assert.equal(state.counters.kick, 1);
});
test('keyboard diagonals normalize in the shared mapping and obey all speed caps', () => {
  const state = keyboard(); state.keyDown('KeyW'); state.keyDown('KeyD');
  assert.equal(state.snapshot().lx, 1000); assert.equal(state.snapshot().ly, 1000);
  for (const [key, cap] of [[null, 800], ['ShiftLeft', 1000], ['KeyC', 400]]) {
    if (key) state.keyDown(key);
    const mapped = mapAxes(state.snapshot()); assert.equal(mapped.x, mapped.y);
    assert.ok(mapped.x*mapped.x + mapped.y*mapped.y <= cap*cap); assert.ok(Math.hypot(mapped.x, mapped.y) >= cap - 2);
  }
  assert.equal(mapAxes(state.snapshot()).boost, false); assert.equal(mapAxes(state.snapshot()).slow, true);
});
test('opposite keys cancel; E/right is clockwise; Q/left is negative', () => {
  const state = keyboard(); state.keyDown('KeyW'); state.keyDown('KeyS'); state.keyDown('KeyA'); state.keyDown('KeyD'); state.keyDown('KeyE');
  assert.deepEqual(mapAxes(state.snapshot()), { x: 0, y: 0, turn: 18000, slow: false, boost: false });
  state.keyUp('KeyE'); state.keyDown('KeyQ'); assert.equal(mapAxes(state.snapshot()).turn, -18000);
});
test('radial and turn deadzones rescale, slow wins at the threshold', () => {
  const neutral = { lx: 120, ly: 0, rx: -120, lt: 127, buttons: BUTTON.RB };
  assert.equal(mapAxes(neutral).x, 0); assert.equal(mapAxes(neutral).turn, 0); assert.equal(mapAxes({ ...neutral, rx: 250 }).turn, 0); assert.equal(mapAxes({ ...neutral, rx: -250 }).turn, 0);
  const halfway = mapAxes({ ...neutral, lx: 560, rx: 560, lt: 128 });
  assert.equal(halfway.x, 200); assert.equal(halfway.turn, 3720); assert.equal(halfway.boost, false);
});
test('malformed/reserved/out-of-range/unfocused-active browser messages reject', () => {
  const state = keyboard(); const original = encodeMessage(state.snapshot(), 0);
  for (const [offset, value] of [[0, 0], [1, 2], [2, 2], [3, 1], [17, 1]]) {
    const changed = original.slice(0); new Uint8Array(changed)[offset] = value; assert.equal(decodeBrowser(changed), null);
  }
  assert.equal(decodeBrowser(original.slice(0, 25)), null);
  const outside = original.slice(0); new DataView(outside).setInt16(6, 1001, true); assert.equal(decodeBrowser(outside), null);
  state.keyDown('KeyW'); const hidden = encodeMessage(state.snapshot(), 1); new Uint8Array(hidden)[2] = 0; assert.equal(decodeBrowser(hidden), null);
});
test('state badges reflect real robot permission, stop and neutral rearm', () => {
  const base = { robotLink: true, owner: 'browser', operatorStop: false, flags: 3, fault: 0, handover: false };
  assert.equal(statusBadge(base)[0], '操作できます');
  assert.equal(statusBadge({ ...base, operatorStop: true })[0], '停止中（Esc で解除）');
  assert.equal(statusBadge({ ...base, flags: 3 | 2048 })[0], 'キーを離してください');
  assert.equal(statusBadge({ ...base, flags: 0 })[0], '赤スイッチで有効にしてください');
  assert.equal(statusBadge({ ...base, owner: 'gamepad' })[0], 'ゲームパッドで操作中');
  assert.equal(statusBadge({ ...base, robotLink: false })[0], '接続待ち');
});
test('pairing window badge uses the specified text and idle class, including the pairing fault', () => {
  const pairing = { robotLink: true, owner: 'none', operatorStop: false, flags: 1 << 9, fault: 6, handover: false };
  assert.deepEqual(statusBadge(pairing), ['ペアリング中（ロボットは停止）', 'idle']);
  assert.deepEqual(statusBadge({ ...pairing, fault: 0 }), ['ペアリング中（ロボットは停止）', 'idle']);
  assert.deepEqual(statusBadge({ ...pairing, robotLink: false }), ['接続待ち', 'idle']);
  assert.deepEqual(statusBadge({ ...pairing, operatorStop: true }), ['停止中（Esc で解除）', 'stopped']);
});

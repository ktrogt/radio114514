import test from 'node:test';
import assert from 'node:assert/strict';
import { ControllerState, BrowserInput, ScreenAwake, BUTTON, encodeMessage, defaultMode } from '../ESP32/web/controls.js';
import { decodeBrowser } from './protocol_helpers.mjs';

export const gamepad = (options = {}) => ({ id: 'Switch Pro Controller', index: 0, connected: true, mapping: 'standard',
  axes: [0, 0, 0, 0], buttons: Array.from({ length: 17 }, () => ({ pressed: false, value: 0 })), ...options });
const state = () => { const controller = new ControllerState(); controller.focus(); return controller; };
const message = controller => decodeBrowser(encodeMessage(controller.snapshot(), 0));
const press = (pad, index, down = true) => { pad.buttons[index] = { pressed: down, value: Number(down) }; };
const neutral = controller => {
  const result = message(controller); assert.deepEqual(result.axes, [0, 0, 0, 0]);
  assert.equal(result.buttons, 0); assert.equal(result.lt, 0);
};

test('standard controller maps axes positionally, inverts Y, preserves tiny inputs, and maps analog LT/RB', () => {
  const controller = state(), pad = gamepad(); controller.update([pad]);
  pad.axes = [.001, -.75, .5, -1]; press(pad, 5); pad.buttons[6] = { pressed: false, value: .5 };
  controller.update([pad]); const result = message(controller);
  assert.deepEqual(result.axes, [1, 750, 500, 0]); assert.equal(result.buttons, BUTTON.RB);
  assert.equal(result.lt, 128); assert.equal(result.rt, 0);
  pad.axes = [-2, 2, -1, 1]; pad.buttons[6].value = 2; controller.update([pad]);
  assert.deepEqual(message(controller).axes, [-1000, -1000, -1000, 0]); assert.equal(message(controller).lt, 255);
});
test('only the first connected standard pad is used; sparse/disconnected/non-standard pads are ignored', () => {
  const controller = state(), custom = gamepad({ mapping: '', index: 0 }), first = gamepad({ index: 2 }), second = gamepad({ index: 3 });
  controller.update([null, custom]); neutral(controller); assert.equal(controller.nonStandard, true); assert.equal(controller.id, null);
  controller.update([gamepad({ connected: false }), custom, first, second]); assert.equal(controller.index, 2);
  press(first, 0); press(second, 1); controller.update([custom, first, second]);
  assert.equal(message(controller).buttons, BUTTON.A); assert.deepEqual(message(controller).counters, [1, 0, 0, 0]);
});
test('face-button counters count rising edges only and wrap; Back carries ordered held/released states', () => {
  const controller = state(), pad = gamepad(); controller.update([pad]); controller.counters.kick = 65535;
  for (const index of [0, 1, 2, 3, 8]) press(pad, index);
  controller.update([pad]); assert.deepEqual(message(controller).counters, [0, 1, 1, 1]); assert.equal(message(controller).buttons, 79);
  controller.update([pad]); assert.deepEqual(message(controller).counters, [0, 1, 1, 1]);
  for (const index of [0, 1, 2, 3, 8]) press(pad, index, false);
  controller.update([pad]); assert.equal(message(controller).buttons, 0);
  press(pad, 0); controller.update([pad]); assert.equal(controller.counters.kick, 1);
});
test('buttons held on initial exposure or focus return cannot fire until release is observed', () => {
  const controller = state(), pad = gamepad();
  for (const index of [0, 1, 2, 3, 5, 6, 8]) press(pad, index);
  controller.update([pad]); neutral(controller); assert.deepEqual(message(controller).counters, [0, 0, 0, 0]);
  for (const index of [0, 1, 2, 3, 5, 6, 8]) press(pad, index, false);
  controller.update([pad]); press(pad, 0); controller.update([pad]); assert.equal(controller.counters.kick, 1);
  controller.blur(); neutral(controller); assert.equal(message(controller).focused, false);
  controller.focus(); controller.update([pad]); assert.equal(message(controller).buttons, 0); assert.equal(controller.counters.kick, 1);
  press(pad, 0, false); controller.update([pad]); press(pad, 0); controller.update([pad]); assert.equal(controller.counters.kick, 2);
});
test('held sticks do not resume after focus/device changes until centered within the ESP32 deadzones; raw axes pass after that', () => {
  const controller = state(), pad = gamepad({ axes: [1, -.5, 1, 0] });
  controller.update([pad]); neutral(controller); assert.equal(controller.motionReady, false);
  // A resting stick that is slightly off-center still counts as centered (move 0.12, turn 0.25).
  pad.axes = [.08, .05, -.2, 1]; controller.update([pad]); assert.equal(controller.motionReady, true);
  assert.deepEqual(message(controller).axes, [80, -50, -200, 0]); // Raw values; the ESP32 applies deadzones.
  controller.focus(); pad.axes = [.5, 0, 0, 0]; controller.update([pad]); neutral(controller);
  assert.equal(controller.motionReady, false);
  pad.axes = [0, 0, .3, 0]; controller.update([pad]); neutral(controller); // Turn held beyond 0.25.
  pad.axes = [0, 0, .1, 0]; controller.update([pad]); assert.equal(controller.motionReady, true);
});
test('poll disappearance/disconnect clears all controls and requires released buttons on reconnection', () => {
  const controller = state(), pad = gamepad(); controller.update([pad]);
  pad.axes[0] = 1; press(pad, 0); press(pad, 8); controller.update([pad]);
  controller.update([]); neutral(controller); assert.equal(controller.id, null); assert.equal(controller.seen, true);
  controller.update([pad]); neutral(controller); assert.equal(controller.counters.kick, 1);
  controller.disconnect(); neutral(controller);
});
test('mode switches retain shared counters, clear input, and remember controller mode', () => {
  const input = new BrowserInput(); input.focus(); input.keyboard.keyDown('Space');
  input.switchMode('controller'); neutral(input); input.focus(); const pad = gamepad(); input.controller.update([pad]);
  press(pad, 0); input.controller.update([pad]); assert.equal(input.snapshot().kick, 2);
  input.switchMode('stick'); neutral(input); assert.equal(input.snapshot().kick, 2);
  assert.equal(defaultMode(false, { getItem: () => 'controller' }), 'controller');
});
test('touch stop can suppress a still-held physical Back until its release', () => {
  const controller = state(), pad = gamepad(); controller.update([pad]); press(pad, 8); controller.update([pad]);
  assert.equal(message(controller).buttons, BUTTON.BACK); controller.blockBackUntilRelease(); controller.update([pad]); neutral(controller);
  press(pad, 8, false); controller.update([pad]); press(pad, 8); controller.update([pad]); assert.equal(message(controller).buttons, BUTTON.BACK);
});
test('optional wake lock tolerates absence/denial and releases a request that resolves after hiding', async () => {
  await new ScreenAwake().update(true);
  await new ScreenAwake({ request: async () => { throw new Error('denied'); } }).update(true);
  let resolveRequest, requests = 0, releases = 0;
  const lock = Object.assign(new EventTarget(), { release: async () => { releases++; } });
  const awake = new ScreenAwake({ request: kind => { assert.equal(kind, 'screen'); requests++; return new Promise(resolve => { resolveRequest = resolve; }); } });
  const pending = awake.update(true); await awake.update(true); assert.equal(requests, 1);
  await awake.update(false); resolveRequest(lock); await pending; assert.equal(releases, 1); assert.equal(awake.lock, null);
});
test('wake lock is retained while wanted and released when the controller page becomes inactive', async () => {
  let releases = 0; const lock = Object.assign(new EventTarget(), { release: async () => { releases++; } });
  const awake = new ScreenAwake({ request: async () => lock });
  await awake.update(true); assert.equal(awake.lock, lock);
  lock.dispatchEvent(new Event('release')); assert.equal(awake.lock, null);
  await awake.update(true); await awake.update(false); assert.equal(releases, 1); assert.equal(awake.lock, null);
});
test('ZR/RT (button 7) also kicks once per press; holding it with the bottom button counts once; R/RB stays boost', () => {
  const controller = state(), pad = gamepad(); controller.update([pad]);
  press(pad, 7); controller.update([pad]); controller.update([pad]);
  assert.equal(controller.counters.kick, 1); assert.equal(message(controller).buttons, BUTTON.A);
  press(pad, 0); controller.update([pad]); assert.equal(controller.counters.kick, 1);
  press(pad, 7, false); controller.update([pad]); assert.equal(controller.counters.kick, 1);
  press(pad, 0, false); controller.update([pad]); press(pad, 7); controller.update([pad]);
  assert.equal(controller.counters.kick, 2);
  press(pad, 7, false); press(pad, 5); controller.update([pad]);
  assert.equal(controller.counters.kick, 2); assert.equal(message(controller).buttons, BUTTON.RB);
});

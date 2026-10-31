import test from 'node:test';
import assert from 'node:assert/strict';
import { stickAxes, throttleAxes, StickState, BrowserInput, defaultMode, rememberMode, MODE_STORAGE_KEY, BUTTON, encodeMessage } from '../ESP32/web/controls.js';
import { decodeBrowser, mapAxes } from './protocol_helpers.mjs';

const stick = () => { const state = new StickState(); state.focus(); return state; };
const message = state => decodeBrowser(encodeMessage(state.snapshot(), 0));
const neutral = state => {
  const result = message(state);
  assert.deepEqual(result.axes, [0, 0, 0, 0]); assert.equal(result.buttons, 0); assert.equal(result.lt, 0);
};

test('movement stick clamps to the unit circle, preserves directions, and adds no deadzone', () => {
  assert.deepEqual(stickAxes(50, -50, 100), { x: 500, y: 500 });
  assert.deepEqual(stickAxes(1, 0, 100), { x: 10, y: 0 });
  assert.deepEqual(stickAxes(-200, 0, 100), { x: -1000, y: 0 });
  assert.deepEqual(stickAxes(0, 0, 0), { x: 0, y: 0 });
  const diagonal = stickAxes(200, -200, 100);
  assert.equal(diagonal.x, diagonal.y); assert.ok(Math.hypot(diagonal.x, diagonal.y) <= 1000);
  assert.ok(Math.hypot(diagonal.x, diagonal.y) > 998);
});
test('left stick has independent turn and bottom-to-top throttle', () => {
  assert.deepEqual(throttleAxes(0, 0, 100), { x: 0, throttle: .5 });
  assert.deepEqual(throttleAxes(200, -200, 100), { x: 1000, throttle: 1 });
  assert.deepEqual(throttleAxes(-200, 200, 100), { x: -1000, throttle: 0 });
  assert.deepEqual(throttleAxes(40, -50, 100), { x: 400, throttle: .75 });
});
test('throttle starts at 50 percent and scales movement only, rounded on the existing wire format', () => {
  const state = stick(); assert.equal(state.throttle, .5);
  state.down('move', 1, 707, 707); state.down('turn', 2, 1000, .5);
  assert.deepEqual(message(state).axes, [354, 354, 1000, 0]);
  assert.equal(message(state).buttons, BUTTON.RB); assert.equal(message(state).lt, 0);
  assert.equal(mapAxes(state.snapshot()).turn, 27000);
  state.move(2, 1000, 1); state.move(1, 1000, 0);
  assert.equal(mapAxes(state.snapshot()).x, 1000);
  state.move(2, 1000, 0); assert.deepEqual(message(state).axes, [0, 0, 1000, 0]);
});
test('left release/cancel retains throttle and resets only turn; right release resets movement', () => {
  const state = stick(); state.down('turn', 1, -700, .5); state.move(1, -700, .8); state.down('move', 2, 1000, 0);
  state.release(1); assert.equal(state.throttle, .8); assert.deepEqual(message(state).axes, [800, 0, 0, 0]);
  state.release(2); neutral(state); assert.equal(state.throttle, .8);
  assert.equal(state.move(1, 1000, 0), false);
});
test('multi-touch isolates two sticks and action presses; a second pointer cannot steal a stick', () => {
  const state = stick(); state.down('move', 1, 1000, 0); state.down('turn', 2, -400, .5); state.move(2, -400, .7); state.down('kick', 3);
  assert.equal(state.down('move', 4, -1000, 0), false);
  assert.equal(state.down('reset', 3), false);
  assert.deepEqual(message(state).axes, [700, 0, -400, 0]); assert.equal(message(state).buttons, BUTTON.RB | BUTTON.A);
  state.release(1); assert.deepEqual(message(state).axes, [0, 0, -400, 0]);
  state.release(3); assert.equal(state.counters.kick, 1); assert.equal(message(state).buttons, BUTTON.RB);
  state.release(2); neutral(state);
});
test('short touch actions count once and wrap; neutral never carries RB or LT', () => {
  const state = stick(); state.counters.kick = 65535;
  state.down('kick', 1); assert.equal(state.down('kick', 2), false); state.release(1);
  state.down('dribbler', 2); state.release(2); state.down('reset', 3); state.release(3); state.down('line', 4); state.release(4);
  neutral(state); assert.deepEqual(message(state).counters, [0, 1, 1, 1]);
  state.down('turn', 5, 0, .5); state.move(5, 0, 0); state.down('move', 6, 1000, 0); neutral(state);
});
test('blur and refocus clear all motion and actions but retain the non-centering throttle', () => {
  const state = stick(); state.down('turn', 1, 1000, .5); state.move(1, 1000, .9); state.down('move', 2, 1000, 0); state.down('stop', 3);
  state.blur(); neutral(state); assert.equal(message(state).focused, false); assert.equal(state.throttle, .9);
  assert.equal(state.down('move', 2, 1000, 0), false); state.focus(); neutral(state); assert.equal(state.throttle, .9);
});
test('mode switches release motion, stop active input, and preserve throttle and shared counters', () => {
  const input = new BrowserInput('stick'); input.focus(); input.stick.down('turn', 1, 500, .5); input.stick.move(1, 500, .8); input.stick.down('kick', 2);
  input.switchMode('keyboard'); neutral(input); assert.equal(input.focused, false);
  input.focus(); input.keyboard.keyDown('KeyW'); input.keyboard.keyDown('Space'); input.keyboard.keyDown('ShiftLeft');
  assert.equal(message(input).buttons, BUTTON.A | BUTTON.RB); assert.equal(input.snapshot().kick, 2);
  input.switchMode('stick'); neutral(input); assert.equal(input.stick.throttle, .8); assert.equal(input.snapshot().kick, 2);
  input.resetSession(); assert.equal(input.stick.throttle, .8); assert.equal(input.snapshot().kick, 0);
});

test('left touch does not change throttle; slides use the initial touch and throttle as their baseline', () => {
  const state = stick();
  state.down('turn', 1, 900, 0); assert.equal(state.throttle, .5); assert.equal(message(state).axes[2], 900);
  state.move(1, -900, 0); assert.equal(state.throttle, .5); // Horizontal sliding only changes turn.
  state.move(1, 100, .25); assert.equal(state.throttle, .75);
  state.move(1, 100, .125); assert.equal(state.throttle, .625); // Relative to the original touch, not the previous move.
  state.release(1); assert.equal(state.throttle, .625); assert.equal(message(state).axes[2], 0);
  state.down('turn', 2, -100, 1); assert.equal(state.throttle, .625); // New touch near the top retains speed.
  state.move(2, -100, .75); assert.equal(state.throttle, .375);
  state.move(2, 100, 1); assert.equal(state.throttle, .625);
});

test('relative throttle clamps at both ends and a new touch re-baselines the retained value', () => {
  const state = stick(); state.down('turn', 1, 0, .25);
  state.move(1, 0, 1); assert.equal(state.throttle, 1);
  state.move(1, 0, .5); assert.equal(state.throttle, .75); // Clamping does not accumulate a new baseline.
  state.release(1); state.down('turn', 2, 0, 1); assert.equal(state.throttle, .75);
  state.move(2, 0, 0); assert.equal(state.throttle, 0);
  state.move(2, 0, .5); assert.equal(state.throttle, .25);
  state.release(2); neutral(state); assert.equal(state.throttle, .25);
});
test('default mode honors valid storage then coarse pointer, including denied/absent storage', () => {
  assert.equal(defaultMode(true), 'stick'); assert.equal(defaultMode(false), 'keyboard');
  for (const mode of ['keyboard', 'stick']) assert.equal(defaultMode(mode === 'keyboard', { getItem: () => mode }), mode);
  assert.equal(defaultMode(true, { getItem: () => 'obsolete' }), 'stick');
  assert.equal(defaultMode(false, { getItem() { throw new Error('denied'); } }), 'keyboard');
  let saved; rememberMode('stick', { setItem: (key, value) => { saved = [key, value]; } });
  assert.deepEqual(saved, [MODE_STORAGE_KEY, 'stick']);
  assert.doesNotThrow(() => rememberMode('stick', { setItem() { throw new Error('denied'); } }));
});

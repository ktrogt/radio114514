import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { readFile } from 'node:fs/promises';
import { decodeBrowser } from './protocol_helpers.mjs';

const script = (await readFile(new URL('../ESP32/web/controls.js', import.meta.url), 'utf8')).replaceAll('export ', '');
function page({ coarse = false, savedMode, storageThrows = false, pads = [], gamepadsThrows = false, wakeLock, initialFocus = true } = {}) {
  let focused = initialFocus, now = 0;
  const elements = new Map();
  const get = id => {
    if (!elements.has(id)) {
      const attributes = new Map(), pointers = new Set();
      elements.set(id, Object.assign(new EventTarget(), { hidden: false, textContent: '', className: '', style: {},
        getBoundingClientRect: () => ({ left: 0, top: 0, width: 200, height: 200 }),
        setAttribute: (name, value) => attributes.set(name, value), getAttribute: name => attributes.get(name),
        setPointerCapture: pointerId => pointers.add(pointerId), hasPointerCapture: pointerId => pointers.has(pointerId),
        releasePointerCapture: pointerId => {
          if (pointers.delete(pointerId)) get(id).dispatchEvent(Object.assign(new Event('lostpointercapture'), { pointerId }));
        },
      }));
    }
    return elements.get(id);
  };
  const document = Object.assign(new EventTarget(), { body: { dataset: {} }, documentElement: { dataset: {} }, hasFocus: () => focused, visibilityState: 'visible', getElementById: get, querySelector: () => ({ content: '81' }) });
  const storage = new Map(savedMode === undefined ? [] : [['gc-rc-input-mode', savedMode]]);
  let polls = 0;
  const navigator = { wakeLock, getGamepads: () => { polls++; if (gamepadsThrows) throw new Error('policy'); return pads; } };
  const window = Object.assign(new EventTarget(), { navigator, focus: () => { focused = true; }, matchMedia: () => ({ matches: coarse }) });
  Object.defineProperty(window, 'localStorage', { get() {
    if (storageThrows) throw new Error('storage unavailable');
    return { getItem: key => storage.get(key), setItem: (key, value) => storage.set(key, value) };
  } });
  const intervals = [], timeouts = new Map(); let nextTimer = 0;
  class Socket extends EventTarget {
    static OPEN = 1;
    static instances = [];
    constructor(url) { super(); this.url = url; this.readyState = 0; this.bufferedAmount = 0; this.messages = []; Socket.instances.push(this); }
    send(buffer) { this.messages.push(buffer); }
    open() { this.readyState = 1; this.dispatchEvent(new Event('open')); }
    close() { this.readyState = 3; this.dispatchEvent(new Event('close')); }
  }
  vm.runInNewContext(script, {
    document, window, WebSocket: Socket, location: { hostname: '192.168.4.1' }, performance: { now: () => now },
    setInterval: (callback, ms) => { intervals.push({ callback, ms }); },
    setTimeout: (callback, ms) => { const id = ++nextTimer; timeouts.set(id, { callback, ms }); return id; },
    clearTimeout: id => timeouts.delete(id),
  });
  const key = (type, code, repeat = false) => window.dispatchEvent(Object.assign(new Event(type, { cancelable: true }), { code, repeat }));
  return { document, window, get, Socket, intervals, timeouts, key, storage,
    get polls() { return polls; },
    setPads(value) { pads = value; },
    padEvent(type, gamepad) { window.dispatchEvent(Object.assign(new Event(type), { gamepad })); },
    pointer(id, type, pointerId, clientX = 100, clientY = 100) {
      get(id).dispatchEvent(Object.assign(new Event(type, { cancelable: true }), { pointerId, clientX, clientY, button: 0, pointerType: 'touch' }));
    },
    click(id, detail = 0) { get(id).dispatchEvent(Object.assign(new Event('click'), { detail })); },
    blur() { focused = false; window.dispatchEvent(new Event('blur')); },
    focus() { focused = true; window.dispatchEvent(new Event('focus')); },
    tick(ms) { now += ms; for (const item of intervals) if (ms >= item.ms) item.callback(); } };
}

const standardPad = (options = {}) => ({ id: 'PXN Switch Pro', index: 0, mapping: 'standard', connected: true,
  axes: [0, 0, 0, 0], buttons: Array.from({ length: 17 }, () => ({ pressed: false, value: 0 })), ...options });
const padButton = (pad, index, down = true) => { pad.buttons[index] = { pressed: down, value: Number(down) }; };

test('page sends 50Hz focused heartbeats and sends every press/release immediately', () => {
  const app = page(), socket = app.Socket.instances[0];
  assert.equal(socket.url, 'ws://192.168.4.1:81/'); socket.open();
  assert.ok(app.intervals.some(item => item.ms === 20));
  const initial = socket.messages.length;
  app.key('keydown', 'Space'); app.key('keyup', 'Space');
  assert.equal(socket.messages.length, initial + 2);
  assert.equal(decodeBrowser(socket.messages.at(-2)).buttons, 1);
  assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  assert.equal(decodeBrowser(socket.messages.at(-1)).counters[0], 1);
  app.tick(20); assert.equal(socket.messages.length, initial + 3);
  app.key('keydown', 'Space', true); assert.equal(socket.messages.length, initial + 3);
});
test('blur/hidden send immediate neutral and stop heartbeats; focus resumes neutral', () => {
  const app = page(), socket = app.Socket.instances[0]; socket.open(); app.key('keydown', 'KeyW');
  app.blur(); const lost = decodeBrowser(socket.messages.at(-1));
  assert.equal(lost.focused, false); assert.deepEqual(lost.axes, [0, 0, 0, 0]);
  const blurredCount = socket.messages.length; app.tick(100); assert.equal(socket.messages.length, blurredCount);
  assert.equal(app.get('focus-overlay').hidden, false);
  app.focus(); assert.equal(decodeBrowser(socket.messages.at(-1)).axes[1], 0);
  app.document.visibilityState = 'hidden'; app.document.dispatchEvent(new Event('visibilitychange'));
  assert.equal(decodeBrowser(socket.messages.at(-1)).focused, false);
  const hiddenCount = socket.messages.length; app.tick(100); assert.equal(socket.messages.length, hiddenCount);
});
test('stop button sends ordered Back edges even with a buffered edge message', () => {
  const app = page(), socket = app.Socket.instances[0]; socket.open(); socket.bufferedAmount = 26;
  app.get('stop').dispatchEvent(new Event('click'));
  assert.equal(decodeBrowser(socket.messages.at(-2)).buttons, 64);
  assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  const count = socket.messages.length; app.tick(20); assert.equal(socket.messages.length, count);
});
test('disconnect schedules automatic reconnection and never restores held keys', () => {
  const app = page(), socket = app.Socket.instances[0]; socket.open(); app.key('keydown', 'KeyD'); socket.close();
  const retry = [...app.timeouts.values()][0]; assert.equal(retry.ms, 500); retry.callback();
  const next = app.Socket.instances[1]; next.open();
  assert.deepEqual(decodeBrowser(next.messages[0]).axes, [0, 0, 0, 0]);
  assert.equal(decodeBrowser(next.messages[0]).sequence, 0);
});
test('excessive WebSocket buffering closes control rather than queuing movement', () => {
  const app = page(), socket = app.Socket.instances[0]; socket.open(); socket.bufferedAmount = 513;
  app.key('keydown', 'KeyW'); assert.equal(socket.readyState, 3);
});
test('F446 pairing status remains visible after the ESP enrollment window closes', () => {
  const app = page(), socket = app.Socket.instances[0]; socket.open();
  const status = { owner: 'none', operatorStop: false, robotLink: true, handover: false, flags: 513, fault: 0,
    heading: 0, targetHeading: 0, commandFlags: 8, pairingSeconds: 0, gamepadConnected: false,
    gamepadApproved: true, pairingSaved: true, firmware: '0.1.0' };
  socket.dispatchEvent(Object.assign(new Event('message'), { data: JSON.stringify(status) }));
  assert.equal(app.get('pairing').hidden, false); assert.match(app.get('pairing').textContent, /ペアリング中/);
  assert.equal(app.get('badge').textContent, 'ペアリング中（ロボットは停止）');
  assert.equal(app.get('badge').className, 'badge idle');
});
test('fault text uses the exact reviewed labels', () => {
  const app = page(), socket = app.Socket.instances[0]; socket.open();
  const labels = ['なし', 'ESP32 との通信切れ', '操作入力なし', 'ジャイロ異常', 'ライン基板と通信できません（G でライン補助を切ると動けます）', '通信データ異常', 'ペアリング中', '初期化異常'];
  for (let fault = 0; fault < labels.length; fault++) {
    const status = { owner: 'none', operatorStop: false, robotLink: true, handover: false, flags: 1, fault,
      heading: 0, targetHeading: 0, commandFlags: 8, pairingSeconds: 0, gamepadConnected: false,
      gamepadApproved: false, pairingSaved: true, firmware: '0.1.0' };
    socket.dispatchEvent(Object.assign(new Event('message'), { data: JSON.stringify(status) }));
    assert.equal(app.get('fault').textContent, `停止: ${labels[fault]}`);
    assert.equal(app.get('fault').hidden, fault === 0);
  }
});
test('footer uses the exact reviewed gamepad-priority wording', async () => {
  const html = await readFile(new URL('../ESP32/web/index.html', import.meta.url), 'utf8');
  assert.match(html, /ゲームパッドがつながっているときはゲームパッドが優先されます/);
  assert.doesNotMatch(html, /キーを離すと操作元を切り替えられます/);
});
test('stale status clears displayed telemetry while keyboard heartbeats continue', () => {
  const app = page(), socket = app.Socket.instances[0]; socket.open();
  const status = { owner: 'browser', operatorStop: false, robotLink: true, handover: false, flags: 3, fault: 0,
    heading: 12, targetHeading: 0, commandFlags: 8, pairingSeconds: 0, gamepadConnected: false,
    gamepadApproved: false, pairingSaved: true, firmware: '0.1.0' };
  socket.dispatchEvent(Object.assign(new Event('message'), { data: JSON.stringify(status) }));
  assert.match(app.get('heading').textContent, /12.0/);
  const count = socket.messages.length; app.tick(701);
  assert.equal(app.get('heading').textContent, '—'); assert.equal(app.get('badge').textContent, '接続待ち');
  assert.equal(socket.messages.length, count + 1);
});

test('page chooses coarse-pointer stick mode, respects saved mode, and tolerates denied storage', () => {
  assert.equal(page({ coarse: true }).document.body.dataset.mode, 'stick');
  assert.equal(page({ coarse: true, savedMode: 'keyboard' }).document.body.dataset.mode, 'keyboard');
  assert.equal(page({ savedMode: 'stick' }).document.body.dataset.mode, 'stick');
  const app = page({ coarse: true, storageThrows: true });
  assert.equal(app.document.body.dataset.mode, 'stick'); assert.match(app.get('viewport').content, /user-scalable=no/);
  app.click('mode-keyboard'); assert.equal(app.document.body.dataset.mode, 'keyboard');
  assert.doesNotMatch(app.get('viewport').content, /user-scalable=no/);
});

test('page captures both sticks and a button simultaneously; releases isolate controls', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.pointer('move-stick', 'pointerdown', 1, 164, 100);
  app.pointer('turn-stick', 'pointerdown', 2, 132, 100);
  app.pointer('turn-stick', 'pointermove', 2, 132, 68);
  app.pointer('touch-kick', 'pointerdown', 3);
  let state = decodeBrowser(socket.messages.at(-1));
  assert.deepEqual(state.axes, [750, 0, 500, 0]); assert.equal(state.buttons, 33); assert.equal(state.lt, 0);
  assert.equal(app.get('throttle').textContent, '速度 75%');
  app.pointer('move-stick', 'pointerdown', 4, 36, 100); // Cannot steal the captured right stick.
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [750, 0, 500, 0]);
  app.pointer('turn-stick', 'pointerup', 2, 132, 68);
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [750, 0, 0, 0]);
  assert.equal(app.get('turn-knob').style.transform, 'translate(0px, -32px)');
  app.pointer('touch-kick', 'pointerup', 3); app.click('touch-kick', 1);
  assert.equal(decodeBrowser(socket.messages.at(-1)).counters[0], 1);
  app.pointer('move-stick', 'pointerup', 1);
  state = decodeBrowser(socket.messages.at(-1)); assert.deepEqual(state.axes, [0, 0, 0, 0]); assert.equal(state.buttons, 0);
  assert.equal(app.get('move-knob').style.transform, 'translate(0px, 0px)');
  assert.equal(app.get('throttle').textContent, '速度 75%');
  const count = socket.messages.length; app.tick(20); assert.equal(socket.messages.length, count + 1);
});

test('pointer moves clamp right movement and update left throttle and turn independently', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.pointer('move-stick', 'pointerdown', 1); app.pointer('turn-stick', 'pointerdown', 2);
  app.pointer('turn-stick', 'pointermove', 2, 164, 36); // Full throttle and clockwise turn.
  app.pointer('move-stick', 'pointermove', 1, 300, -100);
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [707, 707, 1000, 0]);
  app.pointer('turn-stick', 'pointermove', 2, 100, 164); // Zero throttle, zero turn.
  const state = decodeBrowser(socket.messages.at(-1)); assert.deepEqual(state.axes, [0, 0, 0, 0]); assert.equal(state.buttons, 0);
  app.pointer('turn-stick', 'pointerup', 2, 100, 164); assert.equal(app.get('throttle').textContent, '速度 0%');
});

test('pointercancel and capture loss immediately neutralize only their control', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.pointer('turn-stick', 'pointerdown', 1, 164, 100); app.pointer('turn-stick', 'pointermove', 1, 164, 36); app.pointer('move-stick', 'pointerdown', 2, 164, 100);
  app.pointer('touch-dribbler', 'pointerdown', 3);
  app.pointer('move-stick', 'pointercancel', 2);
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 1000, 0]);
  assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 34);
  app.get('turn-stick').releasePointerCapture(1);
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]);
  assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 2); assert.equal(app.get('throttle').textContent, '速度 100%');
  app.get('touch-dribbler').releasePointerCapture(3);
  assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  const count = socket.messages.length; app.pointer('move-stick', 'pointermove', 2, 164, 100); app.pointer('turn-stick', 'pointerup', 1);
  assert.equal(socket.messages.length, count);
});

test('failed pointer capture never accepts motion or an action press', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.get('move-stick').setPointerCapture = () => { throw new Error('inactive pointer'); };
  app.get('touch-kick').setPointerCapture = () => { throw new Error('inactive pointer'); };
  app.pointer('move-stick', 'pointerdown', 1, 164, 100); app.pointer('touch-kick', 'pointerdown', 2); app.tick(20);
  const state = decodeBrowser(socket.messages.at(-1)); assert.deepEqual(state.axes, [0, 0, 0, 0]);
  assert.equal(state.buttons, 0); assert.equal(state.counters[0], 0);
});

test('stick blur, hidden and pagehide send neutral and stop heartbeats, retaining throttle only', () => {
  for (const loss of ['blur', 'hidden', 'pagehide']) {
    const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
    app.pointer('turn-stick', 'pointerdown', 1, 164, 100); app.pointer('turn-stick', 'pointermove', 1, 164, 36); app.pointer('move-stick', 'pointerdown', 2, 164, 100);
    app.pointer('touch-kick', 'pointerdown', 3);
    if (loss === 'blur') app.blur();
    else if (loss === 'hidden') { app.document.visibilityState = 'hidden'; app.document.dispatchEvent(new Event('visibilitychange')); }
    else app.window.dispatchEvent(new Event('pagehide'));
    const state = decodeBrowser(socket.messages.at(-1));
    assert.equal(state.focused, false); assert.deepEqual(state.axes, [0, 0, 0, 0]); assert.equal(state.buttons, 0); assert.equal(state.lt, 0);
    assert.equal(app.get('throttle').textContent, '速度 100%'); assert.equal(app.get('move-stick').hasPointerCapture(2), false);
    const count = socket.messages.length; app.tick(100); assert.equal(socket.messages.length, count);
    app.document.visibilityState = 'visible'; app.focus();
    assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
    app.pointer('move-stick', 'pointermove', 2, 164, 100); // A stale held finger cannot resume.
    assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]);
  }
});

test('mode switch sends unfocused neutral, pauses until new input, and keeps throttle/counters', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.pointer('turn-stick', 'pointerdown', 1, 164, 100); app.pointer('turn-stick', 'pointermove', 1, 164, 36); app.pointer('move-stick', 'pointerdown', 2, 164, 100);
  app.pointer('touch-kick', 'pointerdown', 3); app.pointer('touch-kick', 'pointerup', 3);
  app.click('mode-keyboard');
  let state = decodeBrowser(socket.messages.at(-1)); assert.equal(state.focused, false);
  assert.deepEqual(state.axes, [0, 0, 0, 0]); assert.equal(state.buttons, 0); assert.equal(state.counters[0], 1);
  const count = socket.messages.length; app.tick(20); assert.equal(socket.messages.length, count);
  assert.equal(app.storage.get('gc-rc-input-mode'), 'keyboard');
  app.key('keydown', 'Space'); app.key('keyup', 'Space'); app.key('keydown', 'KeyW'); app.key('keydown', 'KeyC');
  state = decodeBrowser(socket.messages.at(-1)); assert.equal(state.axes[1], 1000); assert.equal(state.lt, 255); assert.equal(state.counters[0], 2);
  app.click('mode-stick'); assert.equal(app.get('throttle').textContent, '速度 100%');
  assert.equal(decodeBrowser(socket.messages.at(-1)).lt, 0); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  app.pointer('move-stick', 'pointermove', 2, 164, 100); assert.equal(decodeBrowser(socket.messages.at(-1)).axes[0], 0);
  app.pointer('move-stick', 'pointerdown', 4, 164, 100); assert.equal(decodeBrowser(socket.messages.at(-1)).axes[0], 1000);
});

test('touch stop sends Back edges once and touch toggles show robot telemetry rather than local guesses', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.pointer('touch-stop', 'pointerdown', 1); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 64);
  app.pointer('touch-stop', 'pointerup', 1); app.click('touch-stop', 1); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  const status = { owner: 'browser', operatorStop: false, robotLink: true, handover: false, flags: 3, fault: 0,
    heading: 12, targetHeading: 0, commandFlags: 0, pairingSeconds: 0, gamepadConnected: false, firmware: '0.1.0' };
  const publish = () => socket.dispatchEvent(Object.assign(new Event('message'), { data: JSON.stringify(status) }));
  publish(); assert.equal(app.get('touch-line-state').textContent, '無効');
  app.pointer('touch-line', 'pointerdown', 2); app.pointer('touch-line', 'pointerup', 2);
  assert.equal(app.get('touch-line-state').textContent, '無効');
  status.flags |= 1024 | 128; publish();
  assert.equal(app.get('touch-line-state').textContent, '有効'); assert.equal(app.get('touch-dribbler-state').textContent, '有効');
  assert.equal(app.get('touch-line').getAttribute('aria-pressed'), 'true');
  assert.equal(app.get('stick-heading').textContent, '12.0°');
  app.tick(701); assert.equal(app.get('touch-line-state').textContent, '—'); assert.equal(app.get('stick-heading').textContent, '—');
});

test('left stick takes its final release throttle; capture loss retains the last reported throttle', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.pointer('turn-stick', 'pointerdown', 1, 164, 100);
  app.pointer('turn-stick', 'pointerup', 1, 164, 36);
  assert.equal(app.get('throttle').textContent, '速度 100%');
  assert.equal(decodeBrowser(socket.messages.at(-1)).axes[2], 0); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  app.pointer('turn-stick', 'pointerdown', 2, 164, 68);
  assert.equal(app.get('throttle').textContent, '速度 100%');
  app.pointer('turn-stick', 'pointermove', 2, 164, 100);
  app.pointer('turn-stick', 'pointercancel', 2, 0, 0);
  assert.equal(app.get('throttle').textContent, '速度 75%');
});

test('viewport resize clears captured motion and actions without changing throttle', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.pointer('turn-stick', 'pointerdown', 1, 164, 100); app.pointer('turn-stick', 'pointermove', 1, 164, 36); app.pointer('move-stick', 'pointerdown', 2, 164, 100);
  app.pointer('touch-kick', 'pointerdown', 3); app.window.dispatchEvent(new Event('resize'));
  const state = decodeBrowser(socket.messages.at(-1)); assert.deepEqual(state.axes, [0, 0, 0, 0]); assert.equal(state.buttons, 0);
  assert.equal(app.get('throttle').textContent, '速度 100%');
  app.pointer('move-stick', 'pointermove', 2, 164, 100); assert.equal(decodeBrowser(socket.messages.at(-1)).axes[0], 0);
});

test('returning from pagehide via pageshow resumes neutral heartbeats without restoring held input', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.pointer('turn-stick', 'pointerdown', 1, 164, 100); app.pointer('turn-stick', 'pointermove', 1, 164, 36); app.pointer('move-stick', 'pointerdown', 2, 164, 100);
  app.window.dispatchEvent(new Event('pagehide')); const count = socket.messages.length;
  app.tick(20); assert.equal(socket.messages.length, count);
  app.window.dispatchEvent(new Event('pageshow'));
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  assert.equal(app.get('throttle').textContent, '速度 100%'); app.tick(20); assert.equal(socket.messages.length, count + 2);
});

test('left touch and horizontal slide retain throttle and knob height; final release applies vertical delta', () => {
  const app = page({ coarse: true }), socket = app.Socket.instances[0]; socket.open();
  app.pointer('move-stick', 'pointerdown', 1, 164, 100);
  app.pointer('turn-stick', 'pointerdown', 2, 132, 132); // Touch below the current knob.
  assert.equal(app.get('throttle').textContent, '速度 50%');
  assert.equal(app.get('turn-knob').style.transform, 'translate(32px, 0px)');
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [500, 0, 500, 0]);
  app.pointer('turn-stick', 'pointermove', 2, 68, 132);
  assert.equal(app.get('throttle').textContent, '速度 50%');
  assert.equal(app.get('turn-knob').style.transform, 'translate(-32px, 0px)');
  app.pointer('turn-stick', 'pointerup', 2, 68, 100);
  assert.equal(app.get('throttle').textContent, '速度 75%');
  assert.equal(app.get('turn-knob').style.transform, 'translate(0px, -32px)');
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [750, 0, 0, 0]);
});

test('focus overlay uses the current mode text on initial selection and every mode switch', () => {
  const app = page({ coarse: true });
  assert.equal(app.get('focus').textContent, 'タップして操作'); app.blur(); assert.equal(app.get('focus-overlay').hidden, false);
  app.click('mode-keyboard'); assert.equal(app.get('focus').textContent, 'クリックして操作');
  app.click('mode-stick'); assert.equal(app.get('focus').textContent, 'タップして操作');
  assert.equal(page({ savedMode: 'keyboard', coarse: true }).get('focus').textContent, 'クリックして操作');
});

test('gamepad connection auto-switches with neutral frames, persists selection, and polls at 50Hz', () => {
  const pad = standardPad(), app = page({ pads: [pad] }), socket = app.Socket.instances[0]; socket.open();
  app.key('keydown', 'KeyW'); app.tick(20); assert.equal(app.polls, 0);
  app.padEvent('gamepadconnected', pad);
  assert.equal(app.document.body.dataset.mode, 'controller'); assert.equal(app.storage.get('gc-rc-input-mode'), 'controller');
  assert.equal(app.get('controller-panel').hidden, false); assert.equal(app.get('stick-panel').hidden, true);
  assert.deepEqual(decodeBrowser(socket.messages.at(-2)).axes, [0, 0, 0, 0]); assert.equal(decodeBrowser(socket.messages.at(-2)).focused, false);
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]);
  app.tick(20); assert.equal(app.polls, 1); assert.equal(app.get('controller-status').textContent, 'コントローラー: PXN Switch Pro');
  pad.axes = [-.5, -.25, .75, 1]; padButton(pad, 0); padButton(pad, 5); pad.buttons[6] = { pressed: false, value: .25 };
  app.tick(20); const state = decodeBrowser(socket.messages.at(-1));
  assert.deepEqual(state.axes, [-500, 250, 750, 0]); assert.equal(state.buttons, 33); assert.equal(state.lt, 64); assert.equal(state.counters[0], 1);
  assert.equal(app.polls, 2);
});

test('controller hidden/pagehide immediately neutralize, stop polling, and resume visibly without replaying held inputs', () => {
  for (const loss of ['hidden', 'pagehide']) {
    const pad = standardPad(), app = page({ savedMode: 'controller', pads: [pad] }), socket = app.Socket.instances[0]; socket.open(); app.tick(20);
    pad.axes[0] = 1; for (const index of [0, 1, 2, 3, 5, 6, 8]) padButton(pad, index); app.tick(20);
    app.blur(); // Controller visibility recovery must also work while the window remains unfocused.
    if (loss === 'hidden') { app.document.visibilityState = 'hidden'; app.document.dispatchEvent(new Event('visibilitychange')); }
    else app.window.dispatchEvent(new Event('pagehide'));
    let state = decodeBrowser(socket.messages.at(-1)); assert.deepEqual(state.axes, [0, 0, 0, 0]); assert.equal(state.buttons, 0); assert.equal(state.lt, 0);
    assert.equal(state.focused, false); const count = socket.messages.length, polls = app.polls;
    app.tick(100); assert.equal(socket.messages.length, count); assert.equal(app.polls, polls);
    assert.equal(app.get('focus-overlay').hidden, true);
    if (loss === 'hidden') { app.document.visibilityState = 'visible'; app.document.dispatchEvent(new Event('visibilitychange')); }
    else app.window.dispatchEvent(new Event('pageshow'));
    app.tick(20); assert.equal(app.document.hasFocus(), false);
    state = decodeBrowser(socket.messages.at(-1)); assert.deepEqual(state.axes, [0, 0, 0, 0]); assert.equal(state.buttons, 0); assert.equal(state.lt, 0);
    assert.deepEqual(state.counters, [1, 1, 1, 1]); assert.equal(app.get('controller-hint').hidden, false);
    pad.axes[0] = 0; for (const index of [0, 1, 2, 3, 5, 6, 8]) padButton(pad, index, false); app.tick(20);
    padButton(pad, 0); app.tick(20); assert.equal(decodeBrowser(socket.messages.at(-1)).counters[0], 2);
  }
});

test('controller disconnect event sends immediate neutral even behind a buffered heartbeat and stays in mode', () => {
  const pad = standardPad(), app = page({ savedMode: 'controller', pads: [pad] }), socket = app.Socket.instances[0]; socket.open(); app.tick(20);
  pad.axes[0] = 1; padButton(pad, 5); app.tick(20); socket.bufferedAmount = 26;
  const count = socket.messages.length; app.setPads([]); app.padEvent('gamepaddisconnected', pad);
  assert.equal(socket.messages.length, count + 1); const state = decodeBrowser(socket.messages.at(-1));
  assert.deepEqual(state.axes, [0, 0, 0, 0]); assert.equal(state.buttons, 0); assert.equal(app.document.body.dataset.mode, 'controller');
  assert.equal(app.get('controller-status').textContent, 'コントローラーが切れました');
});

test('controller disappearance without an event also forces neutral past buffered heartbeats', () => {
  const pad = standardPad(), app = page({ savedMode: 'controller', pads: [pad] }), socket = app.Socket.instances[0]; socket.open(); app.tick(20);
  pad.axes[2] = 1; padButton(pad, 8); app.tick(20); socket.bufferedAmount = 26;
  const count = socket.messages.length; app.setPads([null]); app.tick(20);
  assert.equal(socket.messages.length, count + 1); assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]);
  assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0); assert.equal(app.get('controller-status').textContent, 'コントローラーが切れました');
});

test('controller panel waits for exposure, rejects non-standard pads, and handles a denied/missing API', () => {
  const app = page({ savedMode: 'controller' }), socket = app.Socket.instances[0]; socket.open(); app.tick(20);
  assert.equal(app.get('controller-status').textContent, 'コントローラーのボタンを押してください');
  app.setPads([standardPad({ mapping: '' })]); app.tick(20);
  assert.equal(app.get('controller-status').textContent, '標準配置のコントローラーを接続してください');
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  delete app.window.navigator.getGamepads; app.tick(20);
  assert.equal(app.get('controller-status').textContent, 'このブラウザはコントローラーに対応していません');
  const denied = page({ savedMode: 'controller', gamepadsThrows: true }), deniedSocket = denied.Socket.instances[0]; deniedSocket.open(); denied.tick(20);
  assert.equal(denied.get('controller-status').textContent, 'コントローラーを読み取れません');
  assert.deepEqual(decodeBrowser(deniedSocket.messages.at(-1)).axes, [0, 0, 0, 0]);
});

test('switching away from controller clears buttons and stops Gamepad API polling', () => {
  const pad = standardPad(), app = page({ savedMode: 'controller', pads: [pad] }), socket = app.Socket.instances[0]; socket.open(); app.tick(20);
  pad.axes[0] = 1; padButton(pad, 0); app.tick(20); app.click('mode-stick');
  assert.equal(decodeBrowser(socket.messages.at(-1)).focused, false); assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]);
  const polls = app.polls, count = socket.messages.length; app.tick(20); assert.equal(app.polls, polls); assert.equal(socket.messages.length, count);
  app.click('mode-controller'); app.tick(20); assert.equal(decodeBrowser(socket.messages.at(-1)).counters[0], 1);
  assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0); assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]);
});

test('big stop button works in controller mode and a still-held physical Back cannot toggle it again', () => {
  const pad = standardPad(), app = page({ savedMode: 'controller', pads: [pad] }), socket = app.Socket.instances[0]; socket.open(); app.tick(20);
  padButton(pad, 8); app.tick(20); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 64);
  app.click('stop'); assert.equal(decodeBrowser(socket.messages.at(-3)).buttons, 0);
  assert.equal(decodeBrowser(socket.messages.at(-2)).buttons, 64); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  app.tick(20); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
  padButton(pad, 8, false); app.tick(20); padButton(pad, 8); app.tick(20); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 64);
});

test('gamepad connection while hidden changes mode safely without polling or resuming control', () => {
  const pad = standardPad(), app = page({ pads: [pad] }), socket = app.Socket.instances[0]; socket.open();
  app.document.visibilityState = 'hidden'; app.document.dispatchEvent(new Event('visibilitychange'));
  app.padEvent('gamepadconnected', pad); const count = socket.messages.length; app.tick(100);
  assert.equal(app.polls, 0); assert.equal(socket.messages.length, count); assert.equal(app.document.body.dataset.mode, 'controller');
  assert.equal(decodeBrowser(socket.messages.at(-1)).focused, false); assert.equal(decodeBrowser(socket.messages.at(-1)).buttons, 0);
});

test('visible controller mode polls and sends immediately after open without window focus or an overlay click', () => {
  const pad = standardPad({ id: 'Xbox 360 Controller (XInput STANDARD GAMEPAD)' });
  const app = page({ savedMode: 'controller', pads: [pad], initialFocus: false }), socket = app.Socket.instances[0];
  assert.equal(app.get('focus-overlay').hidden, true); socket.open(); app.tick(20);
  assert.equal(app.polls, 1); assert.equal(app.document.hasFocus(), false);
  assert.equal(app.get('controller-status').textContent, `コントローラー: ${pad.id}`);
  pad.axes = [.5, -.5, 1, 0]; padButton(pad, 0); app.tick(20);
  const state = decodeBrowser(socket.messages.at(-1)); assert.equal(state.focused, true);
  assert.deepEqual(state.axes, [500, 500, 1000, 0]); assert.equal(state.counters[0], 1);
  assert.equal(app.get('focus-overlay').hidden, true);
});

test('controller blur/focus preserves current motion, held buttons and release latches', () => {
  const pad = standardPad(), app = page({ savedMode: 'controller', pads: [pad] }), socket = app.Socket.instances[0]; socket.open(); app.tick(20);
  pad.axes[0] = 1; padButton(pad, 0); app.tick(20);
  const count = socket.messages.length, polls = app.polls; app.blur();
  assert.equal(socket.messages.length, count); assert.equal(app.get('focus-overlay').hidden, true);
  app.tick(20); assert.equal(app.polls, polls + 1);
  let state = decodeBrowser(socket.messages.at(-1)); assert.deepEqual(state.axes, [1000, 0, 0, 0]);
  assert.equal(state.buttons, 1); assert.equal(state.counters[0], 1); assert.equal(state.focused, true);
  app.focus(); assert.equal(socket.messages.length, count + 1); app.tick(20);
  state = decodeBrowser(socket.messages.at(-1)); assert.deepEqual(state.axes, [1000, 0, 0, 0]);
  assert.equal(state.buttons, 1); assert.equal(state.counters[0], 1);
});

test('controller close stops polling; reconnect automatically resumes neutral with release latches while unfocused', () => {
  const pad = standardPad(), app = page({ savedMode: 'controller', pads: [pad], initialFocus: false }), socket = app.Socket.instances[0]; socket.open(); app.tick(20);
  pad.axes[0] = 1; padButton(pad, 0); padButton(pad, 5); app.tick(20); socket.close();
  assert.equal(app.get('focus-overlay').hidden, true);
  const count = socket.messages.length, polls = app.polls;
  app.document.dispatchEvent(new Event('visibilitychange')); app.window.dispatchEvent(new Event('focus')); app.tick(100);
  assert.equal(socket.messages.length, count); assert.equal(app.polls, polls);
  [...app.timeouts.values()][0].callback(); const next = app.Socket.instances[1]; next.open();
  let state = decodeBrowser(next.messages[0]); assert.equal(state.focused, true); assert.equal(state.sequence, 0);
  assert.deepEqual(state.axes, [0, 0, 0, 0]); assert.equal(state.buttons, 0);
  app.tick(20); state = decodeBrowser(next.messages.at(-1)); assert.deepEqual(state.axes, [0, 0, 0, 0]);
  assert.equal(state.buttons, 0); assert.equal(state.counters[0], 0); assert.equal(app.get('focus-overlay').hidden, true);
  pad.axes = [.08, .05, -.2, 0]; padButton(pad, 0, false); padButton(pad, 5, false); app.tick(20);
  assert.equal(app.get('controller-hint').hidden, true); // Claude's deadzone-based center latch is retained.
  pad.axes[0] = 1; padButton(pad, 0); app.tick(20); state = decodeBrowser(next.messages.at(-1));
  assert.equal(state.axes[0], 1000); assert.equal(state.counters[0], 1); assert.equal(app.document.hasFocus(), false);
});

test('controller pagehide cannot be undone by focus/visibility events before pageshow', () => {
  const pad = standardPad(), app = page({ savedMode: 'controller', pads: [pad] }), socket = app.Socket.instances[0]; socket.open(); app.tick(20);
  pad.axes[0] = 1; app.tick(20); app.window.dispatchEvent(new Event('pagehide'));
  assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]);
  app.window.dispatchEvent(new Event('focus')); app.document.dispatchEvent(new Event('visibilitychange'));
  const count = socket.messages.length, polls = app.polls; app.tick(100);
  assert.equal(app.polls, polls); assert.equal(socket.messages.length, count); assert.equal(app.get('focus-overlay').hidden, true);
  app.window.dispatchEvent(new Event('pageshow')); app.tick(20);
  assert.equal(app.polls, polls + 1); assert.deepEqual(decodeBrowser(socket.messages.at(-1)).axes, [0, 0, 0, 0]);
});

test('switching to controller while unfocused resumes visibly; keyboard and stick still require focus', () => {
  for (const mode of ['keyboard', 'stick']) {
    const pad = standardPad(), app = page({ savedMode: mode, pads: [pad], initialFocus: false }), socket = app.Socket.instances[0]; socket.open();
    assert.equal(app.get('focus-overlay').hidden, false); const count = socket.messages.length; app.tick(20);
    assert.equal(socket.messages.length, count); assert.equal(app.polls, 0);
    app.click('mode-controller'); app.tick(20); assert.equal(app.polls, 1); assert.equal(app.get('focus-overlay').hidden, true);
    app.click(`mode-${mode}`); assert.equal(decodeBrowser(socket.messages.at(-1)).focused, false);
    assert.equal(app.get('focus-overlay').hidden, false); const controllerPolls = app.polls; app.tick(20); assert.equal(app.polls, controllerPolls);
  }
});

test('controller wake lock follows visibility rather than window focus', async () => {
  let requests = 0, releases = 0;
  const lock = Object.assign(new EventTarget(), { release: async () => { releases++; } });
  const app = page({ savedMode: 'controller', initialFocus: false, wakeLock: { request: async () => { requests++; return lock; } } });
  app.Socket.instances[0].open(); await Promise.resolve(); assert.equal(requests, 1);
  app.blur(); await Promise.resolve(); assert.equal(releases, 0);
  app.document.visibilityState = 'hidden'; app.document.dispatchEvent(new Event('visibilitychange'));
  await Promise.resolve(); assert.equal(releases, 1);
});

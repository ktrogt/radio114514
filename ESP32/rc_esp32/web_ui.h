// GENERATED — do not edit; source: RC/ESP32/web/ (node RC/tools/sync_esp32.mjs)
#pragma once
#include <pgmspace.h>

static const char WEB_INDEX[] PROGMEM = R"RC_ASSET(<!doctype html>
<html lang="ja">
<head>
  <meta charset="utf-8">
  <meta id="viewport" name="viewport" content="width=device-width, initial-scale=1">
  <meta name="rc-websocket-port" content="81">
  <title>GC RC 操作</title>
  <link rel="stylesheet" href="/style.css">
  <script type="module" src="/controls.js"></script>
</head>
<body data-mode="keyboard">
  <main>
    <header>
      <h1>GC RC 操作</h1>
      <div class="segments modes" role="group" aria-label="操作方法">
        <button id="mode-keyboard" type="button" aria-pressed="true">キーボード</button>
        <button id="mode-stick" type="button" aria-pressed="false">スティック</button>
        <button id="mode-controller" type="button" aria-pressed="false">コントローラー</button>
      </div>
      <p id="connection" role="status">ロボット未接続 · WebSocket 再接続中</p>
      <p class="stick-heading">向き <strong id="stick-heading">—</strong></p>
    </header>
    <section class="robot-state" aria-label="操作状態">
      <div id="badge" class="badge idle" role="status">接続待ち</div>
      <p id="fault" class="fault" role="alert" hidden></p>
      <p id="busy" class="fault" hidden>別のブラウザーが接続中です</p>
      <button id="stop" type="button">停止 / 解除 <kbd>Esc</kbd></button>
    </section>
    <p id="pairing" class="pairing" hidden></p>
    <p class="portrait-hint">横向きがおすすめです</p>
    <section id="stick-panel" class="stick-panel" aria-label="スティック操作" hidden>
      <div class="stick-control turn-control">
        <div id="turn-stick" class="stick-base throttle-base" aria-label="速度・旋回スティック">
          <span class="throttle-track" aria-hidden="true"></span>
          <span id="turn-knob" class="stick-knob" aria-hidden="true"></span>
        </div>
        <strong class="stick-label">速度・旋回</strong>
        <span id="throttle" class="throttle-value">速度 50%</span>
      </div>
      <div class="touch-actions">
        <button id="touch-kick" class="touch-button" type="button">キック</button>
        <button id="touch-dribbler" class="touch-button" type="button" aria-pressed="false">ドリブラー <small id="touch-dribbler-state">—</small></button>
        <button id="touch-reset" class="touch-button" type="button">正面リセット</button>
        <button id="touch-line" class="touch-button" type="button" aria-pressed="false">ライン補助 <small id="touch-line-state">—</small></button>
        <button id="touch-stop" class="touch-button danger" type="button">停止</button>
      </div>
      <div class="stick-control move-control">
        <div id="move-stick" class="stick-base" aria-label="移動スティック">
          <span id="move-knob" class="stick-knob" aria-hidden="true"></span>
        </div>
        <strong class="stick-label">移動</strong>
      </div>
    </section>
    <section id="controller-panel" class="controller-panel" aria-label="コントローラー操作" hidden>
      <p id="controller-status" role="status">コントローラーのボタンを押してください</p>
      <p id="controller-hint" hidden>スティックを中央に戻してください</p>
      <table><thead><tr><th>操作</th><th>コントローラー</th></tr></thead><tbody>
        <tr><td>移動</td><td>左スティック</td></tr>
        <tr><td>向き</td><td>右スティックの左右</td></tr>
        <tr><td>キック</td><td>下のボタン（Switch: B / Xbox: A）か ZR / RT</td></tr>
        <tr><td>ドリブラー</td><td>右のボタン（Switch: A / Xbox: B）</td></tr>
        <tr><td>ライン補助</td><td>左のボタン（Switch: Y / Xbox: X）</td></tr>
        <tr><td>正面リセット</td><td>上のボタン（Switch: X / Xbox: Y）</td></tr>
        <tr><td>ブースト</td><td>R / RB（押している間）</td></tr>
        <tr><td>低速</td><td>ZL / LT（押している間）</td></tr>
        <tr><td>停止 / 解除</td><td>− / View</td></tr>
      </tbody></table>
    </section>
    <div class="details">
    <div>
    <dl class="status">
      <div><dt>操作元</dt><dd id="owner">なし</dd></div>
      <div><dt>向き</dt><dd id="heading">—</dd></div>
      <div><dt>ライン補助</dt><dd id="line">—</dd></div>
      <div><dt>ドリブラー</dt><dd id="dribbler">無効</dd></div>
      <div><dt>速度モード</dt><dd id="speed">通常</dd></div>
      <div><dt>キック</dt><dd id="kick">待機</dd></div>
    </dl>
    </div>
    <section aria-label="キー操作">
      <table><thead><tr><th>操作</th><th>キー</th></tr></thead><tbody>
        <tr><td>移動</td><td><kbd>W</kbd> 前 / <kbd>S</kbd> 後 / <kbd>A</kbd> 左 / <kbd>D</kbd> 右</td></tr>
        <tr><td>旋回</td><td><kbd>Q</kbd> 左 / <kbd>E</kbd> 右</td></tr>
        <tr><td>キック</td><td><kbd>Space</kbd></td></tr>
        <tr><td>ドリブラー 有効 / 無効</td><td><kbd>F</kbd></td></tr>
        <tr><td>正面リセット</td><td><kbd>R</kbd>（移動・旋回キーを離す）</td></tr>
        <tr><td>ライン補助 有効 / 無効</td><td><kbd>G</kbd></td></tr>
        <tr><td>ブースト / 低速</td><td><kbd>Shift</kbd> / <kbd>C</kbd>（低速優先）</td></tr>
        <tr><td>停止 / 解除</td><td><kbd>Esc</kbd></td></tr>
      </tbody></table>
    </section>
    </div>
    <footer>F446 <span id="firmware">—</span> · ゲームパッドがつながっているときはゲームパッドが優先されます</footer>
  </main>
  <div id="focus-overlay" class="overlay" hidden><button id="focus" type="button">クリックして操作</button></div>
</body>
</html>
)RC_ASSET";

static const char WEB_STYLE[] PROGMEM = R"RC_ASSET(:root { color-scheme: light; font-family: system-ui, -apple-system, "Segoe UI", "Noto Sans JP", sans-serif; color: #20262b; background: #f5f6f7; font-size: 19px; }
* { box-sizing: border-box; }
body { margin: 0; }
main { max-width: 1080px; margin: auto; padding: 22px 28px; }
h1 { margin: 0; font-size: 1.35rem; }
header { display: grid; grid-template-columns: 1fr auto; align-items: center; gap: 8px; }
header p { grid-column: 1 / -1; margin: 0 0 10px; font-size: .9rem; }
.badge { padding: 18px 22px; border-radius: 6px; font-size: clamp(1.25rem, 3.5vw, 1.85rem); font-weight: 750; }
.idle { background: #e3e6e8; color: #404b53; }
.allowed { background: #dcefe1; color: #195d31; }
.stopped, .fault { background: #f9e1e0; color: #902d27; }
.fault { padding: 10px 14px; margin: 10px 0; border-radius: 4px; }
button { min-height: 48px; font: inherit; font-weight: 650; cursor: pointer; border: 2px solid #2869a0; border-radius: 5px; padding: 12px 22px; color: #174d79; background: white; }
button:focus-visible { outline: 3px solid #2869a0; outline-offset: 3px; }
#stop { display: block; width: 100%; margin-top: 12px; }
.segments { display: flex; }
.segments button { border-radius: 0; padding: 8px 12px; }
.segments button:first-child { border-radius: 5px 0 0 5px; }
.segments button + button { border-left: 0; }
.segments button:last-child { border-radius: 0 5px 5px 0; }
.segments button[aria-pressed="true"] { background: #2869a0; color: white; }
.modes { font-size: .8rem; }
.stick-heading, .portrait-hint { display: none; }
.details { display: grid; grid-template-columns: 1fr 1.5fr; gap: 26px; margin-top: 22px; }
.status { margin: 0; }
.status div { display: grid; grid-template-columns: 135px 1fr; padding: 9px 0; border-bottom: 1px solid #d4dade; }
dt { color: #46545e; } dd { margin: 0; font-weight: 650; }
.pairing { padding: 12px; border: 1px solid #a9becd; }
table { width: 100%; border-collapse: collapse; font-size: .9rem; }
th, td { text-align: left; padding: 8px 10px; border-bottom: 1px solid #d4dade; }
th { background: #e8edf1; }
kbd { display: inline-block; font: inherit; font-weight: 650; border: 1px solid #bbc5cd; border-radius: 3px; padding: 0 5px; background: #fff; }
footer { margin-top: 18px; font-size: .75rem; color: #53636f; }
.overlay { position: fixed; inset: 0; display: grid; place-items: center; background: #f5f6f7e8; }
.overlay button { font-size: 1.6rem; padding: 25px; }
[hidden] { display: none !important; }
@media (max-width: 600px) { :root { font-size: 17px; } main { padding: 15px; } .status div { grid-template-columns: 130px 1fr; } th, td { padding: 7px 4px; } }
@media (max-width: 900px) { .details { grid-template-columns: 1fr; gap: 22px; } }

/* Touch controls occupy the viewport; keyboard mode keeps its normal document layout. */
html[data-mode="stick"] { height: 100%; overflow: hidden; touch-action: none; overscroll-behavior: none; }
body[data-mode="stick"] { height: 100vh; height: 100dvh; overflow: hidden; touch-action: none; overscroll-behavior: none; user-select: none; -webkit-user-select: none; -webkit-touch-callout: none; }
body[data-mode="stick"] main { max-width: none; height: 100%; padding: max(8px, env(safe-area-inset-top)) max(10px, env(safe-area-inset-right)) max(8px, env(safe-area-inset-bottom)) max(10px, env(safe-area-inset-left)); display: flex; flex-direction: column; gap: 6px; }
body[data-mode="stick"] .details, body[data-mode="stick"] footer, body[data-mode="stick"] #stop { display: none; }
body[data-mode="stick"] header { flex: none; grid-template-columns: 1fr auto; gap: 2px 8px; }
body[data-mode="stick"] h1 { display: none; }
body[data-mode="stick"] .modes { grid-column: 2; grid-row: 1 / 3; }
body[data-mode="stick"] header p { grid-column: 1; margin: 0; font-size: .7rem; }
body[data-mode="stick"] .stick-heading { display: block; }
body[data-mode="stick"] .badge { padding: 5px 10px; font-size: clamp(16px, 2.5vw, 22px); }
body[data-mode="stick"] .fault, body[data-mode="stick"] .pairing { margin: 4px 0 0; padding: 4px 8px; font-size: 12px; overflow-wrap: anywhere; }
.stick-panel { flex: 1; min-height: 0; display: grid; grid-template-columns: minmax(140px, 1fr) minmax(200px, .85fr) minmax(140px, 1fr); align-items: center; gap: 10px; }
.stick-control { display: flex; flex-direction: column; align-items: center; gap: 4px; }
.turn-control { justify-self: start; }
.move-control { justify-self: end; }
.stick-base { width: max(140px, 40vmin); height: max(140px, 40vmin); width: max(140px, 40dvmin); height: max(140px, 40dvmin); position: relative; border: 2px solid #a9becd; border-radius: 50%; background: #e8edf1; touch-action: none; }
.stick-knob { position: absolute; width: 36%; height: 36%; top: 32%; left: 32%; border-radius: 50%; border: 2px solid #2869a0; background: #fff; pointer-events: none; }
.throttle-track { position: absolute; top: 18%; bottom: 18%; left: 50%; width: 2px; background: #a9becd; pointer-events: none; }
.stick-label { font-size: 16px; }
.throttle-value { font-size: 15px; font-variant-numeric: tabular-nums; }
.touch-actions { display: grid; grid-template-columns: 1fr 1fr; gap: 6px; width: 100%; max-width: 300px; justify-self: center; }
.touch-button { min-height: 48px; padding: 6px 8px; font-size: 15px; }
.touch-button small { display: block; font-size: 12px; font-weight: 500; }
#touch-kick { grid-column: 1 / -1; min-height: 66px; font-size: 23px; }
#touch-line { grid-column: 1 / -1; }
.danger { grid-column: 1 / -1; min-height: 58px; background: #f9e1e0; border-color: #902d27; color: #902d27; font-size: 22px; }
.touch-button[aria-pressed="true"], .touch-button.pressed { background: #e8edf1; }
.danger.pressed { background: #902d27; color: white; }
@media (orientation: portrait) {
  body[data-mode="stick"] .portrait-hint { display: block; margin: 0; text-align: center; font-size: 12px; color: #53636f; }
  .stick-panel { grid-template-columns: 1fr 1fr; grid-template-rows: auto 1fr; align-content: end; gap: 12px 8px; }
  .touch-actions { grid-column: 1 / -1; grid-row: 1; max-width: 480px; grid-template-columns: 1fr 1fr; }
  #touch-kick { grid-column: 1; grid-row: 1 / 3; min-height: 108px; }
  #touch-dribbler { grid-column: 2; grid-row: 1; }
  #touch-reset { grid-column: 2; grid-row: 2; }
  #touch-line { grid-column: 1; }
  #touch-stop { grid-column: 2; }
  .turn-control { grid-column: 1; grid-row: 2; align-self: end; }
  .move-control { grid-column: 2; grid-row: 2; align-self: end; padding-bottom: 23px; }
}
@media (orientation: landscape) and (max-height: 420px) {
  body[data-mode="stick"] main { gap: 4px; }
  body[data-mode="stick"] header { grid-template-columns: 1fr auto auto; align-items: center; }
  body[data-mode="stick"] .modes { grid-column: 3; grid-row: 1; }
  body[data-mode="stick"] header .stick-heading { grid-column: 2; }
  .stick-panel { gap: 8px; }
  #touch-kick { min-height: 58px; }
  .danger { min-height: 52px; }
}
@media (orientation: landscape) and (max-height: 340px) {
  .touch-actions { gap: 4px; }
  #touch-kick { min-height: 56px; }
  #touch-line { grid-column: 1; }
  #touch-stop { grid-column: 2; min-height: 48px; }
}

body[data-mode="controller"] { height: 100vh; height: 100dvh; overflow: hidden; }
body[data-mode="controller"] main { height: 100%; padding: max(10px, env(safe-area-inset-top)) max(12px, env(safe-area-inset-right)) max(10px, env(safe-area-inset-bottom)) max(12px, env(safe-area-inset-left)); display: flex; flex-direction: column; gap: 8px; }
body[data-mode="controller"] h1, body[data-mode="controller"] .details, body[data-mode="controller"] footer, body[data-mode="controller"] #stop kbd { display: none; }
body[data-mode="controller"] header { flex: none; gap: 4px 8px; }
body[data-mode="controller"] .modes { grid-column: 1 / -1; grid-row: 1; }
body[data-mode="controller"] header p { font-size: .75rem; margin: 0; grid-column: 1; }
body[data-mode="controller"] .stick-heading { display: block; grid-column: 2; text-align: right; }
body[data-mode="controller"] .badge { padding: 8px 12px; font-size: 20px; }
body[data-mode="controller"] .robot-state { flex: none; }
body[data-mode="controller"] #stop { background: #f9e1e0; border-color: #902d27; color: #902d27; min-height: 56px; font-size: 22px; margin-top: 8px; }
body[data-mode="controller"] .fault, body[data-mode="controller"] .pairing { margin: 4px 0 0; padding: 5px 8px; font-size: 13px; overflow-wrap: anywhere; }
.controller-panel { min-height: 0; overflow-y: auto; overscroll-behavior: contain; }
.controller-panel p { margin: 4px 0 8px; overflow-wrap: anywhere; font-size: .85rem; }
#controller-hint { color: #46545e; }
.controller-panel table { font-size: .8rem; }
.controller-panel th, .controller-panel td { padding: 6px 8px; }
@media (max-width: 600px) {
  header .modes { grid-column: 1 / -1; width: 100%; }
  .modes button { flex: 1; padding: 8px 6px; font-size: 13px; }
  body[data-mode="stick"] header .modes { grid-column: 1 / -1; grid-row: 1; }
  body[data-mode="stick"] header #connection { grid-column: 1; grid-row: 2; }
  body[data-mode="stick"] header .stick-heading { grid-column: 2; grid-row: 2; text-align: right; }
}
@media (orientation: landscape) and (max-height: 420px) {
  body[data-mode="controller"] main { gap: 5px; }
  body[data-mode="controller"] header { grid-template-columns: 1fr auto auto; }
  body[data-mode="controller"] .modes { grid-column: 3; grid-row: 1; }
  body[data-mode="controller"] header #connection { grid-column: 1; grid-row: 1; }
  body[data-mode="controller"] header .stick-heading { grid-column: 2; grid-row: 1; }
  body[data-mode="controller"] .badge { padding: 5px 10px; font-size: 18px; }
}
)RC_ASSET";

static const char WEB_CONTROLS[] PROGMEM = R"RC_ASSET(export const BUTTON = Object.freeze({ A: 1, B: 2, X: 4, Y: 8, LB: 16, RB: 32, BACK: 64 });
export const MESSAGE_SIZE = 26;
const action = { Space: 'kick', KeyF: 'dribbler', KeyR: 'reset', KeyG: 'line' };
const supported = new Set(['KeyW', 'KeyA', 'KeyS', 'KeyD', 'KeyQ', 'KeyE', 'Space', 'KeyF', 'KeyR', 'KeyG', 'ShiftLeft', 'ShiftRight', 'KeyC', 'Escape']);
const counters = () => ({ kick: 0, dribbler: 0, reset: 0, line: 0 });
const modes = ['keyboard', 'stick', 'controller'];
export const MODE_STORAGE_KEY = 'gc-rc-input-mode';
export function defaultMode(coarse, storage) {
  try {
    const saved = storage?.getItem(MODE_STORAGE_KEY);
    if (modes.includes(saved)) return saved;
  } catch { /* Storage may be disabled on a phone or a private tab. */ }
  return coarse ? 'stick' : 'keyboard';
}
export function rememberMode(mode, storage) {
  try { storage?.setItem(MODE_STORAGE_KEY, mode); } catch { /* The controls also work without storage. */ }
}
// No deadzone here: the ESP32 applies the shared mapping to every source.
export function stickAxes(dx, dy, radius) {
  if (!(radius > 0) || !Number.isFinite(dx) || !Number.isFinite(dy)) return { x: 0, y: 0 };
  let x = dx / radius, y = -dy / radius;
  const length = Math.hypot(x, y);
  if (length > 1) { x /= length; y /= length; }
  return { x: Math.trunc(x * 1000) || 0, y: Math.trunc(y * 1000) || 0 };
}
export function throttleAxes(dx, dy, radius) {
  if (!(radius > 0) || !Number.isFinite(dx) || !Number.isFinite(dy)) return { x: 0, throttle: .5 };
  return { x: Math.round(Math.max(-1, Math.min(1, dx / radius)) * 1000) || 0,
    throttle: Math.max(0, Math.min(1, (.5 - dy / (2 * radius)))) };
}

export class KeyboardState {
  constructor(sharedCounters = counters()) {
    this.keys = new Set();
    this.focused = false;
    this.counters = sharedCounters;
  }
  keyDown(code, repeat = false) {
    if (!this.focused || !supported.has(code) || repeat || this.keys.has(code)) return false;
    this.keys.add(code);
    if (action[code]) this.counters[action[code]] = (this.counters[action[code]] + 1) & 0xffff;
    return true;
  }
  keyUp(code) { return this.keys.delete(code); }
  focus() { this.keys.clear(); this.focused = true; }
  blur() { this.keys.clear(); this.focused = false; }
  snapshot() {
    const has = code => this.focused && this.keys.has(code);
    return {
      focused: this.focused,
      lx: (Number(has('KeyD')) - Number(has('KeyA'))) * 1000,
      ly: (Number(has('KeyW')) - Number(has('KeyS'))) * 1000,
      rx: (Number(has('KeyE')) - Number(has('KeyQ'))) * 1000, ry: 0,
      lt: has('KeyC') ? 255 : 0, rt: 0,
      buttons: (has('Space') ? BUTTON.A : 0) | (has('KeyF') ? BUTTON.B : 0) |
        (has('KeyG') ? BUTTON.X : 0) | (has('KeyR') ? BUTTON.Y : 0) |
        (has('ShiftLeft') || has('ShiftRight') ? BUTTON.RB : 0) | (has('Escape') ? BUTTON.BACK : 0),
      ...this.counters,
    };
  }
}

const touchButtons = { kick: BUTTON.A, dribbler: BUTTON.B, line: BUTTON.X, reset: BUTTON.Y, stop: BUTTON.BACK };
export class StickState {
  constructor(sharedCounters = counters()) {
    this.counters = sharedCounters;
    this.focused = false;
    this.pointers = new Map();
    this.throttle = .5;
  }
  down(control, pointerId, x = 0, y = 0) {
    if (!this.focused || this.pointers.has(pointerId) ||
      [...this.pointers.values()].some(pointer => pointer.control === control)) return false;
    if (!['move', 'turn', ...Object.keys(touchButtons)].includes(control)) return false;
    this.pointers.set(pointerId, { control, x, y, y0: y, t0: this.throttle });
    if (control in this.counters) this.counters[control] = (this.counters[control] + 1) & 0xffff;
    return true;
  }
  move(pointerId, x, y) {
    const pointer = this.pointers.get(pointerId);
    if (!pointer || !['move', 'turn'].includes(pointer.control)) return false;
    pointer.x = x; pointer.y = y;
    if (pointer.control === 'turn') this.throttle = Math.max(0, Math.min(1, pointer.t0 + (y - pointer.y0)));
    return true;
  }
  release(pointerId) {
    const pointer = this.pointers.get(pointerId);
    if (!pointer) return false;
    this.pointers.delete(pointerId);
    return true;
  }
  clear() { this.pointers.clear(); }
  focus() { this.clear(); this.focused = true; }
  blur() { this.clear(); this.focused = false; }
  snapshot() {
    let lx = 0, ly = 0, rx = 0, buttons = 0;
    if (this.focused) for (const pointer of this.pointers.values()) {
      if (pointer.control === 'move') { lx = Math.round(pointer.x * this.throttle) || 0; ly = Math.round(pointer.y * this.throttle) || 0; }
      else if (pointer.control === 'turn') rx = pointer.x;
      else buttons |= touchButtons[pointer.control] ?? 0;
    }
    // RB is present only while motion is requested. Neutral must acquire/rearm without RB.
    if (lx || ly || rx) buttons |= BUTTON.RB;
    return { focused: this.focused, lx, ly, rx, ry: 0, buttons, lt: 0, rt: 0, ...this.counters };
  }
}

// [standard button indices, virtual bit, counter]. Kick on the bottom face button or ZR/RT
// (index 7; user, 2026-10-15); R/RB stays boost. The first index keys the release latch.
const controllerButtons = [[[0, 7], BUTTON.A, 'kick'], [[1], BUTTON.B, 'dribbler'], [[2], BUTTON.X, 'line'],
  [[3], BUTTON.Y, 'reset'], [[5], BUTTON.RB], [[8], BUTTON.BACK]];
const axisValue = value => Number.isFinite(value) ? Math.round(Math.max(-1, Math.min(1, value)) * 1000) || 0 : 0;
const triggerValue = value => Number.isFinite(value) ? Math.round(Math.max(0, Math.min(1, value)) * 255) : 0;
export class ControllerState {
  constructor(sharedCounters = counters()) {
    this.counters = sharedCounters; this.focused = false; this.id = null; this.index = null;
    this.seen = false; this.nonStandard = false; this.clear();
  }
  clear() {
    this.axes = [0, 0, 0]; this.buttons = 0; this.lt = 0;
    this.released = new Set(); this.motionReady = false;
  }
  focus() { this.clear(); this.focused = true; }
  blur() { this.clear(); this.focused = false; }
  disconnect() { this.clear(); this.id = null; this.index = null; }
  blockBackUntilRelease() { this.released.delete(8); this.buttons &= ~BUTTON.BACK; }
  update(gamepads) {
    if (!this.focused) return;
    const connected = Array.from(gamepads ?? []).filter(pad => pad?.connected);
    const pad = connected.find(pad => pad.mapping === 'standard');
    this.nonStandard = !pad && connected.length > 0;
    if (!pad) { this.disconnect(); return; }
    if (pad.index !== this.index || pad.id !== this.id) {
      this.clear(); this.index = pad.index; this.id = pad.id;
    }
    this.seen = true;
    // Left stick moves, right-stick X turns (a swap was tried and reverted on 2026-10-15).
    const axes = [axisValue(pad.axes?.[0]), -axisValue(pad.axes?.[1]) || 0, axisValue(pad.axes?.[2])];
    // A held stick must not resume after focus/mode/device changes. This is a
    // release latch, not a deadzone: send the raw axes once centered. "Centered" uses
    // the ESP32's own deadzones (move 0.12 radial, turn 0.25), because analog sticks
    // rarely rest at exactly 0.
    if (Math.hypot(axes[0], axes[1]) <= 120 && Math.abs(axes[2]) <= 250) this.motionReady = true;
    this.axes = this.motionReady ? axes : [0, 0, 0];
    let buttons = 0;
    for (const [indices, bit, counter] of controllerButtons) {
      const index = indices[0];
      const down = indices.some(i => Boolean(pad.buttons?.[i]?.pressed));
      if (!down) this.released.add(index);
      else if (this.released.has(index)) {
        buttons |= bit;
        if (counter && !(this.buttons & bit)) this.counters[counter] = (this.counters[counter] + 1) & 0xffff;
      }
    }
    this.buttons = buttons;
    const lt = triggerValue(pad.buttons?.[6]?.value);
    if (lt === 0) this.released.add(6);
    this.lt = this.released.has(6) ? lt : 0;
  }
  snapshot() {
    const [lx, ly, rx] = this.focused ? this.axes : [0, 0, 0];
    return { focused: this.focused, lx, ly, rx, ry: 0, lt: this.focused ? this.lt : 0, rt: 0,
      buttons: this.focused ? this.buttons : 0, ...this.counters };
  }
}

export class ScreenAwake {
  constructor(api) { this.api = api; this.wanted = false; this.lock = null; this.pending = false; }
  async update(wanted) {
    this.wanted = wanted;
    if (!wanted) {
      const lock = this.lock; this.lock = null;
      try { await lock?.release(); } catch { /* A system-released lock is harmless. */ }
      return;
    }
    if (!this.api?.request || this.lock || this.pending) return;
    this.pending = true;
    try {
      const lock = await this.api.request('screen');
      if (!this.wanted) { await lock.release(); return; }
      this.lock = lock;
      lock.addEventListener('release', () => { if (this.lock === lock) this.lock = null; });
    } catch { /* HTTP, low battery and browser policy may deny this optional API. */ }
    finally { this.pending = false; }
  }
}

export class BrowserInput {
  constructor(mode = 'keyboard') { this.mode = mode; this.resetSession(); }
  resetSession() {
    const shared = counters();
    const throttle = this.stick?.throttle ?? .5;
    this.keyboard = new KeyboardState(shared); this.stick = new StickState(shared); this.stick.throttle = throttle;
    this.controller = new ControllerState(shared);
  }
  get focused() { return this[this.mode].focused; }
  focus() { this.keyboard.focus(); this.stick.focus(); this.controller.focus(); }
  blur() { this.keyboard.blur(); this.stick.blur(); this.controller.blur(); }
  releaseAll() { this.keyboard.keys.clear(); this.stick.clear(); this.controller.clear(); }
  switchMode(mode) {
    if (!modes.includes(mode) || mode === this.mode) return false;
    this.blur(); this.mode = mode; return true;
  }
  snapshot() { return this[this.mode].snapshot(); }
}

export function encodeMessage(state, sequence) {
  const buffer = new ArrayBuffer(MESSAGE_SIZE);
  const view = new DataView(buffer);
  view.setUint8(0, 0x56); view.setUint8(1, 1); view.setUint8(2, state.focused ? 1 : 0);
  view.setUint16(4, sequence & 0xffff, true);
  ['lx', 'ly', 'rx', 'ry'].forEach((axis, index) => view.setInt16(6 + index * 2, state[axis], true));
  view.setUint8(14, state.lt); view.setUint8(15, state.rt); view.setUint16(16, state.buttons, true);
  ['kick', 'dribbler', 'reset', 'line'].forEach((counter, index) => view.setUint16(18 + index * 2, state[counter] & 0xffff, true));
  return buffer;
}

export function statusBadge(status) {
  if (status.operatorStop) return ['停止中（Esc で解除）', 'stopped'];
  if (!status.robotLink) return ['接続待ち', 'idle'];
  if (status.flags & (1 << 9)) return ['ペアリング中（ロボットは停止）', 'idle'];
  if (status.fault) return ['停止中', 'stopped'];
  if (!(status.flags & 1)) return ['赤スイッチで有効にしてください', 'idle'];
  if ((status.flags & (1 << 11)) || status.handover) return ['キーを離してください', 'idle'];
  if (status.owner === 'gamepad') return ['ゲームパッドで操作中', status.flags & 2 ? 'allowed' : 'idle'];
  if (status.owner === 'browser' && (status.flags & 2)) return ['操作できます', 'allowed'];
  return ['接続待ち', 'idle'];
}

function attachPage() {
  const element = id => document.getElementById(id);
  let storage;
  try { storage = window.localStorage; } catch { /* Access itself can throw. */ }
  const input = new BrowserInput(defaultMode(Boolean(window.matchMedia?.('(pointer: coarse)').matches), storage));
  const navigator = window.navigator ?? {};
  const awake = new ScreenAwake(navigator.wakeLock);
  let controllerError = false;
  const captures = new Map();
  let socket = null, sequence = 0, retry = 500, timer = null;
  let robot = null, statusAt = 0, busy = false, pageShown = true;
  const port = document.querySelector('meta[name="rc-websocket-port"]').content;
  const pageVisible = () => document.visibilityState === 'visible';
  const pageFocused = () => document.hasFocus() && pageVisible();
  // The wire's focused bit means this source is active. Controller mode needs
  // visibility, not window focus; reconnect/page return still resets its latch.
  const inputAllowed = () => input.mode === 'controller'
    ? pageShown && pageVisible() && socket?.readyState === WebSocket.OPEN : pageFocused();
  const updateOverlay = () => { element('focus-overlay').hidden = input.mode === 'controller' || input.focused; };
  const send = (edge = false, backOverride = null) => {
    if (socket?.readyState !== WebSocket.OPEN) return;
    if (socket.bufferedAmount > 512) { socket.close(); return; }
    const state = input.snapshot();
    if (backOverride !== null) state.buttons = (state.buttons & ~BUTTON.BACK) | (backOverride ? BUTTON.BACK : 0);
    if (edge || socket.bufferedAmount === 0) socket.send(encodeMessage(state, sequence++));
    // Edge messages retain Back press/release order; heartbeats never build a backlog.
  };
  const renderController = () => {
    const controller = input.controller;
    element('controller-status').textContent = typeof navigator.getGamepads !== 'function'
      ? 'このブラウザはコントローラーに対応していません'
      : controllerError ? 'コントローラーを読み取れません'
      : controller.nonStandard ? '標準配置のコントローラーを接続してください'
      : controller.id !== null ? `コントローラー: ${controller.id}`
      : controller.seen ? 'コントローラーが切れました' : 'コントローラーのボタンを押してください';
    element('controller-hint').hidden = controller.id === null || controller.motionReady;
  };
  const pollController = () => {
    const previous = input.controller.snapshot(), previousId = input.controller.id, previousIndex = input.controller.index;
    try { input.controller.update(navigator.getGamepads?.() ?? []); controllerError = false; }
    catch { input.controller.disconnect(); controllerError = true; }
    renderController();
    const current = input.controller.snapshot();
    return previousId !== input.controller.id || previousIndex !== input.controller.index ||
      ['buttons', 'lt', 'kick', 'dribbler', 'line', 'reset'].some(key => previous[key] !== current[key]) ||
      (['lx', 'ly', 'rx'].some(key => previous[key] !== 0) && ['lx', 'ly', 'rx'].every(key => current[key] === 0));
  };
  const updateAwake = () => { void awake.update(input.mode === 'controller' && input.focused && inputAllowed()); };
  const drawInputs = () => {
    const state = input.stick.snapshot();
    const move = [...input.stick.pointers.values()].find(pointer => pointer.control === 'move');
    for (const [name, x, y] of [['move', move?.x ?? 0, move?.y ?? 0], ['turn', state.rx, (input.stick.throttle * 2 - 1) * 1000]]) {
      const travel = element(`${name}-stick`).getBoundingClientRect().width * .32;
      element(`${name}-knob`).style.transform = `translate(${x / 1000 * travel}px, ${-y / 1000 * travel}px)`;
    }
    element('throttle').textContent = `速度 ${Math.round(input.stick.throttle * 100)}%`;
    for (const control of Object.keys(touchButtons)) element(`touch-${control}`).className =
      `touch-button ${control === 'stop' ? 'danger' : ''} ${[...input.stick.pointers.values()].some(pointer => pointer.control === control) ? 'pressed' : ''}`;
  };
  const releaseCaptures = () => {
    for (const [pointerId, target] of captures) {
      captures.delete(pointerId);
      try { if (target.hasPointerCapture(pointerId)) target.releasePointerCapture(pointerId); } catch { /* A detached pointer is already released. */ }
    }
  };
  const applyMode = () => {
    document.body.dataset.mode = input.mode;
    document.documentElement.dataset.mode = input.mode;
    element('stick-panel').hidden = input.mode !== 'stick';
    element('controller-panel').hidden = input.mode !== 'controller';
    element('focus').textContent = input.mode === 'keyboard' ? 'クリックして操作' : 'タップして操作';
    element('viewport').content = input.mode === 'stick'
      ? 'width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover'
      : 'width=device-width, initial-scale=1';
    for (const mode of modes) element(`mode-${mode}`).setAttribute('aria-pressed', String(input.mode === mode));
    drawInputs(); renderController(); updateAwake(); updateOverlay();
  };
  const changeMode = mode => {
    if (input.mode === mode) return;
    input.blur(); releaseCaptures(); send(true); // Neutral in the old mode before changing any controls.
    input.switchMode(mode); rememberMode(mode, storage); applyMode();
    if (robot) render(robot, false);
    if (mode === 'controller') setFocus();
  };
  const updateConnection = () => {
    const fresh = robot && performance.now() - statusAt < 700 && robot.robotLink;
    element('connection').textContent = `${fresh ? 'ロボット接続 OK' : 'ロボット未接続'} · WebSocket ${socket?.readyState === WebSocket.OPEN ? '接続中' : '再接続中'}`;
    if (!robot || performance.now() - statusAt >= 700) {
      element('badge').textContent = '接続待ち'; element('badge').className = 'badge idle';
      element('fault').hidden = true;
      for (const id of ['heading', 'stick-heading', 'line', 'dribbler', 'kick', 'firmware', 'touch-dribbler-state', 'touch-line-state']) element(id).textContent = '—';
      element('touch-dribbler').setAttribute('aria-pressed', 'false');
      element('touch-line').setAttribute('aria-pressed', 'false');
      element('pairing').hidden = true;
    }
  };
  const setFocus = () => {
    if (inputAllowed()) input.focus(); else input.blur();
    releaseCaptures(); drawInputs();
    updateOverlay();
    renderController(); updateAwake(); send(true);
  };
  const loseFocus = () => {
    input.blur(); releaseCaptures(); drawInputs(); renderController(); updateAwake(); send(true); updateOverlay();
  };
  const faults = ['なし', 'ESP32 との通信切れ', '操作入力なし', 'ジャイロ異常', 'ライン基板と通信できません（G でライン補助を切ると動けます）', '通信データ異常', 'ペアリング中', '初期化異常'];
  const render = (status, received = true) => {
    robot = status; if (received) statusAt = performance.now();
    const [label, colour] = statusBadge(status);
    element('badge').textContent = input.mode !== 'keyboard' && status.operatorStop ? '停止中（停止ボタンで解除）'
      : input.mode === 'stick' && label === 'キーを離してください' ? 'スティックを離してください' : label;
    element('badge').className = `badge ${colour}`;
    element('fault').hidden = !status.robotLink || !status.fault;
    element('fault').textContent = `停止: ${faults[status.fault] ?? '異常'}`;
    element('owner').textContent = ({ browser: ({ keyboard: 'キーボード', stick: 'スティック', controller: 'コントローラー' })[input.mode], gamepad: 'ゲームパッド', none: 'なし' })[status.owner];
    element('heading').textContent = status.robotLink ? `${status.heading.toFixed(1)}° / 目標 ${status.targetHeading.toFixed(1)}°` : '—';
    element('stick-heading').textContent = status.robotLink ? `${status.heading.toFixed(1)}°` : '—';
    element('line').textContent = status.robotLink ? `${status.flags & (1 << 10) ? '有効' : '無効'} · ${status.flags & (1 << 12) ? '判定不明' : status.flags & (1 << 4) ? '検知' : '検知なし'}` : '—';
    element('dribbler').textContent = status.robotLink ? status.flags & (1 << 7) ? '有効' : '無効' : '—';
    element('touch-dribbler-state').textContent = element('dribbler').textContent;
    element('touch-line-state').textContent = status.robotLink ? status.flags & (1 << 10) ? '有効' : '無効' : '—';
    element('touch-dribbler').setAttribute('aria-pressed', String(status.robotLink && Boolean(status.flags & (1 << 7))));
    element('touch-line').setAttribute('aria-pressed', String(status.robotLink && Boolean(status.flags & (1 << 10))));
    element('kick').textContent = status.robotLink ? status.flags & (1 << 8) ? '作動中' : '待機' : '—';
    element('speed').textContent = status.commandFlags & 32 ? '低速' : status.commandFlags & 16 ? 'ブースト' : '通常';
    element('firmware').textContent = status.robotLink ? status.firmware : '—';
    const robotPairing = status.robotLink && Boolean(status.flags & (1 << 9));
    element('pairing').hidden = !status.pairingSeconds && !robotPairing && !status.gamepadConnected;
    const pairingText = status.pairingSeconds
      ? `ペアリング受付中: 残り ${status.pairingSeconds} 秒 · 最初のゲームパッドを登録します`
      : robotPairing ? 'ペアリング中（ロボットは無効）' : '';
    const controllerText = status.gamepadConnected
      ? `${status.gamepadModel} · ${status.gamepadAddress} · ${status.gamepadApproved ? '登録済み' : '未登録'}${status.pairingSaved ? '' : '（保存できませんでした）'}` : '';
    element('pairing').textContent = [pairingText, controllerText].filter(Boolean).join(' · ');
    updateConnection();
  };
  const connect = () => {
    socket = new WebSocket(`ws://${location.hostname}:${port}/`);
    socket.binaryType = 'arraybuffer';
    socket.addEventListener('open', () => {
      input.blur(); releaseCaptures(); input.resetSession(); sequence = 0; busy = false; retry = 500;
      element('busy').hidden = true; setFocus(); updateConnection();
    });
    socket.addEventListener('message', event => {
      if (typeof event.data !== 'string') return;
      try {
        const status = JSON.parse(event.data);
        if (status.error === 'control_busy') { busy = true; element('busy').hidden = false; }
        else if (typeof status.flags === 'number' && typeof status.heading === 'number') render(status);
      } catch { /* Ignore malformed status messages. */ }
    });
    socket.addEventListener('close', () => {
      input.blur(); releaseCaptures(); drawInputs(); updateAwake(); robot = null; updateConnection(); updateOverlay();
      clearTimeout(timer); timer = setTimeout(connect, busy ? 5000 : retry); retry = Math.min(retry * 2, 5000);
    });
    socket.addEventListener('error', () => socket.close());
  };
  window.addEventListener('keydown', event => {
    if (input.mode !== 'keyboard') return;
    if (!supported.has(event.code)) return;
    event.preventDefault();
    if (!input.focused && pageFocused() && !event.repeat) setFocus();
    if (input.keyboard.keyDown(event.code, event.repeat)) send(true);
  });
  window.addEventListener('keyup', event => {
    if (input.mode !== 'keyboard') return;
    if (!supported.has(event.code)) return;
    event.preventDefault(); if (input.keyboard.keyUp(event.code)) send(true);
  });
  window.addEventListener('blur', () => { if (input.mode !== 'controller') loseFocus(); });
  window.addEventListener('focus', () => { if (input.mode !== 'controller') setFocus(); });
  document.addEventListener('visibilitychange', () => inputAllowed() ? setFocus() : loseFocus());
  window.addEventListener('pagehide', () => { pageShown = false; loseFocus(); });
  window.addEventListener('pageshow', () => { pageShown = true; setFocus(); });
  window.addEventListener('resize', () => {
    if (input.mode === 'stick') { input.releaseAll(); releaseCaptures(); drawInputs(); send(true); }
  });
  element('focus').addEventListener('click', () => { window.focus(); setFocus(); });
  element('stop').addEventListener('click', () => {
    if (!input.focused) setFocus();
    if (!input.focused) return;
    input.keyboard.keyUp('Escape'); input.controller.blockBackUntilRelease();
    send(true, false); send(true, true); send(true, false);
  });
  for (const mode of modes) element(`mode-${mode}`).addEventListener('click', () => changeMode(mode));
  window.addEventListener('gamepadconnected', () => {
    if (input.mode !== 'controller') changeMode('controller');
    else { input.controller.disconnect(); send(true); renderController(); }
  });
  window.addEventListener('gamepaddisconnected', () => {
    input.controller.disconnect(); renderController();
    if (input.mode === 'controller') send(true);
  });
  const bindPointer = (target, control) => {
    const position = event => {
      if (!['move', 'turn'].includes(control)) return { x: 0, y: 0 };
      const rect = target.getBoundingClientRect();
      const dx = event.clientX - rect.left - rect.width / 2, dy = event.clientY - rect.top - rect.height / 2;
      if (control === 'turn') {
        const value = throttleAxes(dx, dy, rect.width * .32);
        return { x: value.x, y: value.throttle };
      }
      return stickAxes(dx, dy, rect.width * .32);
    };
    target.addEventListener('pointerdown', event => {
      if (input.mode !== 'stick' || !pageFocused() || event.button !== 0) return;
      event.preventDefault();
      if (!input.focused) setFocus();
      if ([...input.stick.pointers.values()].some(pointer => pointer.control === control)) return;
      // Capture before accepting the press: a capture failure cannot leave an active command.
      try { target.setPointerCapture(event.pointerId); } catch { return; }
      const { x, y } = position(event);
      if (!input.stick.down(control, event.pointerId, x, y)) {
        target.releasePointerCapture(event.pointerId); return;
      }
      captures.set(event.pointerId, target); drawInputs(); send(true);
    });
    target.addEventListener('pointermove', event => {
      if (captures.get(event.pointerId) !== target) return;
      event.preventDefault();
      const { x, y } = position(event);
      if (input.stick.move(event.pointerId, x, y)) { drawInputs(); send(true); }
    });
    const release = event => {
      if (captures.get(event.pointerId) !== target) return;
      event.preventDefault(); captures.delete(event.pointerId);
      if (event.type === 'pointerup' && control === 'turn') {
        const { x, y } = position(event); input.stick.move(event.pointerId, x, y);
      }
      if (input.stick.release(event.pointerId)) { drawInputs(); send(true); }
      try { if (target.hasPointerCapture(event.pointerId)) target.releasePointerCapture(event.pointerId); } catch { /* Already lost. */ }
    };
    target.addEventListener('pointerup', release);
    target.addEventListener('pointercancel', release);
    target.addEventListener('lostpointercapture', release);
    target.addEventListener('contextmenu', event => { if (input.mode === 'stick') event.preventDefault(); });
    if (control in touchButtons) {
      target.addEventListener('click', event => {
        // Pointer presses were handled above; retain Enter/assistive activation without duplicate counters.
        if (event.detail !== 0 || input.mode !== 'stick' || !pageFocused()) return;
        if (!input.focused) setFocus();
        if (input.stick.down(control, 'activation')) { drawInputs(); send(true); input.stick.release('activation'); drawInputs(); send(true); }
      });
    }
  };
  for (const name of ['move', 'turn']) bindPointer(element(`${name}-stick`), name);
  for (const name of Object.keys(touchButtons)) bindPointer(element(`touch-${name}`), name);
  document.addEventListener('contextmenu', event => { if (input.mode === 'stick') event.preventDefault(); });
  // Enter on a focused button also works; held keyboard keys never survive focus recovery.
  setInterval(() => {
    if (!input.focused || !inputAllowed()) return;
    send(input.mode === 'controller' ? pollController() : false);
  }, 20);
  setInterval(updateConnection, 100);
  applyMode(); setFocus(); connect();
}
if (typeof document !== 'undefined') attachPage();
)RC_ASSET";

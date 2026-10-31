// Independent JS port of the frozen UART contract for golden-vector tests.
export function crc8(bytes) {
  let crc = 0;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit++) crc = ((crc << 1) ^ (crc & 128 ? 7 : 0)) & 255;
  }
  return crc;
}
export function encodeFrame(type, sequence, payload) {
  if (!((type === 1 && payload.length === 12) || (type === 2 && payload.length === 28))) throw new RangeError('type/length');
  const frame = new Uint8Array(8 + payload.length);
  frame.set([0xa5, 0x5a, 1, type, payload.length]);
  new DataView(frame.buffer).setUint16(5, sequence, true);
  frame.set(payload, 7); frame[frame.length - 1] = crc8(frame.subarray(2, -1));
  return frame;
}
export function decodeFrame(frame) {
  if (frame.length < 8 || frame[0] !== 0xa5 || frame[1] !== 0x5a || frame[2] !== 1 ||
      !((frame[3] === 1 && frame[4] === 12) || (frame[3] === 2 && frame[4] === 28)) ||
      frame.length !== 8 + frame[4] || crc8(frame.subarray(2, -1)) !== frame.at(-1)) return null;
  return { type: frame[3], sequence: new DataView(frame.buffer, frame.byteOffset, frame.byteLength).getUint16(5, true), payload: frame.slice(7, -1) };
}
export function commandPayload({ flags = 8, x = 0, y = 0, turn = 0, kick = 0, reset = 0 } = {}) {
  const bytes = new Uint8Array(12), view = new DataView(bytes.buffer);
  view.setUint16(0, flags, true); view.setInt16(2, x, true); view.setInt16(4, y, true);
  view.setInt16(6, turn, true); view.setUint16(8, kick, true); view.setUint16(10, reset, true);
  return bytes;
}
export function newer(candidate, previous) { const delta = (candidate - previous) & 65535; return delta > 0 && delta < 32768; }
export function decodeBrowser(buffer) {
  const bytes = new Uint8Array(buffer), view = new DataView(buffer);
  if (bytes.length !== 26 || bytes[0] !== 0x56 || bytes[1] !== 1 || (bytes[2] & ~1) || bytes[3]) return null;
  const axes = [6, 8, 10, 12].map(offset => view.getInt16(offset, true));
  const buttons = view.getUint16(16, true);
  if (axes.some(axis => Math.abs(axis) > 1000) || (buttons & ~127)) return null;
  if (!bytes[2] && (axes.some(Boolean) || bytes[14] || bytes[15] || buttons)) return null;
  return { focused: Boolean(bytes[2]), sequence: view.getUint16(4, true), axes, lt: bytes[14], rt: bytes[15], buttons,
    counters: [18, 20, 22, 24].map(offset => view.getUint16(offset, true)) };
}
// Mapping port verifies the virtual convention and vector cap, not browser-specific scaling.
export function mapAxes({ lx, ly, rx, lt, buttons }) {
  const slow = lt >= 128, boost = !slow && Boolean(buttons & 32);
  const cap = slow ? 400 : boost ? 1000 : 800, turn = slow ? 9000 : boost ? 27000 : 18000;
  let x = lx / 1000, y = ly / 1000, rotation = rx / 1000;
  const magnitude = Math.hypot(x, y);
  if (magnitude <= .12) x = y = 0;
  else { const scale = (Math.min(magnitude, 1) - .12) / (.88 * magnitude); x *= scale; y *= scale; }
  rotation = Math.abs(rotation) <= .25 ? 0 : Math.sign(rotation) * (Math.min(Math.abs(rotation), 1) - .25) / .75;
  return { x: Math.trunc(x * cap), y: Math.trunc(y * cap), turn: Math.trunc(rotation * turn), slow, boost };
}

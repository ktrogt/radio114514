import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { crc8, encodeFrame, decodeFrame, commandPayload, newer } from './protocol_helpers.mjs';

const header = await readFile(new URL('../Common/rc_protocol.h', import.meta.url), 'utf8');
const golden = header.split('Example complete golden frames')[1].split('*/')[0].split(/\n \* \d\)/).slice(1).map(section => {
  const hex = section.slice(section.indexOf('A5')).match(/\b[0-9A-F]{2}\b/g);
  return Uint8Array.from(hex, byte => parseInt(byte, 16));
});
test('CRC-8/ATM standard check vector', () => assert.equal(crc8(new TextEncoder().encode('123456789')), 0xf4));
test('all three frozen golden frames encode, decode and re-encode exactly', () => {
  assert.equal(golden.length, 3);
  const telemetry = new Uint8Array(28); telemetry[1] = 8; telemetry[17] = 1;
  const encoded = [encodeFrame(1, 0, commandPayload()), encodeFrame(1, 1, commandPayload({ flags: 11, y: 400, kick: 1 })), encodeFrame(2, 0, telemetry)];
  encoded.forEach((frame, index) => {
    assert.deepEqual(frame, golden[index]);
    const decoded = decodeFrame(golden[index]); assert.ok(decoded);
    assert.deepEqual(encodeFrame(decoded.type, decoded.sequence, decoded.payload), golden[index]);
  });
});
test('frame lengths and frozen macro offsets', () => {
  assert.deepEqual(golden.map(frame => frame.length), [20, 20, 36]);
  for (const [name, value] of [['RC_OFFSET_SEQUENCE', 5], ['RC_OFFSET_PAYLOAD', 7], ['RC_COMMAND_PAYLOAD_SIZE', 12], ['RC_TELEMETRY_PAYLOAD_SIZE', 28]])
    assert.match(header, new RegExp(`#define ${name} ${value}U`));
});
test('signed little-endian command fields and uint16 wrap', () => {
  const frame = encodeFrame(1, 65536, commandPayload({ flags: 43, x: -120, y: 250, turn: -4500, kick: 65536, reset: 65535 }));
  const decoded = decodeFrame(frame), view = new DataView(decoded.payload.buffer);
  assert.equal(decoded.sequence, 0); assert.equal(view.getInt16(2, true), -120);
  assert.equal(view.getInt16(6, true), -4500); assert.equal(view.getUint16(8, true), 0); assert.equal(view.getUint16(10, true), 65535);
});
test('corrupted, incomplete, unknown and oversized frames are rejected', () => {
  for (let index = 0; index < golden[2].length; index++) {
    const changed = golden[2].slice(); changed[index] ^= 1; assert.equal(decodeFrame(changed), null);
  }
  assert.equal(decodeFrame(golden[0].slice(0, -1)), null);
  assert.equal(decodeFrame(Uint8Array.from([...golden[0], 0])), null);
  assert.throws(() => encodeFrame(1, 0, new Uint8Array(28)), RangeError);
});
test('sequence ordering rejects duplicates/old frames and accepts wrap', () => {
  assert.equal(newer(0, 65535), true); assert.equal(newer(7, 7), false);
  assert.equal(newer(65535, 0), false); assert.equal(newer(32768, 0), false); assert.equal(newer(32767, 0), true);
});

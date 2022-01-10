import * as ipc from 'ipc';

const LITTLE_ENDIAN = (function () {
  const buffer = new ArrayBuffer(2);
  new DataView(buffer).setInt16(0, 256, true);
  return new Int16Array(buffer)[0] === 256;
})();

const DEFAULT_CAPACITY = 32;

/**
 * parcel构建器
 */
class Parcel {
  constructor(capacity, littleEndian) {
    this._miniCapacity = this._capacity = (capacity || DEFAULT_CAPACITY);
    this._buffer = new ArrayBuffer(this._capacity);
    this._littleEndian = littleEndian !== undefined ? littleEndian : LITTLE_ENDIAN;
    this._view = new DataView(this._buffer);
    this._uint8 = new Uint8Array(this._buffer);
    this._pos = -1;
  }

  grow(length) {
    let growed = false;
    while (this._capacity - (this._pos + length + 1) < 0) {
      this._capacity = this._capacity + (this._miniCapacity >> 1);
      growed = true;
    }

    if (growed) {
      const newBuffer = new ArrayBuffer(this._capacity);
      const newUint8 = new Uint8Array(newBuffer);
      for (let i = 0; i < this._uint8.length; i++) {
        newUint8[i] = this._uint8[i];
      }

      this._buffer = newBuffer;
      this._view = new DataView(this._buffer);
      this._uint8 = newUint8;
    }
  }

  writeString(str) {
    const bytes = [];
    for (let i = 0; i < str.length; i++) {
      let code = str.charCodeAt(i);
      if (0x00 <= code && code <= 0x7f) {
        bytes.push(code);
      } else if (0x80 <= code && code <= 0x7ff) {
        bytes.push(192 | (31 & (code >> 6)));
        bytes.push(128 | (63 & code));
      } else if ((0x800 <= code && code <= 0xd7ff)
        || (0xe000 <= code && code <= 0xffff)) {
        bytes.push(224 | (15 & (code >> 12)));
        bytes.push(128 | (63 & (code >> 6)));
        bytes.push(128 | (63 & code));
      }
    }

    this.grow(bytes.length);
    for (let i = 0; i < bytes.length; i++) {
      this._uint8[++this._pos] = bytes[i];
    }
    this._uint8[++this._pos] = 0;
    return this;
  }

  writeByteArray(data) {
    return this;
  }

  writeInt32(value) {
    this.grow(4);
    this._view.setInt32(this._pos + 1, value);
    this._pos += 4;
    return this;
  }

  writeUint32(value) {
    this.grow(4);
    this._view.setUint32(this._pos + 1, value);
    this._pos += 4;
    return this;
  }

  writeFloat(value) {
    this.grow(4);
    this._view.setFloat32(this._pos + 1, value);
    this._pos += 4;
    return this;
  }

  writeDouble(value) {
    this.grow(8);
    this._view.setFloat64(this._pos + 1, value);
    this._pos += 8;
    return this;
  }

  readString() {
    let out = '', char2, char3;

    while (this._uint8[this._pos] !== 0) {
      let c = this._uint8[this._pos++];
      switch (c >> 4) {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
          // 0xxxxxxx
          out += String.fromCharCode(c);
          break;
        case 12: case 13:
          // 110x xxxx   10xx xxxx
          char2 = this._uint8[this._pos++];
          out += String.fromCharCode(((c & 0x1F) << 6) | (char2 & 0x3F));
          break;
        case 14:
          // 1110 xxxx  10xx xxxx  10xx xxxx
          char2 = this._uint8[this._pos++];
          char3 = this._uint8[this._pos++];
          out += String.fromCharCode(((c & 0x0F) << 12) |
            ((char2 & 0x3F) << 6) |
            ((char3 & 0x3F) << 0));
          break;
      }
    }
    return out;
  }

  readByteArray(len) {

  }

  readInt32() {
    let result = this._view.getInt32(this._pos + 1);
    this._pos += 4;
    return result;
  }

  readUInt32() {
    let result = this._view.getUint32(this._pos + 1);
    this._pos += 4;
    return result;
  }

  readFloat() {
    let result = this._view.getFloat32(this._pos + 1);
    this._pos += 4;
    return result;
  }

  readDouble() {
    let result = this._view.getFloat64(this._pos + 1);
    this._pos += 8;
    return result;
  }

  isEmpty() {
    return this._pos == 0;
  }

  getLength() {
    this._buffer.byteLength;
  }

  getDataPosition() {
    this._pos;
  }

  getData() {
    return this._buffer;
  }

  setDataPositoin(pos) {
    this._pos = pos;
  }

  recycle() {
    this._buffer = null;
    this._view = null;
    this._pos = -1;
    this._uint8 = null;
  }
}

class Ipc {
  constructor(sid) {
    this.sid = sid;
    this.service = ipc.service(this.sid);
  }

  /**
   * 请注意,下面的arg参数不是所有都可以生效.底层c实现为两个32位的有符号(无符号)int,或者一个64位的u64/float/double.
   * 后面会覆盖前面
   * message对象属性:
   *  what:请求类型
   *  
   *  参数组1:两个int
   *  arg
   *  arg2
   * 
   *  参数组2:两个uint32
   *  argU32
   *  arg2U32
   * 
   *  参数组3
   *  argI64
   * 
   *  参数组4:
   *  argU64
   * 
   *  参数组5:
   *  argFloat
   *  
   *  参数组:6
   *  argDouble
   * 
   *  payload:本地请求的payload,支持string或ArrayBuffer,根据实际业务需求协商定义.
   * @param {Object} message Ipc请求消息
   * 
   * 支持调用形式(timeout, callback可为空.底层timout默认值为2000ms):
   * request(what, arg)
   * request(what, arg,timeout)
   * request(object, timeout)
   */
  request(what, arg, timeout) {
    this.service.request.apply(this.service, arguments)
  }

  /**
   * 参数同request
   * @param {Object} message Ipc请求消息
   * 
   * 
   * 支持调用形式(timeout, callback可为空):
   * requestAsync(what, callback)
   * requestAsync(what, arg, callback)
   * requestAsync(what, arg, timeout, callback)
   * requestAsync(object, timeout, callback)
   * requestAsync(object, callback)
   */
  requestAsync(what, arg, timeout, callback) {
    this.service.requestAsync.apply(this.service, arguments);
  }

  sendMessage(){
    
  }

  on(callback) {
    this.service.on(callback);
  }

  off(callback) {
    this.service.off(callback);
  }

  finalize() {
    this.service.finalize();
  }
}

Ipc.Parcel = Parcel;

export default Ipc;
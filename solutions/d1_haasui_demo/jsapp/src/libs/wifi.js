import Ipc from './ipc.js';

const WIFI_SERVICE_SID = 10001;

class WifiService extends Ipc {

  static instance() {
    return new WifiService();
  }

  constructor() {
    const ipc = new Ipc(WIFI_SERVICE_SID);
  }

  open() {
    ipc.request()

  }

  close() {

  }

  scan() {

  }

  connect() {

  }
}

export default WifiService;
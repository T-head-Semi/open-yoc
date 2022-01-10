import dbus from 'qjs-dbus'
import events from 'events'

var MediaDbusEvent = {
    IFACE_GOT: "interface_got",
};

class MediaDbus extends events.EventEmitter {
    constructor() {
        console.log("MediaDbus Constructor");
        super();
        this._instance = null;
        this._interf = null;
        this._timeid = null;
        this._retry_cnt = 0;
        this._bus = dbus.getBus('system');

        this._init();
    }

    _init() {
        var s = 'org.media.server';
        var o = '/org/media/path';
        var i = 'org.media.interface';

        this._bus.getInterface(s, o, i, oniface.bind(this));

        function oniface(err, obj) {
            this._clearTimer();
            if (err) {
                if (this._retry_cnt++ < 10) {
                    console.log("MediaDbus: get dbus iface error, retry cnt = " + this._retry_cnt);
                    this._timeid = setTimeout(function() {
                        this._bus.getInterface(s, o, i, oniface.bind(this));
                    }.bind(this), 2000);
                }
            } else {
                console.log("MediaDbus: get dbus iface ok");
                this._interf = obj;
                this.emit(MediaDbusEvent.IFACE_GOT, this._interf);
            }
        }
    }

    _clearTimer() {
        if (this._timeid) {
            clearTimeout(this._timeid);
            this._timeid = null;
        }
    }

    /**
     * @description: get the single instance of MediaDbus
     * @param {*}
     * @return {Object} null on error
     */
    static getInstance() {
        if (!this._instance) {   
            this._instance = new MediaDbus();
        }
        
        return this._instance;
    }

    /**
     * @description: get low-level dbus interface
     * @param {*}
     * @return {*}
     */     
    getInterface() {
        return this._interf;
    }  
}

export { MediaDbus, MediaDbusEvent };



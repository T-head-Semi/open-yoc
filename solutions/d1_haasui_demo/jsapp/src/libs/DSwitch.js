import dbus from 'qjs-dbus'
import events from 'events'

class DSwitch extends events.EventEmitter {
    /**
     * @description: constructor function
     * @param {Number} type
     * @param {Object} options config param
     * @return {*}
     */    
    constructor() {
        console.log("DSwitch Constructor");
        super();    
        var s = 'org.dbus_demo.server';
        var o = '/org/dbus_demo/path';
        var i = 'org.dbus_demo.interface';

        this._timeid = null;
        this._retry_cnt = 0;
        this._interf = null;
        this._bus = dbus.getBus('system');
        this._bus.getInterface(s, o, i, oniface.bind(this));

        function oniface(err, obj) {
            this._clearTimer();
            if (err) {
                if (this._retry_cnt++ < 10) {
                    console.log("DSwitch: get dbus iface error, retry cnt = " + this._retry_cnt);
                    this._timeid = setTimeout(function() {
                        this._bus.getInterface(s, o, i, oniface.bind(this));
                    }.bind(this), 2000);
                }
            } else {
                console.log("DSwitch: get dbus iface ok");
                this._interf = obj;
                this._lisen();
            }
        }
    }

    _lisen() {
        this._interf.on('SwitchStatus', function(obj) {
            console.log("==>>SwitchStatus");
            console.log(obj);
            this.emit("SwitchEvent", obj);
        }.bind(this));
    }

    _clearTimer() {
        if (this._timeid) {
            clearTimeout(this._timeid);
            this._timeid = null;
        }
    }

    /**
     * @description: inner callback, return result
     * @param {Object} cb
     * @param {Object} err
     * @param {Auto} result
     * @param {Auto} default_val
     * @return {*}
     */
    _doCallback(cb, err, result, default_val) {
        var rc = default_val;

        if (err) {
            console.log("inner err happens");
            console.log(err);
        } else {
            rc = result;
        }
        
        if (cb)
            cb(rc);
    }

    /**
     * @description: inner callback, return result
     * @param {Object} cb
     * @param {Auto} result
     * @return {*}
     */
    _doErrorCallback(cb, result) {
        if (!this._interf)
            console.log("interface don't got now, may be call later");
        else
            console.log("param may be failed");
        if (cb)
            cb(result);
    }

    /**
     * @description: switch-play of the Switch
     * @param {Boolean} on or off
     * @param {Object} cb function(Number)
     * @return {*}
     */    
    doSwitch(onoff, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        this._interf.Switch(onoff, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }

    /**
     * @description: Destroy the Switch
     * @return {*}
     */    
    destroy() {
        this._clearTimer();
		this._bus.disconnect();
    }
}

export { DSwitch };




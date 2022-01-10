import dbus from 'qjs-dbus'
import events from 'events'
import { MediaDbus, MediaDbusEvent } from './MediaDbus.js';

var MediaEvent = {
    MUTE: "mute",
    VOL_CHANGE: "vol_change",
};

class MediaService extends events.EventEmitter {
    constructor() {
        console.log("MediaService Constructor");
        super();
        this.type = 0xff;//MediaType.MAX
        this._vol = 60;
        this._instance = null;

        this._interf = MediaDbus.getInstance().getInterface();
        if (this._interf) {
            console.log("MediaService: media interface got yet");
            this.setVol(this._vol, null);
            this._lisen();
        } else {
            console.log("MediaService: media interface don't got now, listen it!");
            MediaDbus.getInstance().on(MediaDbusEvent.IFACE_GOT, function(obj) {
                this._interf = obj;
                this.setVol(this._vol, null);
                this._lisen();
                console.log("MediaService: got media interface now");
            }.bind(this));    
        }
    }

    _lisen() {
        this._interf.on('PlayMute', function(obj) {
            console.log("==>>MediaService PlayMute:");
            console.log(obj);
            this.emit(MediaEvent.MUTE, obj.type, obj.mute);
        }.bind(this));          

        this._interf.on('PlayVolChange', function(obj) {
            console.log("==>>MediaService vol change:");
            console.log(obj);
            this.emit(MediaEvent.VOL_CHANGE, obj.type, obj.valOld, obj.valNew);         
        }.bind(this));         
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
            console.log("inner err happens, url = " + this.url);
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
        console.log("interface don't got now, may be call later");
        if (cb)
            cb(result);
    } 

    /**
     * @description: get the single instance of MediaService
     * @param {*}
     * @return {Object} null on error
     */
    static getInstance() {
        if (!this._instance) {   
            this._instance = new MediaService();
        }
        
        return this._instance;
    }

    /**
     * @description: mute the system
     * @param {Object} cb function(Number)
     * @return {*}
     */    
    mute(cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        this._interf.Mute(this.type, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }

    /**
     * @description: unMute the system
     * @param {Object} cb function(Number)
     * @return {*}
     */    
    unMute(cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        this._interf.UnMute(this.type, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }

    /**
     * @description: get the volume  of the system
     * @param {Object} cb function(Number)
     * @return {*}
     */    
    getVol(cb) {
        //TODO:
        if (cb) {
            cb(this.vol);
        }
    }

    /**
     * @description: set the volume  of the system
     * @param {Number} vol [0, 100]
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setVol(vol, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: vol
        }
        this._interf.SetVol(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }    
}

export { MediaService, MediaEvent };



import dbus from 'qjs-dbus'
import events from 'events'
import { MediaDbus, MediaDbusEvent } from './MediaDbus.js';

var MediaType = {
    MEDIA: 0x0,
    SYSTEM: 0x1,
    MAX: 0xff
};

var PlayerEvent = {
    ERROR: "error",
    START: "start",
    PAUSE: "pause",
    RESUME: "resume",
    STOP: "stop",
    FINISH: "finish",
    UNDER_RUN: "under_run",
    OVER_RUN: "over_run",

    MUTE: "mute",
    TIME_UPDATE: "time_update",
    VOL_CHANGE: "vol_change",
};

var PlayerStatus = {
    UNKNOWN: 0x0,
    STOPED: 0x1,
    PLAYING: 0x2,
    PAUSED: 0x3
};

var DisplayFormat = {
    LETTER_BOX: 0x0,
    PAN_SCAN: 0x1,
    AUTO_FILLED: 0x2
};

var RotateType = {
    DEGREE_0: 0,
    DEGREE_90: 90,
    DEGREE_180: 180,
    DEGREE_270: 270,
};

class AuiPlayer extends events.EventEmitter {
    /**
     * @description: constructor function
     * @param {Number} type
     * @param {Object} options config param
     * @return {*}
     */    
    constructor(type, options) {
        console.log("AuiPlayer Constructor");
        super();    
        this.type = type;
        this.url = "";
        this._ptime = {
            duration: 0,
            curtime: 0
        }
        // TODO:
        this._timerId = 0;
        this._timeUpdateInterval = 0;
        if (options) {
            this._timeUpdateInterval = options.timeUpdateInterval ? options.timeUpdateInterval : this._timeUpdateInterval;
        }
        
        this._interf = MediaDbus.getInstance().getInterface();
        if (this._interf) {
            console.log("AuiPlayer: media interface got yet");
            this._lisen();
        } else {
            console.log("AuiPlayer: media interface don't got now, listen it!");
            MediaDbus.getInstance().on(MediaDbusEvent.IFACE_GOT, function(obj) {
                this._interf = obj;
                this._lisen();
                console.log("AuiPlayer: got media interface now");
            }.bind(this));    
        }
    }

    _startUpdateTime() {
        if (this._timeUpdateInterval > 0) {
            this.getCurTime(function(ptime) {
                if (ptime && ptime.duration > 0 && ptime.curtime >= 0) {
                    this._ptime = ptime;
                    this.emit(PlayerEvent.TIME_UPDATE, ptime.duration, ptime.curtime);
                }
            }.bind(this));
            
            this._timerId = setTimeout(() => {
                this._startUpdateTime();
            }, this._timeUpdateInterval);
        }
    }    

    _stopUpdateTime() {
        if (this._timerId) {
            clearTimeout(this._timerId);
            this._timerId = 0;
        }
    }    

    _reset() {
        this.url = "";
        this._ptime = {
            duration: 0,
            curtime: 0
        }        
        this._stopUpdateTime();
    } 

    _lisen() {
        this._interf.on('PlayError', function(obj) {
            console.log("==>>PlayError:");
            console.log(obj);
            if (obj.type == this.type && obj.url == this.url) {
                this._stopUpdateTime();
                this.emit(PlayerEvent.ERROR);
            }                 
        }.bind(this));

        this._interf.on('PlayStart', function(obj) {
            console.log("==>>PlayStart:");
            console.log(obj);
            if (obj.type == this.type && obj.url == this.url) {
                this._startUpdateTime();
                this.emit(PlayerEvent.START);
            }
        }.bind(this));
        
        this._interf.on('PlayPause', function(obj) {
            console.log("==>>PlayPause:");
            console.log(obj);
            if (obj.type == this.type && obj.url == this.url) {
                this._stopUpdateTime();
                this.emit(PlayerEvent.PAUSE);
            }
        }.bind(this));

        this._interf.on('PlayResume', function(obj) {
            console.log("==>>PlayResume:");
            console.log(obj);
            if (obj.type == this.type && obj.url == this.url) {
                this._startUpdateTime();
                this.emit(PlayerEvent.RESUME); 
            }
        }.bind(this));        

        this._interf.on('PlayStop', function(obj) {
            console.log("==>>PlayStop:");
            console.log(obj);
            if (obj.type == this.type && obj.url == this.url) {
                this._stopUpdateTime();
                this.emit(PlayerEvent.STOP); 
            }
        }.bind(this));

        this._interf.on('PlayFinish', function(obj) {
            console.log("==>>PlayFinish:");
            console.log(obj);
            if (obj.type == this.type && obj.url == this.url) {
                this._stopUpdateTime();
                this.emit(PlayerEvent.FINISH);
            }
        }.bind(this));

        this._interf.on('PlayUnderRun', function(obj) {
            console.log("==>>PlayUnderRun:");
            console.log(obj);
            if (obj.type == this.type && obj.url == this.url) {               
                this.emit(PlayerEvent.UNDER_RUN);
            }
        }.bind(this));   

        this._interf.on('PlayOverRun', function(obj) {
            console.log("==>>PlayOverRun:");
            console.log(obj);
            if (obj.type == this.type && obj.url == this.url) {               
                this.emit(PlayerEvent.OVER_RUN);
            }
        }.bind(this));  

        this._interf.on('PlayMute', function(obj) {
            console.log("==>>PlayMute:");
            console.log(obj);
            if (obj.type == this.type && (obj.url == this.url || obj.url == "default")) {              
                this.emit(PlayerEvent.MUTE, obj.mute);
            }
        }.bind(this));          

        this._interf.on('PlayVolChange', function(obj) {
            console.log("==>>vol change:");
            console.log(obj);
            if (obj.type == this.type && (obj.url == this.url || obj.url == "default")) {           
                this.emit(PlayerEvent.VOL_CHANGE, obj.valOld, obj.valNew);
            }            
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
        if (!this._interf)
            console.log("interface don't got now, may be call later");
        else
            console.log("param may be failed");
        if (cb)
            cb(result);
    }
     
    /**
     * @description: play the url of the player
     * @param {String} url
     * @param {Object} options
     * @param {Object} cb function(Number)
     * @return {*}
     */
    play(url, options, cb) {
        if (!(this._interf && url && url.length)) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var resume = 0;
        var seek_time = 0;
        
        if (options) {
            resume = options.resume ? options.resume : resume;
            seek_time = options.seek_time ? options.seek_time : seek_time;
        }

        var params = { 
            type: this.type,
            url: url,
            resume: resume,
            seek_time: seek_time
        }
        this.url = url;
        this._interf.Play(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }

    /**
     * @description: pause-play the media of the player
     * @param {Object} cb function(Number)
     * @return {*}
     */    
    pause(cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }        
        this._stopUpdateTime();

        this._interf.Pause(this.type, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }

    /**
     * @description: resume-play of the player
     * @param {Object} cb function(Number)
     * @return {*}
     */
    resume(cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }        
        this._interf.Resume(this.type, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));

        this._startUpdateTime();
    }

    /**
     * @description: mute the player
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
     * @description: unMute the player
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
     * @description: stop-play of the player
     * @param {Object} cb function(Number)
     * @return {*}
     */    
    stop(cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }        
        this._interf.Stop(this.type, function(err, result) {
            this._doCallback(cb, err, result, -1);
            this._reset();
        }.bind(this));
    }

    /**
     * @description: get play time of the player
     * @param {Object} cb function(Object)
     * @return {*}
     */
    getTime(cb) {
        if (!cb) {
            console.log("param err");
            return;
        }
        var ptime = { 
            duration: 0,
            curtime: 0
        }      
        if (!this._interf) {
            this._doErrorCallback(cb, ptime);
            return;
        }          
        this._interf.GetTime(this.type, function(err, result) {
            this._doCallback(cb, err, result, ptime);
        }.bind(this));
    }

    /**
     * @description: get the volume  of the player
     * @param {Object} cb function(Number)
     * @return {*}
     */    
    getVol(cb) {
        if (!cb) {
            console.log("param err");
            return;
        }
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        this._interf.GetVol(this.type, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }

    /**
     * @description: set the volume  of the player
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

    /**
     * @description: seek to the position of the player
     * @param {Number} seek_time
     * @param {Object} cb function(Number)
     * @return {*}
     */
    seek(seek_time, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: seek_time
        }
        this._interf.Seek(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }

    /**
     * @description: set play speed  of the player
     * @param {Number} speed [0.5, 2.0]
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setSpeed(speed, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: parseInt(speed * 10)
        } 
        this._interf.SetSpeed(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }

    /**
     * @description: get the speed of the player
     * @param {Object} cb function(Number)
     * @return {*}
     */   
    getSpeed(cb) {
        if (!cb) {
            console.log("param err");
            return;
        }
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        this._interf.GetSpeed(this.type, function(err, result) {
            if (!err) {
                result = parseFloat(result / 10.0);
            }
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    } 

    /**
     * @description: get status of the player
     * @param {Object} cb function(PlayerStatus)
     * @return {*}
     */   
    getStatus(cb) {
        if (!cb) {
            console.log("param err");
            return;
        }
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        this._interf.GetStatus(this.type, function(err, result) {
            this._doCallback(cb, err, result, PlayerStatus.UNKNOWN);
        }.bind(this));
    } 

    /**
     * @description: get media source url of the player
     * @return {String}
     */   
    getUrl() {
        return this.url;
    }   

    /**
     * @description: get media info of the player
     * @param {Object} cb function(Object)
     * @return {*}
     */   
    getMediaInfo(cb) {
        if (!cb) {
            console.log("param err");
            return;
        }
        if (!this._interf) {
            this._doErrorCallback(cb, null);
            return;
        }
        this._interf.GetMediaInfo(this.type, function(err, result) {
            this._doCallback(cb, err, result, null);
        }.bind(this));
    }    

    /**
     * @description: switch audio track of the player
     * @param {Number} idx index of audio
     * @param {Object} cb function(Number)
     * @return {*}
     */
    switchAudioTrack(idx, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: parseInt(idx)
        }        
        this._interf.SwitchAudioTrack(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }

    /**
     * @description: switch subtitle track of the player
     * @param {Number} idx index of subtitle
     * @param {Object} cb function(Number)
     * @return {*}
     */
    switchSubtitleTrack(idx, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: parseInt(idx)
        }        
        this._interf.SwitchSubtitleTrack(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }    

    /**
     * @description: set external subtitle url of the player
     * @param {String} url path of the external subtitle
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setSubtitleUrl(url, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            url: url
        }        
        this._interf.SetSubtitleUrl(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    } 

    /**
     * @description: show/hide subtitle of the player
     * @param {Boolean} visible show/hide
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setSubtitleVisible(visible, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: visible ? 1 : 0
        }        
        this._interf.SetSubtitleVisible(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }  

    /**
     * @description: show/hide video of the player
     * @param {Boolean} visible show/hide
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setVideoVisible(visible, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: visible ? 1 : 0
        }        
        this._interf.SetVideoVisible(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }    
    
    /**
     * @description: crop video of the player
     * @param {Object} win crop window
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setVideoCrop(win, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            x: win.x,
            y: win.y,
            width: win.width,
            height: win.height
        }        
        this._interf.SetVideoCrop(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }       

    /**
     * @description: set display window of the player
     * @param {Object} win display window
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setDisplayWindow(win, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            x: win.x,
            y: win.y,
            width: win.width,
            height: win.height
        }        
        this._interf.SetDisplayWindow(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }      

    /**
     * @description: enable/disable fullscreen of the player
     * @param {Boolean} onoff
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setFullScreen(onoff, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: onoff ? 1 : 0
        }        
        this._interf.SetFullScreen(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }  

    /**
     * @description: set display format of the player
     * @param {DisplayFormat} format display format
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setDisplayFormat(format, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: format
        }        
        this._interf.SetDisplayFormat(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    } 

    /**
     * @description: rotate video of the player
     * @param {RotateType} type
     * @param {Object} cb function(Number)
     * @return {*}
     */
    setVideoRotate(type, cb) {
        if (!this._interf) {
            this._doErrorCallback(cb, -1);
            return;
        }
        var params = { 
            type: this.type,
            value: type
        }        
        this._interf.SetVideoRotate(params, function(err, result) {
            this._doCallback(cb, err, result, -1);
        }.bind(this));
    }  
}

export { AuiPlayer, MediaType, PlayerEvent, PlayerStatus, DisplayFormat, RotateType };




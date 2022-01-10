import dbus from 'qjs-dbus'
import events from 'events'
import util from 'util'

var voiceLogEnableFlag = true;

export default function Voice(name) {
    voiceLOG("Voice Constructor");

    if (!(this instanceof Voice)) {
        console.log("what's this??");
        return new Voice(name);
    }
    
    events.EventEmitter.call(this);

    this.interfaceRevision = "v0.2.0";
    this.name = name;
    this.bus = null
    try {
      this.bus = dbus.getBus('system');
    } catch{}

    this.state = "";
    this.timeid = null;
    this._init();
}

util.inherits(Voice, events.EventEmitter)

Voice.prototype.destroy = function(cb) {
    this.timeid && clearTimeout(this.timeid);
    this.bus.disconnect();
}

Voice.prototype._init = function(cb) {
    var self = this;

    getVoiceDBusInterface(self.bus, oniface)

    function oniface(err, obj) {
        if (err) {
            self.timeid = setTimeout(function() {
                if (self.bus == null) {
                    try {
                      self.bus = dbus.getBus('system');
                    } catch{}
                }
                getVoiceDBusInterface(self.bus, oniface);
            }, 1000);
            return;
        }
        self._interf = obj;
        self._lisen(cb);
    }
}

Voice.prototype._lisen = function() {
    var self = this;

    this._interf.on('sessionBegin', function(props) {
        self.state = "busy";
        self.emit("sessionBegin", props);
        voiceLOG("get event SessionBegin");
        voiceLOG(props);
    })

    this._interf.on('sessionEnd', function(props) {
        self.state = "idle";
        self.emit("sessionEnd");
        voiceLOG("get event SessionEnd");
    })

    this._interf.on('VAD', function(props) {
        self.emit("vad");
        voiceLOG("get event VAD");
    })

    this._interf.on('ASRBegin', function(props) {
        self.emit("ASRBegin", props);
        voiceLOG("get event ASRBegin");
    })

    this._interf.on('ASRChanged', function(props) {
        self.emit("ASRChanged", props);
        voiceLOG("get event ASRChanged");
        voiceLOG(props);
    })

    this._interf.on('ASREnd', function(props) {
        self.emit("ASREnd", props);
        voiceLOG("get event ASREnd");
        voiceLOG(props);
    })

    this._interf.on('NLP', function(props) {
        self.emit("NLP", props);
        voiceLOG("get event NLP");
        voiceLOG(props);
    })

    this._interf.on('TTSBegin', function(props) {
        self.emit("TTSBegin", props);
        voiceLOG("get event TTSBegin");
        voiceLOG(props);
    })

    this._interf.on('TTSEnd', function(props) {
        self.emit("TTSEnd", props);
        voiceLOG("get event TTSEnd");
        voiceLOG(props);
    })
}

Voice.prototype.getInterfaceRevision = function() {
    return this.interfaceRevision;
}

Voice.prototype.start = function() {
    this._interf.start(function(err, result) {
        if (err) {
            voiceLOG(err);
            return -1;
        } else {
            return result;
        }
    });
}

Voice.prototype.stop = function() {
    this._interf.stop(function(err, result) {
        if (err) {
            voiceLOG(err);
            return -1;
        } else {
            return result;
        }
    });
}

Voice.prototype.getState = async function() {
    var self = this;

    async function p(){
      return new Promise(function(resolve, reject) {
          self._interf.getState(function(err, result) {
              if (err) {
                  voiceLOG(err);
                  return -1;
              } else {
                  resolve(result);
                  self.state = result;
              }
          });
      });
    }

    await p().then(function(state) {
        self.state = state;
    }).catch(function(){});

    return this.state;
}

Voice.prototype.getName = function() {
    return this.name;
}

Voice.prototype.PCMConfig = function() {

}

Voice.prototype.ALGConfig = function() {

}

Voice.prototype.logEnable = function(en) {
    if (en == true) {
        voiceLogEnableFlag = true;
    } else {
        voiceLogEnableFlag = false;
    }
}

Voice.prototype.NLP = function() {
    this._interf.NLP(function(err, result) {
        if (err) {
            voiceLOG(err);
            return -1;
        } else {
            return result;
        }
    });
}

Voice.prototype.TTS = function(text) {
    this._interf.TTS(text, function(err, result) {
        if (err) {
            voiceLOG(err);
            return -1;
        } else {
            return result;
        }
    });
}

Voice.prototype.play = function(text) {
    this._interf.play(text, function(err, result) {
        if (err) {
            voiceLOG(err);
            return -1;
        } else {
            return result;
        }
    });
}

function getVoiceDBusInterface (bus, cb) {
    var s = 'org.voice.server'
    var o = '/org/voice/path'
    var i = 'org.voice.interface'

    if (bus == null) {
        return cb("err");
    }

    bus.getInterface(s, o, i, oniface)

    function oniface (err, s) {
        if (err) {
            voiceLOG(err);
            return cb(err)
        }
        cb(err, s);
    }
}

function voiceLOG(out) {
    if (voiceLogEnableFlag == true) {
        console.log(out);
    }
}

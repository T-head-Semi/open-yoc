<template>
  <div v-if="current">
    <div class="video-wrapper" v-if="showVideo">
      <hole style="position:absolute;left:3px;top:0px;width:745px;height:390px;">
      </hole>
      <div class="btn-wrapper">
          <div class="btn-player">
            <text class="btn" @click="onClickOpen">Open file</text>
            <text class="btn" @click="onClickPlay">Play</text>
            <text class="btn" @click="onClickPause">Pause</text>
            <text class="btn" @click="onClickResume">Resume</text>
            <text class="btn" @click="onClickStop">Stop</text>
            <text class="btn" @click="onClickSpeed">{{ strSpeed }}</text>
            <text class="btn" @click="onClickVolAdd">VOL+</text>
            <text class="btn" @click="onClickVolSub">VOL-</text>
            <text class="btn" @click="onClickMute">{{ strMute }}</text>         
          </div>
      </div>
      <div class="bar-wrapper">
        <fl-seekbar v-model="curSec" v-bind="seekbarCfg" width="600" @change="onChangeSeek" />
        <text class="ptext">{{ time }}</text>
      </div>
      <div class="volume-wrapper">
        <fl-seekbar v-model="curSec" v-bind="volumebarCfg" width="50" />
      </div>
        <Toast
            ref="toast1"
            :style="{
            color: '#34495e',
            borderWidth: '1px',
            borderStyle: 'solid',
            borderColor: 'rgb(25,25,80)',
            backgroundColor: 'rgb(222,218,209)',
            marginBottom: '200px',
            }"
        />      
    </div>
    <div class="folder-wrapper" v-if="!showVideo">
      <scroller show-scrollbar="false">
        <div class="path-wrapper">
          <text class="file-name">Current dir:{{ curpath }}</text>
          <text class="btn-ret" @click="retp">Return</text>
        </div>
        <div v-if="update_dir">
          <div v-for="(file, index) in filepaths" :key="index" class="file-item" @click="chosefile(index, file)">
            <div class="file-flex-row">
              <text class="file-name" style="letter-spacing:0px;">{{ file }}</text>
            </div>
          </div>
        </div>
        <Toast
          ref="toast"
          :style="{
            color: '#34495e',
            borderWidth: '1px',
            borderStyle: 'solid',
            borderColor: 'rgb(25,25,80)',
            backgroundColor: 'rgb(222,218,209)',
            marginBottom: '200px',
          }"
        />
      </scroller>
    </div>
  </div>
</template>

<script>
import { AuiPlayer, MediaType, PlayerEvent, PlayerStatus, DisplayFormat, RotateType } from '../../../libs/AuiPlayer.js';
import { MediaService, MediaEvent } from '../../../libs/MediaService.js';
import { FlSeekbar } from "falcon-ui";
import { Toast } from "falcon-ui";

export default {
  props:{
    current:{
      type:Boolean,
      default:false
    }
  },
  components: { FlSeekbar, Toast },
  data() {
    return {
      mservice: MediaService.getInstance(),
      player: new AuiPlayer(MediaType.MEDIA, null), 
      fs: $falcon.jsapi.fs,
      speed: 1,
      strSpeed: "1.0X",
      mute: 0,
      strMute: "Mute",
      curSec: 0,
      showVideo: true,
      update_dir: true,
      time: "-0:00",
      speed: 1,
      strSpeed: "1.0X",
      vol: 60,
      seekbarCfg: Object.assign({
        length: 580,
        height: 15,
        handleSize: 40,
        inactiveColor: "#868FAB",
        activeColor: "#66CCFF",
        borderRadius: 5,
        min: 0,
        max: 100,
      }),
      volumebarCfg: Object.assign({
        length: 50,
        height: 15,
        handleSize: 30,
        inactiveColor: "#868FAB",
        activeColor: "#66CCFF",
        borderRadius: 5,
        min: 0,
        max: 100,
      }),
      curpath: "/mnt",
      filename:"",
      filepaths: [
         { path : "/bin"},
         { path : "/usr"},
         { path : "/lib"},
      ],
      filetypes: [],
    };
  },
  methods: {
    async onClickPlay() {
      this.mute = 0;
      this.mservice.unMute(null);
      this.strMute = "Mute";   
      this.player.play(this.filename, null);
    },    
    async onClickPause() {
      this.player.pause(null);
      if (this.timerId) {
        clearTimeout(this.timerId);
        this.timerId = 0;
      }      
    },
    async onClickResume() {
      this.player.resume(null);
      if (!this.timerId) {
        this.updateTime();
      }        
    },           
    async onClickSpeed() {
      var oldSpeed = this.speed;

      if (Math.abs(this.speed - 0.5) <= Number.EPSILON) {
          this.speed = 1;
      } else if (Math.abs(this.speed - 1) <= Number.EPSILON) {
          this.speed = 1.2;
      } else if (Math.abs(this.speed - 1.2) <= Number.EPSILON) {
          this.speed = 1.5;
      } else if (Math.abs(this.speed - 1.5) <= Number.EPSILON) {
          this.speed = 2;
      } else if (Math.abs(this.speed - 2) <= Number.EPSILON) {
          this.speed = 0.5;          
      }
      
      this.strSpeed = this.speed.toFixed(1) + "X";
      this.player.setSpeed(this.speed, function(result) {
          if (result < 0) {
            this.speed = oldSpeed;
            this.$refs.toast1.show("倍速播放失败");
            this.strSpeed = this.speed.toFixed(1) + "X";
          }         
      }.bind(this));           
    },
    doSetVol(vol) {
      var old_vol = this.vol;

      this.vol = vol > 100 ? 100 : vol;
      this.vol = vol < 0 ? 0 : vol;

      //this.player.setVol(this.vol, function(result) {
      this.mservice.setVol(this.vol, function(result) {
          if (result < 0) {
            this.$refs.toast1.show("音量设置失败");
            this.vol = old_vol;
          }         
      }.bind(this));        
    },       
    async onClickVolAdd() {
      this.doSetVol(this.vol + 10);    
    },    
    async onClickVolSub() {
      this.doSetVol(this.vol - 10);      
    },
    async onClickMute() {
      if (this.mute) {
        this.mute = 0;
        this.mservice.unMute(null);
      } else {
        this.mute = 1;
        this.mservice.mute(null);
      }
      this.strMute = this.mute ? "UnMute" : "Mute";
    },            
    doSeek(time) {
      if (this.timerSeek) {
        clearTimeout(this.timerSeek);
        this.timerSeek = 0;
      }

      this.player.seek(time * 1000, function(result) {
          if (result < 0) {
            this.$refs.toast1.show("跳转播放失败");
          }
          if (!this.timerId) {
            this.updateTime();
          }             
      }.bind(this));      
    },   

    async onChangeSeek(time) {
      if (this.timerSeek) {
        clearTimeout(this.timerSeek);
        this.timerSeek = 0;
      }     
      if (time < this.seekbarCfg.max) {
        //防止seek过程中seekbar乱跳
        this.curSec = time;
        if (this.timerId) {
            clearTimeout(this.timerId);
            this.timerId = 0;
        }          
        
        this.timerSeek = setTimeout(() => {
          this.doSeek(time);
        }, 800);
      }
    },     
    async onClickStop() {
      this.player.stop(null);
      this.time = "-0:00";
      this.curSec = 0;
      this.speed = 1;
      this.strSpeed = this.speed.toFixed(1) + "X";
      if (this.timerId) {
        clearTimeout(this.timerId);
        this.timerId = 0;
      }
      if (this.timerSeek) {
        clearTimeout(this.timerSeek);
        this.timerSeek = 0;
      }        
    },
    updateTime() {
      this.player.getTime(function(ptime) {
          if (ptime && ptime.duration > 0 && ptime.curtime >= 0) {
              var remain_sec = parseInt((ptime.duration - ptime.curtime) / 1000);
              this.curSec = parseInt(ptime.curtime / 1000);
              this.seekbarCfg.max = parseInt(ptime.duration / 1000);
              this.time = "-" + parseInt(remain_sec / 60) + ":" + (Array(2).join(0) + parseInt(remain_sec % 60)).slice(-2);
          }
      }.bind(this));
      
      if (this.curSec <= this.seekbarCfg.max) {
        this.timerId = setTimeout(() => {
          this.updateTime();
        }, 600);
      }
    },    
    async updatepath() {
      this.update_dir = false;
      var request = {
        path: this.curpath
      }
      var res = await this.fs.do_readdir(request);
      this.filepaths.length = 0;
      this.filetypes.length = 0;
      for(var file in res.result) {
         this.filepaths.push(file)
         this.filetypes.push(res.result[file])
      }
      this.update_dir = true;
    },
    async onClickOpen() {
      this.onClickStop();
      await this.updatepath();
      this.showVideo = false;
    },
    retp() {
      this.showVideo = true;
    },
    async chosefile(index, file) {
      var curl =/^url.txt/;  
      if (this.filetypes[index]) {
        var request = {
          path: this.curpath + "/" + file
        }
        var res = await this.fs.do_realpath(request);
        this.curpath = res.result;
        this.updatepath();
        return;
      } 
          if (curl.test(file)) {
            var request = {
              path: this.curpath + "/" + file
            }
            var res = await this.fs.do_getline(request);
            console.log(res);
            this.filename = res.result;
            console.log("=>>>>>>>>filename:", this.filename);
            this.showVideo = true;
          }
          else {
            console.log("=>>>>>>>>3");
            this.filename = "file://" + this.curpath + "/" + file;
            console.log("=>>>>>>>>filename:", this.filename);
            this.showVideo = true;
          }
    },
  },
  mounted() {
  },
  created() {
    this.strMute = this.mute ? "UnMute" : "Mute";
    this.player.on(PlayerEvent.ERROR, function() {
      console.log('#######vue PlayError: ');
      this.$refs.toast1.show("播放失败");
      this.onClickStop();  
    }.bind(this));

    this.player.on(PlayerEvent.START, function() {
      console.log('#######vue PlayStart: ');
      if (!this.timerId) {
        this.updateTime();
      }
      this.player.getVol(function(vol) {
          if (vol) {
              this.vol = vol;
          }
      }.bind(this));           
    }.bind(this));

    this.player.on(PlayerEvent.PAUSE, function() {
      console.log('#######vue PlayPause: ');
      if (this.timerId) {
        clearTimeout(this.timerId);
        this.timerId = 0;
      }     
    }.bind(this));

    this.player.on(PlayerEvent.RESUME, function() {
      console.log('#######vue PlayResume: ');
      if (!this.timerId) {
        this.updateTime();
      }       
    }.bind(this));
    
    this.player.on(PlayerEvent.STOP, function() {
      console.log('#######vue PlayStop: ');
      this.onClickStop();  
    }.bind(this));    

    this.player.on(PlayerEvent.FINISH, function() {
      console.log('#######vue PlayFinish: ');
      this.onClickStop();  
    }.bind(this));       
    
    this.player.on(PlayerEvent.TIME_UPDATE, function(duration, curtime) {
      console.log('#######vue TIME_UPDATE: ');
      console.log("duration: " + duration);
      console.log("curtime: " + curtime);
    }.bind(this));     

    this.player.on(PlayerEvent.MUTE, function(mute) {
      console.log('#######vue PlayMute: '); 
      this.mute = mute;
      this.strMute = this.mute ? "UnMute" : "Mute";
      this.$refs.toast1.show(this.mute ? "静音" : "取消静音");
    }.bind(this));

    this.player.on(PlayerEvent.VOL_CHANGE, function(oldVal, newVal) {
      console.log('#######vue VOL_CHANGE: '); 
      console.log("oldVal: " + oldVal);
      console.log("newVal: " + newVal);
    }.bind(this));    
  },  
  beforeDestroy () {
    return this.onClickStop();
  }
};

</script>

<style lang="less" scoped>
@import "./scene.less";
.video-wrapper {
  width: 760px;
  height: 400px;
  left:20px;
  align-items: center;
  justify-content: center;
  position: relative;
}

.folder-wrapper {
  width: 760px;
  height: 350px;
  left:20px;
  align-items: center;
  justify-content: center;
  position: relative;
}

.path-wrapper {
  width: 760px;
  height: 60px;
  left: 20px;
  flexDirection: row;
  justify-content: space-between;
}

.file-item{
  width: 760px;
  height: 68px;
  flexDirection: row;
  justifyContent: space-between;
  alignItems: center;
  marginTop: 5px;
  marginRight: 0;
  marginBottom: 5px;
  marginLeft: 0;
  paddingTop: 0px;
  paddingRight: 30px;
  paddingBottom: 0px;
  paddingLeft: 30px;
  borderRadius: 20px;
  backgroundColor: #141414;
};

.file-name{
  fontSize: 26px;
  fontWeight: bold;
  letterSpacing: 3px;
  paddingRight: 30px;
  color: #ffffff;
};

.file-flex-row{
  flexDirection: row;
}

.bar-wrapper{
  margin: 50px;
  width: 720px;
  height: 60px;
  background-color: transparent;
  align-self: flex-start;
  position: relative;
  bottom:30px;
  flex-direction: row;
}

.volume-wrapper {
  width: 60px;
  height: 70px;
  background-color: transparent;
  bottom:30px;
  transform: rotate(270deg);
}

.ptext{
  font-size: 22px;
  align-self: center;
  color: #ffffff;
}

.btn-wrapper {
  align-items: center;
  width: 720px;
  height: 480px;
  align-self: center;
  justify-content: flex-end;
  padding-top: 26px;
  padding-bottom: 8px;
  position: relative;
}

.btn-player {
  flex-direction: column;
  justify-content: flex-end;
  width: 150px;
  height: 460px;
  margin-bottom: 5px;
  align-self: flex-end;
}

.btn {
  background-color: #0d8bd5;
  border-radius: 2px;
  color: #ffffff;
  font-size: 13px;
  width: 120px;
  height: 28px;
  text-align: center;
  line-height: 28px;
  margin: 2px 2px 2px 2px;
}

.btn:active {
  background-color: #293343;
}

.btn-ret {
  background-color: #0d8bd5;
  border-radius: 10px;
  color: #ffffff;
  font-size: 22px;
  width: 120px;
  height: 50px;
  text-align: center;
  line-height: 50px;
  align-self: center;
  right: 20px;
}

</style>

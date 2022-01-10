<template>
    <div class="backgroud-wrapper">
      <div class="asr-text-wrapper">
        <div class="section">
          <text class="section-title">ASR</text>
          <div class="demo-block" style="flex-direction: row; flex-wrap: wrap; padding-left: 24px">
            <text style="lines: 3; font-size: 24px; color: #868fab"
              >{{text}}
            </text>
          </div>
        </div>
        <div class="btn-wrapper" v-if="current">
            <div class="btn-voice">
              <text class="btn" @click="startvoice">开始语音AI</text>
              <text class="btn" @click="stopvoice">停止语音AI</text>
              <text class="btn" @click="tts">文字转语音</text>
            </div>
        </div>
      </div>
    </div>
</template>

<script>
import voice from '../../../libs/voice.js'
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
  data() {
    return {
      text: "",
    };
  },
  methods: {
    async createvoice() {
      var self = this;

      this.voice = new voice("voice0");
      this.voice.logEnable(true);
      this.text = "你好啊！"

      this.voice.on("sessionBegin", (props) => {
        var option = {
          resume: 1,
          seek_time: 0
        };
        this.player.play("file:///mnt/wakeup_hello.mp3", option, null);
          // self.voice.play("/lib/firmware/message.wav");
        self.text = "天猫精灵";
      });

      this.voice.on("ASRBegin", (props) => {
        self.text = props;
      });

      this.voice.on("ASRChanged", (props) => {
        self.text = props;
      });

      this.voice.on("ASREnd", (props) => {
        self.text = props;
        self.asr_end_text = props;
      });

      this.voice.on("TTSEnd", (url) => {
        self.voice.play(url);
      });
    },
    async startvoice() {
      var res = this.voice.start();
      return res;
    },
    async stopvoice() {
      var res = this.voice.stop();
      return res;
    },
    async tts() {
      var res = this.voice.TTS(this.asr_end_text);
      return res;
    },
  },
  mounted() {
    return this.createvoice();
  },
  created() {
    this.mservice = MediaService.getInstance();
    this.player = new AuiPlayer(MediaType.SYSTEM, null);
    this.asr_end_text = "你好啊！";
  },
  beforeDestroy () {
    this.voice.destroy();
    this.player.destroy();
  }
};

</script>

<style lang="less" scoped>
@import "./scene.less";
// @import "../../../style/base.less";
.btn-wrapper {
  align-items: center;
  width: 700px;
  height: 220px;
  align-self: center;
  justify-content: flex-end;
  padding-top: 26px;
  padding-bottom: 18px;
  position: relative;
}

.btn-voice {
  flex-direction: column;
  justify-content: flex-end;
  width: 150px;
  height: 320px;
  margin-bottom: 10px;
  align-self: flex-end;
  padding-top: 10px;
  padding-bottom: 10px;
}

.btn {
  background-color: #0d8bd5;
  border-radius: 10px;
  color: #ffffff;
  font-size: 18px;
  width: 100px;
  height: 40px;
  text-align: center;
  line-height: 40px;
  margin: 5px 5px 5px 5px;
}

.btn:active {
  background-color: #293343;
}

.backgroud-wrapper {
//  background-image: url("../images/t-head_logo.png");
  align-items: center;
  justify-content: center;
  position: relative;
}

.asr-text-wrapper{
  width:700px;
  min-height:385px;
  align-self: center;
  background-color: #2C3543;
  border-radius: 32px;
  padding-bottom: 24px;
  margin-bottom:40px;
}


.color-item-wrap {
  flex-direction: row;
  margin: 10px 24px;
}
.color-demo-cycle {
  width: 36px;
  height: 36px;
  border-radius: 18px;
  margin-right: 12px;
}
.color-demo-text {
  font-size: 24px;
  line-height: 36px;
}

</style>

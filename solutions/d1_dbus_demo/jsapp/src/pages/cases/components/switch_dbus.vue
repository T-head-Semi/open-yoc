<template>
  <div class="scene-wrapper" v-if="current">
    <div class="switch-button-wrap" @click="toggleSwitch">
      <image class="switch-button" resize="contain" :src="require('../images/off.png')"  />
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
  </div>
</template>

<script>
import { DSwitch } from '../../../libs/DSwitch.js';
import { FlIcon, FlPopup } from "falcon-ui";
import { Toast } from "falcon-ui";
export default {
  components: {
    FlIcon,
    FlPopup,
    Toast,
  },
  props:{
    current:{
      type:Boolean,
      default:false
    }
  },
  data() {
    return {
    };
  },
  created() {
    this.dswitch      = new DSwitch();
    this.switchStatus = 0;
    this.dswitch.on("SwitchEvent", function(obj) {
      console.log('#######vue event: ' + obj.status);
      this.switchStatus = parseInt(obj.status);
	  var strStatus = this.switchStatus == 1 ? "已打开" : "已关闭";
      this.$refs.toast.show(strStatus);
    }.bind(this));
  },

  beforeDestroy () {
    console.log('#######vue destory');
    this.dswitch.destroy();
  },

  methods: {
    toggleSwitch() {
      this.switchStatus = this.switchStatus ? 0 : 1;
      this.dswitch.doSwitch(this.switchStatus, null);
    },
  }
};
</script>

<style lang="less" scoped>
@import "./scene.less";
.scene-wrapper {
  position: relative;
  justify-content: flex-start;
  padding-top:26px;
}
.switch-button-wrap{
  border-radius:22px;
  width:78px;
  height:78px;
  border-radius:50px;
  position: absolute;
  right: 18px;
  top: 18px;
  align-items: center;
  justify-content: center;

  opacity: 0.9;
  background-color: @background-color;
}
.switch-button-wrap:active{
  // background-color: rgba(255,255,255, 0.2);
  opacity: 0.6;
}
.switch-button {
  width: 44px;
  height: 44px;
}
.speed-slider-wrapper {
  position: relative;
  width: 300px;
  align-items: center;
  align-self: center;
}
.speed-slider {
  width: 100%;
  height: 60px;
}
.speed-item {
  color: @text-color;
  opacity: 0.2;
  font-size: 28px;
  width: 100px;
  line-height: 60px;
  text-align: center;
  font-weight: bold;
}
.speed-item-selected {
  opacity: 1;
  font-size: 32px;
}
.label-fan {
  color: @text-color;
  opacity: 0.6;
}
// .speed-slider-cover{
//   position:absolute;
//   .bg-linear-gradient(to right; rgba(255,255,255,0.4) 0%, rgba(255,255,255,0) 30%,rgba(255,255,255,0) 70%, rgba(255,255,255,0.4) 100%);
//   width:100%;
//   height:100%;
// }
.temperature-wrap {
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
  padding: 0 60px;
  margin-top: 10px;
}
.temperature-ctrl-wrap {
  width: 100px;
  height: 100px;
  align-items: center;
  justify-content: center;
  border-radius:50px;

  opacity: 0.9;
  background-color: @background-color;
}
.temperature-ctrl-wrap:active{
  // background-color: rgba(255,255,255, 0.2);
  opacity: 0.6;
}
.ctrl-minus {
  width: 38px;
  height: 10px;
}
.ctrl-plus {
  width: 46px;
  height: 46px;
}
.label-temperature-wrapper {
  flex-direction: row;
  margin-left:35px;
}
.label-temperature {
  font-size: 120px;
  color: @text-color;
}
.label-temperature-unit {
  font-size: 30px;
  color: @text-color;
  padding-top: 30px;
}
.mode-slider {
  width: 260px;
  height: 50px;
  margin-top: 16px;
  align-self: center;
}
.mode-slider-image {
  width: 44px;
  height: 44px;
  opacity: 0.2;
}
.mode-item-selected {
  opacity: 1;
}
.air-cover-wrapper {
  width: 100%;
  height: 380px;
  padding-top: 35px;
  align-items: center;
  justify-content: flex-start;
  background-color: rgba(0, 0, 0, 0.95);
  position:absolute;
  border-radius: 24px;
  left:0;
  top:0;
}
.air-pop-title {
  color: @white;
  font-size: 36px;
  text-align: center;
}
.switch-button-big {
  color: @white;
  margin-top: 70px;
  font-weight: bold;
  width:129px;
  height:132px;
}
</style>

<template>
  <div class="scene-wrapper" v-if="current">
    <div class="switch-button-wrap" @click="toggleSwitch">
      <image class="switch-button" resize="contain" :src="require('../images/off.png')"  />
    </div>
    <div class="speed-slider-wrapper">
      <slider
        class="speed-slider"
        infinite="true"
        :index="speedIndex"
        @change="speedChanged"
        previousMargin="100px"
        nextMargin="100px"
        :enableAcceleration="true"
      >
        <text
          v-for="(item, index) in ['Low', 'Mid', 'High']"
          @click="speedItemClick(index)"
          :class="'speed-item' + (index === speedIndex ? ' speed-item-selected' : '')"
          :key="'speed' + index"
          >{{ item }}</text
        >
      </slider>
      <!-- <div class="speed-slider-cover"></div> -->
      <text class="label-fan">Fan</text>
    </div>
    <div class="temperature-wrap">
      <div class="temperature-ctrl-wrap" @click="tempMinus">
        <image class="temperature-ctrl ctrl-minus" resize="contain" :src="require('../images/minus.png')" />
      </div>
      <div class="label-temperature-wrapper">
        <text class="label-temperature">{{ temperatureCurrent }}</text>
        <text class="label-temperature-unit">℃</text>
      </div>
      <div class="temperature-ctrl-wrap" @click="tempPlus">
        <image class="temperature-ctrl ctrl-plus" resize="contain" :src="require('../images/plus.png')" />
      </div>
    </div>
    <slider
      class="mode-slider"
      infinite="true"
      :index="modeIndex"
      @change="modeChanged"
      previousMargin="80px"
      nextMargin="80px"
      :enableAcceleration="true"
    >
      <img
        v-for="(item, index) in modes"
        @click="modeItemClick(index)"
        :key="'mode' + index"
        :src="modes[index]"
        :class="'mode-slider-image' + (index === modeIndex ? ' mode-item-selected' : '')"
        resize="contain"
      />
    </slider>
    
    <div class="air-cover-wrapper" v-if="airClose">
      <text class="air-pop-title">Air Conditioning</text>
      <image class="switch-button-big" resize="contain" :src="require('../images/off.png')" @click="toggleSwitch" />
    </div>
  </div>
</template>

<script>
import { FlIcon, FlPopup } from "falcon-ui";
export default {
  components: {
    FlIcon,
    FlPopup
  },
  props:{
    current:{
      type:Boolean,
      default:false
    }
  },
  data() {
    return {
      speedIndex: 0,
      modeIndex: 0,
      temperatureCurrent: 25,
      modes: [require("../images/sun.png"), require("../images/snow.png"), require("../images/water.png")],
      airClose: false
    };
  },
  methods: {
    toggleSwitch() {
      setTimeout(() => {
        this.airClose = !this.airClose;
      });
    },
    speedChanged(e) {
      this.speedIndex = e.index;
    },
    speedItemClick(index) {
      this.speedIndex = index;
    },
    modeChanged(e) {
      this.modeIndex = e.index;
    },
    modeItemClick(index) {
      this.modeIndex = index;
    },
    tempMinus() {
      if (this.temperatureCurrent > 16) {
        this.temperatureCurrent--;
      }
    },
    tempPlus() {
      if (this.temperatureCurrent < 32) {
        this.temperatureCurrent++;
      }
    }
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

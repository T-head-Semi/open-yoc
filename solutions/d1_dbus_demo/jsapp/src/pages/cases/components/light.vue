<template>
  <div class="scene-wrapper" v-if="current">
    <div class="switch-button-wrap" @click="toggleSwitch">
      <image class="switch-button" resize="contain" :src="require('../images/off.png')"  />
    </div>
    <div class="mode-switch">
      <div class="mode-item" @click="switchMode(0)">
        <text :class="'switch-text' + (mode === 0 ? ' switch-text-selected' : '')">White</text>
        <div :class="'item-under' + (mode === 0 ? ' item-under-selected' : '')"></div>
      </div>

      <div class="mode-item" @click="switchMode(1)">
        <text :class="'switch-text' + (mode === 1 ? ' switch-text-selected' : '')">Color</text>
        <div :class="'item-under' + (mode === 1 ? ' item-under-selected' : '')"></div>
      </div>
    </div>

    <div class="brightness-wrap">
      <div class="brightness-text-wrap">
        <text class="brightness-title">Brightness</text>
        <text class="brightness-percent">{{ brightnessPercent }}%</text>
      </div>
      <div class="brightness-seek-wrap">
        <div class="brightness-seek-scale-wrap">
          <div class="brightness-seek-scale-item" v-for="(item, index) in 10" :key="'scale-item' + index"></div>
        </div>
        <fl-seekbar
          v-model="brightnessPercent"
          class="brightness-seekbar"
          :length="660"
          :height="70"
          :handleSize="60"
          inactiveColor="transparent"
          activeColor="transparent"
          handleBorderColor="#a5a5a5"
        />
      </div>
    </div>

    <div class="color-wrap" v-if="mode === 0">
      <div class="color-text-wrap">
        <text class="color-title">Color temperature</text>
        <text class="color-percent">{{ colorTemperaturePercent }}%</text>
      </div>
      <div class="color-seek-wrap">
        <div class="white-seek-scale-wrap">
          <fl-seekbar
            v-model="colorTemperaturePercent"
            class="brightness-seekbar"
            :length="660"
            :height="92"
            :handleSize="60"
            inactiveColor="transparent"
            activeColor="transparent"
            handleBorderColor="#a5a5a5"
          />
        </div>
      </div>
    </div>
    <div class="color-wrap" v-else>
      <div class="color-text-wrap">
        <text class="color-title">Color</text>
      </div>
      <div class="color-seek-wrap">
        <div class="color-seek-scale-wrap">
          <fl-seekbar
            v-model="colorPercentPercent"
            class="brightness-seekbar"
            :length="660"
            :height="92"
            :handleSize="60"
            inactiveColor="transparent"
            activeColor="transparent"
            handleBorderColor="#a5a5a5"
          />
        </div>
      </div>
    </div>

    <div class="light-cover-wrapper" v-if="lightClose">
      <text class="light-pop-title">Smart Lights</text>
      <image class="switch-button-big" resize="contain" :src="require('../images/off.png')" @click="toggleSwitch" />
    </div>
  </div>
</template>

<script>
import { FlIcon, FlPopup, FlSeekbar } from "falcon-ui";
export default {
  components: {
    FlIcon,
    FlPopup,
    FlSeekbar
  },
  props:{
    current:{
      type:Boolean,
      default:false
    }
  },
  data() {
    return {
      mode: 0,
      lightClose: false,
      brightnessPercent: 60,
      colorTemperaturePercent: 30,
      colorPercentPercent: 50
    };
  },
  methods: {
    switchMode(mode) {
      this.mode = mode;
    },

    toggleSwitch() {
      setTimeout(() => {
        this.lightClose = !this.lightClose;
      }, 0);
    }
  }
};
</script>

<style lang="less" scoped>
@import "./scene.less";
.scene-wrapper {
  justify-content: flex-start;
  padding-top: 26px;
  padding-bottom: 18px;
  position: relative;
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
  z-index: 10;

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
.mode-switch {
  flex-direction: row;
  padding-left: 36px;
}
.mode-item {
  flex-direction: column;
  align-items: center;
  margin-right: 30px;
}
.switch-text {
  color: @text-color;
  opacity: 0.4;
  font-size: 36px;
  font-weight: bold;
}
.switch-text-selected {
  opacity: 1;
}
.item-under {
  background-color: transparent;
  height: 5px;
  width: 33px;
  margin-top: 4px;
  border-radius: 3px;
}
.item-under-selected {
  background-color: #ff6a00;
}

.light-cover-wrapper {
  width: 100%;
  height: 380px;
  padding-top: 35px;
  align-items: center;
  background-color: rgba(0, 0, 0, 0.95);
  position:absolute;
  border-radius: 24px;
  z-index:11;
  justify-content: flex-start;
  top:0;
}
.light-pop-title {
  color: @white;
  font-size: 36px;
  text-align: center;
}
.switch-button-big {
  color: @white;
  margin-top: 70px;
  width:129px;
  height:132px;
}

.color-wrap,
.brightness-wrap {
  margin-top: 35px;
}
.color-text-wrap,
.brightness-text-wrap {
  flex-direction: row;
  margin: 0 36px 20px 36px;
  justify-content: space-between;
}
.brightness-text-wrap{
  margin-bottom:10px;
}
.color-title,
.brightness-title {
  font-size: 20px;
  color: @text-color;
}
.color-percent,
.brightness-percent {
  font-size: 20px;
  color: @text-color;
}
.color-seek-wrap,
.brightness-seek-wrap {
  position: relative;
}
.color-seek-scale-wrap,
.white-seek-scale-wrap,
.brightness-seek-scale-wrap {
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  height:30px;
  margin: 20px 36px;
}
.brightness-seek-scale-item {
  width: 3px;
  height: 12px;
  background-color: @text-color;
  opacity: 0.4;
}
.color-seekbar,
.brightness-seekbar {
  position: absolute;
  left: 0;
  top: 0;
}

.color-wrap {
  margin-top: 22px;
}

.white-seek-scale-wrap,
.color-seek-scale-wrap {
  width: 720px;
  height: 98px;
  background-image: linear-gradient(
    268deg,
    #ffbf00 0%,
    #ff8000 8%,
    #ffff00 17%,
    #beff00 21%,
    #80ff00 25%,
    #00ff00 33%,
    #00ff80 42%,
    #00ffff 50%,
    #0080ff 58%,
    #0000ff 67%,
    #8000ff 75%,
    #ff00ff 83%,
    #ff0000 100%,
    #ff0080 100%
  );
  border-top-left-radius: 0;
  border-top-right-radius: 0;
  border-bottom-left-radius: 24px;
  border-bottom-right-radius: 24px;
  margin: 0;
}
.white-seek-scale-wrap {
  background-image: linear-gradient(90deg, #159CE6 0%, #FFFFFF 48%, #DE9B30 100%);
}
</style>

<!-- 指针表盘 -->
<template>
  <div class="clock-wrapper">
    <div class="weather-wrap">
      <image class="weather-icon" :src="require('../images/cloudy.png')" />
      <text class="weather-text">18℃</text>
    </div>

    <div class="date-wrapper">
      <text class="date-text">{{ dateText }}</text>
    </div>
    <div class="clock-scale clock-scale0" />
    <div class="clock-scale clock-scale1" />
    <div class="clock-scale clock-scale2" />
    <div class="clock-scale clock-scale3" />
    <div class="clock-scale clock-scale4" />
    <div class="clock-scale clock-scale5" />
    <div class="clock-scale clock-scale6" />
    <div class="clock-scale clock-scale7" />
    <div class="clock-scale clock-scale8" />
    <div class="clock-scale clock-scale9" />
    <div class="clock-scale clock-scale10" />
    <div class="clock-scale clock-scale11" />

    <div class="pointer-hour" :style="hStyle" />
    <div class="pointer-minute" :style="mStyle" />
    <div class="pointer-second" :style="sStyle" />

    <!-- 指针上的圆环 -->
    <div class="pointer-cover">
      <div class="pointer-cover-inner" />
    </div>
  </div>
</template>
<script>
const WEEKDAYS = ["日", "一", "二", "三", "四", "五", "六"];
export default {
  data() {
    const degs = this.caculateDeg();
    return {
      ...degs
    };
  },
  created() {
    this.$page.on("show", this.onPageShow);
    this.$page.on("hide", this.onPageHide);
  },
  mounted() {
    this.updateTime();
  },
  destroyed() {
    this.timerId && clearTimeout(this.timerId);
    this.$page.off("show", this.onPageShow);
    this.$page.off("hide", this.onPageHide);
  },
  computed: {
    dateText() {
      const now = new Date();
      const month = String(now.getMonth() + 1).padStart(2, "0");
      const date = String(now.getDate()).padStart(2, "0");
      const day = WEEKDAYS[new Date().getDay()];

      return `${month}.${date} 周${day}`;
    },
    hStyle() {
      return {
        transform: `rotate(${this.hours}deg)`
      };
    },
    mStyle() {
      return {
        transform: `rotate(${this.minutes}deg)`
      };
    },
    sStyle() {
      return {
        transform: `rotate(${this.seconds}deg)`
      };
    }
  },
  methods: {
    onPageShow() {
      if (!this.timerId) {
        this.updateTime();
      }
    },
    onPageHide() {
      if (this.timerId) {
        clearTimeout(this.timerId);
        this.timerId = 0;
      }
    },
    updateTime() {
      this.timerId && clearTimeout(this.timerId);
      const result = this.caculateDeg();
      this.hours = result.hours;
      this.minutes = result.minutes;
      this.seconds = result.seconds;
      this.timerId = setTimeout(() => {
        this.updateTime();
      }, 1000 - (Date.now() % 1000));
    },
    caculateDeg() {
      const now = new Date();
      const hours = now.getHours() % 12;
      const minutes = now.getMinutes();
      const seconds = now.getSeconds();

      const sDeg = seconds * 6;
      const mDeg = (minutes * 60 + seconds) * 0.1; //每一秒走0.1度
      const hDeg = ((hours % 12) * 60 + minutes) * 0.5; //每分钟走0.25度
      return {
        hours: parseInt(hDeg),
        minutes: parseInt(mDeg),
        seconds: parseInt(sDeg)
      };
    }
  }
};
</script>
<style lang="less" scoped>
@import "./scene.less";
.clock-wrapper {
  position: relative;
  left:0;
  top:0;
}
.weather-wrap{
  position:absolute;
  flex-direction: row;
  left:96px;
  top:166px;
}
.weather-icon{
  width: 51px;
  height: 51px;
  margin-right:6px;
}
.weather-text{
  color: @white;
  font-size:30px;
  line-height: 51px;;
}
.date-wrapper{
  position:absolute;
  right:83px;
  top:166px;
}
.date-text{
  color: @white;
  font-size:30px;
}
.pointer-cover {
  width: 16px;
  height: 16px;
  position: absolute;
  left: 292px;
  top: 182px;
  border-radius: 8px;
  background-color: #000000;
  align-items: center;
  justify-content: center;
}
.pointer-cover-inner {
  width: 12px;
  height: 12px;
  background-color: #ff0000;
  border-radius: 6px;
}
.pointer-hour,
.pointer-minute,
.pointer-second {
  position: absolute;
  background-color: @white;
  box-shadow: 0px 0px 6px #000;
}
.pointer-hour {
  width: 30px;
  height: 120px;
  left: 285px;
  top: 85px;
  border-radius: 15px;
  transform-origin: 50% 105px;
}
.pointer-minute {
  width: 20px;
  height: 180px;
  left: 290px;
  top: 20px;
  border-radius: 10px;
  transform-origin: 50% 170px;
}
.pointer-second {
  width: 4px;
  height: 195px;
  left: 298px;
  top: 6px;
  background-color: #ff0000;
  border-radius: 3px;
  transform-origin: 50% 184px;
}
.clock-scale {
  width: 10px;
  background-color: @white;
  border-radius: 24px;
  position: absolute;
}
.clock-scale0 {
  height: 24px;
  left: 295px;
  top: 25px;
}
.clock-scale1 {
  height: 32px;
  left: 381px;
  top: 25px;
  transform: rotate(30deg);
}
.clock-scale2 {
  height: 36px;
  right: 40px;
  top: 25px;
  transform: rotate(60deg);
}
.clock-scale3 {
  height: 24px;
  right: 25px;
  top: 178px;
  transform: rotate(90deg);
}

.clock-scale4 {
  height: 36px;
  right: 40px;
  bottom: 25px;
  transform: rotate(120deg);
}
.clock-scale5 {
  height: 32px;
  left: 381px;
  bottom: 25px;
  transform: rotate(150deg);
}
.clock-scale6 {
  height: 24px;
  left: 295px;
  bottom: 25px;
  transform: rotate(180deg);
}
.clock-scale7 {
  height: 36px;
  left: 210px;
  bottom: 25px;
  transform: rotate(210deg);
}
.clock-scale8 {
  height: 32px;
  left: 37px;
  bottom: 25px;
  transform: rotate(240deg);
}
.clock-scale9 {
  height: 24px;
  left: 25px;
  top: 178px;
  transform: rotate(270deg);
}
.clock-scale10 {
  height: 36px;
  left: 41px;
  top: 25px;
  transform: rotate(300deg);
}
.clock-scale11 {
  height: 32px;
  left: 209px;
  top: 25px;
  transform: rotate(330deg);
}
</style>

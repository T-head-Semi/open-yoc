<!-- 环形表盘 -->
<template>
  <div class="clock-wrapper">
    <div style="position:absolute;left:0; top:0;border-radius:24px;">
      <FlLottie ref="bg_lottie" :width="600" :height="380" :animationData="lottieData" :loopCount="1" :autoplay="false" />
    </div>
    <text class="weather-text">晴 25℃</text>
    <div class="inner-cycle" ref="innerCycle">
      <text class="inner-hours">{{ hours }}</text>
      <text class="inner-date">{{ month }}.{{ date }} 周{{ day }}</text>
    </div>
    <div class="minutes-wrapper" :style="minutesStyle">
      <text class="minutes">{{ minutes }}</text>
    </div>
  </div>
</template>
<script>
import { FlLottie } from "falcon-ui";
import LOTTIE_SUMSHINE from '../movies/sunshine.json';
const WEEKDAYS = ["日", "一", "二", "三", "四", "五", "六"];
export default {
  components: { FlLottie },
  props:{
    current:{
      type:Boolean,
      default:false
    }
  },
  data() {
    const now = new Date();
    return {
      now
    };
  },
  created() {
    this.$page.on("show", this.onPageShow);
    this.$page.on("hide", this.onPageHide);
  },
  destroyed() {
    this.timerId && clearTimeout(this.timerId);
    this.$page.off("show", this.onPageShow);
    this.$page.off("hide", this.onPageHide);
  },
  mounted() {
    this.updateTime();
  },
  computed: {
    lottieData() {
      return LOTTIE_SUMSHINE;
    },
    hours() {
      return String(this.now.getHours()).padStart(2, "0");
    },
    minutes() {
      return this.now.getMinutes();
    },
    month() {
      return String(this.now.getMonth() + 1).padStart(2, "0");
    },
    date() {
      return String(this.now.getDate()).padStart(2, "0");
    },
    day() {
      return WEEKDAYS[this.now.getDay()];
    },
    minutesStyle() {
      if (this.now && !this.$refs.innerCycle) {
        return {
          opacity: 0
        };
      }
      const RADIUS = 146; //外圈的半径
      const minutes = this.now.getMinutes();
      const seconds = this.now.getSeconds();

      const mDeg = (minutes * 60 + seconds) * 0.1 - 90; //每一秒走0.1度,并且换算成x轴正方向为0度(3点方向)
      const mRadian = (mDeg / 180) * Math.PI; //
      let top = Math.sin(mRadian) * RADIUS;
      let left = Math.cos(mRadian) * RADIUS;
      return {
        left: `${left}px`,
        top: `${top}px`
      };
    }
  },
  watch: {
    current(newVal, oldVal) {
      if (newVal) {
        this.$refs.bg_lottie.startAnim();
      }
    }
  },
  methods: {
    updateTime() {
      this.timerId && clearTimeout(this.timerId);
      this.now = new Date();
      const updateInterval = 1000 * 60; //一分钟更新一次
      const nextUpdate = updateInterval - (Date.now() % updateInterval);
      this.timerId = setTimeout(() => {
        this.updateTime();
      }, nextUpdate);
    },
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
    }
  }
};
</script>
<style lang="less" scoped>
@import "./scene.less";
.clock-wrapper {
  // background-image: url("../images/cycle-time-bg.png");
  align-items: center;
  justify-content: center;
  position: relative;
}
.weather-text {
  position: absolute;
  left: 24px;
  top: 16px;
  font-size: 30px;
  color: @white;
  letter-spacing: 1.36px;
}
.inner-cycle {
  width: 292px;
  height: 292px;
  .border(2px, @white, 146px);
  justify-content: center;
  align-items: center;
  position: absolute;
  left: 154px;
  top: 44px;
}
.inner-hours {
  font-size: 150px;
  color: @white;
  margin-top: -10px;
}
.inner-date {
  font-size: 24px;
  color: @white;
  margin-top: -18px;
}
.minutes-wrapper {
  width: 80px;
  height: 80px;
  background-color: @white;
  border-radius: 40px;
  align-items: center;
  justify-content: center;
  position: relative;
}
.minutes {
  font-size: 44px;
  color: #ff2100;
}
</style>

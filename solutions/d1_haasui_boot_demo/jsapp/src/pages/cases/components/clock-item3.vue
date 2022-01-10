<!-- 数字表盘-->
<template>
  <div class="clock-wrapper">
    <simple-time
      class="inner-timer"
      :twinkling="true"
      :textStyle="{ fontSize: '160px', color: '#fff' }"
      :colonStyle="{ width: '60px', textAlign: 'center' }"
    />
    <text class="inner-date">{{ month }}月{{ date }}日 星期{{ day }}</text>
  </div>
</template>
<script>
import LOTTIE_WINDOW from "../movies/window.json";
import SimpleTime from "@/components/SimpleTime.vue";
const WEEKDAYS = ["日", "一", "二", "三", "四", "五", "六"];
export default {
  components: { SimpleTime },
  props: {
    current: {
      type: Boolean,
      default: false
    }
  },
  data() {
    return {};
  },
  watch: {
    current(newVal, oldVal) {
      if (newVal) {
        this.$refs.bg_lottie.startAnim();
      }
    }
  },
  computed: {
    lottieData() {
      return LOTTIE_WINDOW;
    },
    month() {
      return new Date().getMonth() + 1;
    },
    date() {
      return new Date().getDate();
    },
    day() {
      return WEEKDAYS[new Date().getDay()];
    }
  }
};
</script>
<style lang="less" scoped>
@import "./scene.less";
.clock-wrapper {
  background-image: url("../images/digital-time-bg.png");
  align-items: center;
}
.inner-timer {
  margin-top: 53px;
  align-self: center;
}
.inner-date {
  font-size: 30px;
  color: @white;
}
</style>

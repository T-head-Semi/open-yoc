<template>
  <div class="time-wrapper">
    <text class="text" :style="textStyle">{{ hours }}</text>
    <text class="text" :style="Object.assign({}, textStyle, colonStyle)">{{
      showColon ? ":" : ""
    }}</text>
    <text class="text" :style="textStyle">{{ minutes }}</text>
  </div>
</template>
<script>
export default {
  props: {
    twinkling: {
      type: Boolean,
      default: false,
    },
    textStyle: {
      type: Object,
      default: () => {},
    },
    colonStyle: {
      type: Object,
      default: () => {},
    },
  },
  data() {
    const now = new Date();
    return {
      now,
      showColon: true,
    };
  },
  created() {
    this.$page.on("show", this.onPageShow);
    this.$page.on("hide", this.onPageHide);
    this.timerId = 0;
  },
  mounted() {
    if (!this.timerId) {
      this.updateTime();
    }
  },
  destroyed() {
    this.timerId && clearTimeout(this.timerId);
    this.$page.off("show", this.onPageShow);
    this.$page.off("hide", this.onPageHide);
  },
  computed: {
    hours() {
      const hour = String(this.now.getHours());
      return hour.padStart(2, "0");
    },
    minutes() {
      const minutes = String(this.now.getMinutes());
      return minutes.padStart(2, "0");
    },
  },
  watch: {},
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
      this.now = new Date();
      const updateInterval = this.twinkling ? 1000 : 1000 * 60;
      this.showColon = this.twinkling ? !this.showColon : true;
      const nextUpdate = updateInterval - (Date.now() % updateInterval);
      this.timerId = setTimeout(() => {
        this.updateTime();
      }, nextUpdate);
    },
  },
};
</script>
<style lang="less" scoped>
@import "base.less";
.time-wrapper {
  flex-direction: row;
}
.hours {
}
.minutes {
}
.colon {
  width: 20px;
}
.text {
  color: @text-color;
}
</style>

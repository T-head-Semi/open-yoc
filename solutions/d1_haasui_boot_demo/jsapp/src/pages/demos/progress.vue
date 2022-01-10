<template>
  <demo-scroller padding="0px">
    <div class="main-wrapper">

      <div class="section">
        <text class="section-title">基础样式</text>
        <div class="demo-block">
          <fl-progress class="progress" percent="50" />
          <fl-progress class="progress" stroke-color="white" percent="60" />
        </div>
      </div>

      <div class="section">
        <text class="section-title">渐变进度</text>
        <div class="demo-block">
          <fl-progress class="progress" stroke-color="#FFFFFF,#FF6A00" percent="70" />
          <fl-progress class="progress" stroke-color="#FF6A00,#FFFFFF" percent="80" />
        </div>
      </div>

    </div>
  </demo-scroller>
</template>
<script>
import { FlProgress, FlButton } from "falcon-ui";

export default {
  name: "Progress",
  components: {
    FlProgress,
    FlButton
  },
};
</script>
<style lang="less" scoped>
@import "base.less";
.progress {
  width: 100%;
}
.percent-text {
  .text();
  margin-left: 6px;
  width: 50px;
}
</style>

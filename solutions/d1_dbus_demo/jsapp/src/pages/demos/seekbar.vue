<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">基础样式</text>
        <div class="demo-block">
          <fl-seekbar v-model="value" v-bind="seekbarCfg" :length="672-44" />
        </div>
      </div>
      <div class="section">
        <text class="section-title">块状样式</text>
        <div class="demo-block">
          <fl-seekbar v-model="valueBlock" v-bind="seekbarCfg" useBlockStyle :length="672" />
        </div>
      </div>
      <div class="section">
        <text class="section-title">块状样式(纵向)</text>
        <div class="demo-block">
          <fl-seekbar v-model="valueVertical" v-bind="seekbarCfg" useBlockStyle direction="vertical" :length="400" />
        </div>
      </div>
    </div>
  </demo-scroller>
</template>
<script>
import { FlSeekbar } from "falcon-ui";

export default {
  name: "SeekbarDemo",
  components: { FlSeekbar },
  data() {
    return {
      value: 36,
      valueBlock: 36,
      valueVertical: 36,
      seekbarCfg: { min: 0, max: 100 },
    };
  },
  mounted() {},
  methods: {}
};
</script>
<style lang="less" scoped>
@import "base.less";
</style>

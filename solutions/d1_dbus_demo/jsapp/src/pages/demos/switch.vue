<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">基础样式</text>
        <div class="demo-block demo-row">
          <fl-switch v-model="value11" />
          <fl-switch v-model="value12" />
        </div>
      </div>

      <div class="section">
        <text class="section-title">卡片样式</text>
        <div class="demo-block demo-row">
          <SwitchCard v-model="value21" />
          <SwitchCard v-model="value22" />
        </div>
      </div>

    </div>
  </demo-scroller>
</template>

<script>
import { FlSwitch } from "falcon-ui";
import SwitchCard from '../../components/SwitchCard.vue'

export default {
  components: { FlSwitch, SwitchCard },
  data() {
    return {
      value11: true,
      value12: false,

      value21: true,
      value22: false,

      payValue: true,
      switchChecked: false,
      extendValue: 0
    };
  },
};
</script>

<style lang="less" scoped>
@import "base.less";
</style>

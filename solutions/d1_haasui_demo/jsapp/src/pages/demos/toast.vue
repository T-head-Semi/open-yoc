<template>
  <demo-scroller>
    <div class="main-wrapper" padding="0px">
      <div class="section">
        <text class="section-title">基础样式</text>
        <div class="demo-block" style="row-gap: 72px;">
          <fl-button @click="toast" style="width: 100%;">顶部提示</fl-button>
          <fl-button @click="toast2" style="align-self: center;">居中提示</fl-button>
        </div>
      </div>
    </div>
    <Toast ref="toast" style="width: 100%;" position="top" />
    <Toast ref="toast2" position="middle" />
  </demo-scroller>
</template>

<script>
import { Toast } from "falcon-ui";
import { FlButton } from "falcon-ui";

export default {
  components: { Toast, FlButton },
  methods: {
    toast() {
      this.$refs.toast.show("顶部提示");
    },
    toast2() {
      this.$refs.toast2.show("居中提示");
    },
  },
};
</script>
<style lang="less" scoped>
@import "base.less";
</style>

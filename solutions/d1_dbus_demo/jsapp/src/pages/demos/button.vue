<template>
  <demo-scroller class="scroller">
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">默认</text>
        <div class="demo-block demo-row">
          <fl-button class="btn1" type="primary">主要</fl-button>
          <fl-button class="btn1">次要</fl-button>
          <fl-button class="btn1" type="danger">警告</fl-button>
        </div>
      </div>

      <div class="section">
        <text class="section-title">幽灵按钮</text>
        <div class="demo-block demo-row">
          <fl-button class="btn1" plain type="primary">主要</fl-button>
          <fl-button class="btn1" plain>次要</fl-button>
          <fl-button class="btn1" plain type="danger">警告</fl-button>
        </div>
      </div>

      <div class="section">
        <text class="section-title">禁用态</text>
        <div class="demo-block demo-row">
          <fl-button class="btn1" disabled type="primary">不透明度40%</fl-button>
          <fl-button class="btn1" disabled plain type="primary">不透明度40%</fl-button>
        </div>
      </div>

      <div class="section">
        <text class="section-title">尺寸</text>
        <div class="demo-block">
          <fl-button size="large" style="width: 100%">大号</fl-button>
          <fl-button style="width: 326px">默认</fl-button>
          <fl-button size="small" style="width: 160px">小号</fl-button>
        </div>
      </div>

      <div class="section">
        <text class="section-title">圆角</text>
        <div class="demo-block demo-row">
          <fl-button class="btn1" type="primary" round>圆角按钮</fl-button>
          <fl-button class="btn1" type="primary" round plain>圆角按钮</fl-button>
        </div>
      </div>

      <div class="section">
        <text class="section-title">Icon</text>
        <div class="demo-block demo-row">
          <fl-button class="btn1" icon="delete">删除</fl-button>
          <fl-button round icon="search"></fl-button>
        </div>
      </div>

    </div>
  </demo-scroller>
</template>
<script>
import { FlButton } from "falcon-ui";

export default {
  name: "FlButtonDemo",
  components: { FlButton },
  data() {
    return {
    };
  },
  methods: {
  }
};
</script>
<style lang="less" scoped>
@import "base.less";
.btn1 {
  width: 326px;
}
</style>

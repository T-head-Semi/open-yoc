<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">基础用法</text>
        <div class="demo-block demo-row">
          <fl-button
            class="btn1"
            @click="showDialog(1)"
            >基础样式</fl-button
          >
          <fl-button
            class="btn1"
            @click="showDialog(2)"
            >输入框</fl-button
          >
          <fl-button
            class="btn1"
            @click="showDialog(3)"
            >自定义内容</fl-button
          >
        </div>
      </div>
    </div>
    <fl-dialog
      v-model="show1"
      title="基础样式"
      center
      contentCenter
      :dialogStyle="{width: '720px', height: '400px'}"
      :showConfirm="false"
      cancelText="知道了"
      content="这里是详细内容描述"
    >
    </fl-dialog>

    <fl-dialog
      v-model="show2"
      title="输入框"
      center
      contentCenter
      content=""
      :dialogStyle="{width: '720px', height: '400px'}"
      :showCancel="false">
      <input class="show2-input" placeholder="请输入文本" placeholderColor="rgba(255, 255, 255, 0.4)">
    </fl-dialog>

    <fl-dialog
      v-model="show3"
      title="自定义内容"
      center
      contentCenter
      :dialogStyle="{width: '720px', height: '400px'}"
      :showCancel="false"
      confirmText="确定"
    >
    <div class="fl-dialog-self-defined-content-right-wrap">
      <text class="fl-dialog-self-defined-content-right">36</text>
    </div>
    <div class="fl-dialog-self-defined-content-left-wrap">
      <text class="fl-dialog-self-defined-content-left">标题</text>
    </div>
    </fl-dialog>

  </demo-scroller>
</template>
<script>
import { FlDialog } from "falcon-ui";
import { FlButton, FlRow, FlCol } from "falcon-ui";

export default {
  name: "Dialog",
  components: { FlDialog, FlButton, FlRow, FlCol },
  data() {
    return {
      show1: false,
      show2: false,
      show3: false,
    };
  },
  mounted() {},
  methods: {
    showDialog(index) {
      this["show" + index] = true;
    },
  }
};
</script>
<style lang="less" scoped>
@import "base.less";
.btn1 {
  width: 326px;
}
.fl-dialog-self-defined-content-left-wrap {
  position: absolute;
  left: 79px;
  top: 40px;
  width: 309px;
  height: 120px;
  border-radius: 24px;
  background-color: @primary;
  justify-content: center;
}
.fl-dialog-self-defined-content-left {
  position: relative;
  margin-left: 24px;
  font-family: Alibaba-PuHuiTi-Medium;
  height: 32px;
  width: 48px;
  font-size: @font-size-content-large;
  color: @white;
}
.fl-dialog-self-defined-content-right-wrap {
  position: absolute;
  left: 79px;
  top: 40px;
  height: 120px;
  width: 562px;
  border-radius: 24px;
  background-color: @background-color;
  align-items: flex-end;
  justify-content: center;
}
.fl-dialog-self-defined-content-right {
  position: relative;
  margin-right: 16px;
  font-family: Alibaba-PuHuiTi-Medium;
  height: 44px;
  width: 48px;
  font-size: @font-size-title-medium;
  color: @white;
}
.col-content {
  .text();
  font-size: @font-size-title-small;
  text-align: center;
  line-height: 40px;
  border-radius: 4px;
}
.col-content1 {
  .col-content();
  background-color: #99a9bf;
}
.col-content2 {
  .col-content();
  background-color: #d3dce6;
}
.col-content3 {
  .col-content();
  background-color: #e5e9f2;
}
.show2-input {
  width: 610px;
  height: 101px;
  border-radius: @radius-medium;
  background-color: @background-color;
  position: absolute;
  padding-left: 12px;
  left: 55px;
  top: 56px;
  color: @white;
}
</style>

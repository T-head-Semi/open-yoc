<template>
  <DemoScroller padding="0px">
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">图标合集</text>
        <div class="demo-block icon-block">
          <div class="icons-item" v-for="(value, key, index) in IconTypes" :key="index">
            <fl-icon class="icon" :name="key" />
            <text class="icon-type">{{ key }}</text>
          </div>
        </div>
      </div>
    </div>
  </DemoScroller>
</template>
<script>
import { FlIcon } from "falcon-ui";
import IconTypes from "falcon-ui/src/packages/icon/type.js";

export default {
  name: "Icon",
  components: { FlIcon },
  computed: {
    IconTypes() {
      const result = {}
      for (let key of Object.keys(IconTypes).slice(0, 10)) {
        result[key] = IconTypes[key]
      }
      return result
    }
  }
};
</script>
<style lang="less" scoped>
@import "base.less";
.icon-block {
  flex-direction: row;
  flex-wrap: wrap;
  justify-content: space-between;
  border-radius: @radius-medium;
}
.icons-item {
  width: 100px;
  height: 100px;
  .border();
  // background-color:#FFA564;
  margin-bottom: 5px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  padding: 8px 2px;
}
.icon {
  color: @text-color;
  font-size: 40px;
  text-align: center;
}
.icon-type {
  font-size:16px;
  color: @text-color;
  text-align: center;
}
</style>

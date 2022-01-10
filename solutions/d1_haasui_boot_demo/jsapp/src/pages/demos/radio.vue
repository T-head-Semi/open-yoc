<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">基础样式</text>
        <div class="demo-block">
          <fl-radio
            :items="radioItems"
            v-model="radioValue"
            direction='column'
          />
        </div>
      </div>
    </div>
  </demo-scroller>
</template>
<script>
import { FlRadio } from "falcon-ui";

export default {
  name: "Radio",
  components: { FlRadio },
  data() {
    return {
      radioMsg: "选中：无",
      radioValue: "opt2",
      radioItems: [
        { label: "选项1", value: "opt1" },
        { label: "选项2", value: "opt2" },
        { label: "选项3", value: "opt3" /*disabled: true*/ }
      ]
    };
  },
  mounted() {},
  methods: {}
};
</script>
<style lang="less" scoped>
@import "base.less";
</style>

<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">基础样式</text>
        <div class="demo-block">
          <fl-checkbox
            :items="checkboxItems"
            v-model="checkboxValues"
            direction='column'
          />
        </div>
      </div>
    </div>
  </demo-scroller>
</template>
<script>
import { FlCheckbox } from "falcon-ui";

export default {
  name: "CheckboxDemo",
  components: { FlCheckbox },
  data() {
    return {
      checkboxItems: [
        { label: "选项1", value: "opt1" },
        { label: "选项2", value: "opt2" },
        { label: "选项3", value: "opt3" /*disabled: true*/ }
      ],
      checkboxValues: ["opt2"]
    };
  },
  mounted() {},
  methods: {}
};
</script>
<style lang="less" scoped>
@import "base.less";
</style>

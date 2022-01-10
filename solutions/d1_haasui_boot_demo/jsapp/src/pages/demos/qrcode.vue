<template>
  <demo-scroller>
    <div class="main-wrapper">
      <text class="section-title">基础用法</text>
      <div class="section">
        <div class="demo-block qrcode-block">
          <fl-qrcode text="https://www.aliyun.com" />
        </div>
      </div>
    </div>
  </demo-scroller>
</template>
<script>
import { FlQrcode } from "falcon-ui";

export default {
  name: "QrCode",
  components: { FlQrcode },
  data() {
    return {};
  },
  mounted() {},
  methods: {}
};
</script>
<style lang="less" scoped>
@import "base.less";
.qrcode-block {
  width: 100%;
  flex-direction: row;
  justify-content: center;
}
</style>

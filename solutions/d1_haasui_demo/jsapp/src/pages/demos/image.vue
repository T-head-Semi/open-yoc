<template>
  <demo-scroller>
    <div class="main-wrapper">

      <div class="section">
        <text class="section-title">Contain</text>
        <div class="demo-block">
          <image class="image" resize="contain" :src="require('./images/image1.png')" />
        </div>
      </div>

      <div class="section">
        <text class="section-title">Cover</text>
        <div class="demo-block">
          <image class="image" resize="cover" @load="onLoad" :src="require('./images/image1.png')" />
        </div>
      </div>

      <div class="section">
        <text class="section-title">Stretch</text>
        <div class="demo-block">
          <image class="image" resize="stretch" :src="require('./images/image1.png')" />
        </div>
      </div>

      <div class="section">
        <text class="section-title">Gif</text>
        <div class="demo-block">
          <image class="image" resize="contain" :src="require('./images/gif.gif')" />
        </div>
      </div>
    </div>
    <Toast ref="toast" />
  </demo-scroller>
</template>
<script>
import { Toast } from "falcon-ui";
export default {
  name: "Image",
  components: { Toast },
  data() {
    return {};
  },
  mounted() {},
  methods: {
    onLoad(e) {
      console.log("image onLoad ", e);
      this.$refs.toast.show("image onload result=" + e.success);
    }
  }
};
</script>
<style lang="less" scoped>
@import "base.less";
.image {
  width: 672px;
  height: 259px;
  background-color: @background-color;
}
</style>

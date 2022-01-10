<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">横向滚动</text>
        <scroller scroll-direction="horizontal" class="scroller-h">
          <div v-for="(item, i) in 20" :key="i" class="scroller-h-item">
            <text class="text">{{ item }}</text>
          </div>
        </scroller>
      </div>

      <div class="section">
        <text class="section-title">纵向滚动</text>
        <scroller scroll-direction="vertical" class="scroller-v">
          <div v-for="(item, i) in 20" :key="i" class="scroller-v-item">
            <text class="text">{{ item }}</text>
          </div>
        </scroller>
      </div>
    </div>
  </demo-scroller>
</template>
<script>
export default {
  components: {},
  name: "Scroller",
  data() {
    return {
    };
  },
  mounted() {},
  methods: {
  },
};
</script>
<style lang="less" scoped>
@import "base.less";
#scroller() {
  margin: 5px;
  border-width: 0px;
  border-radius: @radius-medium;
  background-color: fade(@primary, 30%);
  padding: @space-normal;
}
.scroller-v {
  height: 400px;
  #scroller();
  align-self: center;
}
.text {
  font-size: @font-size-title-small;
  color: @white;
}
#scroll-item() {
  align-items: center;
  justify-content: center;
  border-radius: @border-radius-medium;
  background-color: @primary;
  height:125px;
  margin: @gap-normal / 2;
}
.scroller-v-item {
  width: 125px;
  #scroll-item();
}
.scroller-h {
  width: 672px;
  align-self: center;
  #scroller();
}
.scroller-h-item {
  #scroll-item();
  width: 125px;
}
</style>

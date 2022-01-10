<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">Infinite index:{{ index1 }} </text>
        <FlSlider
          :infinite="true"
          :showIndicators="true"
          @change="e => index1 = e.index"
        >
          <div
            v-for="(img, i) in imageList" :key="i"
            :class="{image: true, active: i == index1}"
          >
            <FlImage :src="img.src" />
          </div>
        </FlSlider>
      </div>

      <div class="section">
        <text class="section-title">Autoplay index:{{ index2 }}</text>
        <FlSlider
          :auto-play="true"
          @change="e => index2 = e.index"
        >
          <div
            v-for="(img, i) in imageList" :key="i"
            :class="{image: true, active: i == index2}"
          >
            <FlImage :src="img.src" />
          </div>
        </FlSlider>
      </div>

      <div class="section">
        <text class="section-title">Scrollable(false) index:{{ index3 }}</text>
        <FlSlider
          :auto-play="true"
          :scrollable="false"
          @change="e => index3 = e.index"
        >
          <div
            v-for="(img, i) in imageList" :key="i"
            :class="{image: true, active: i == index3}"
          >
            <FlImage :src="img.src" />
          </div>
        </FlSlider>
      </div>
    </div>
  </demo-scroller>
</template>
<script>
import { FlSlider, FlImage } from 'falcon-ui'

export default {
  name: "Slider",
  components: {FlSlider, FlImage},
  data() {
    return {
      imageList: [
        { src: require("./images/image1.png") },
        { src: require("./images/image2.png") },
        { src: require("./images/image3.png") }
      ],
      index1: 0,
      index2: 0,
      index3: 0
    };
  },
  mounted() {},
  methods: {}
};
</script>
<style lang="less" scoped>
@import "base.less";
.image {
  padding: 0 @space-normal;
  opacity: 0.4;
}
.active {
  opacity: 1;
}
</style>

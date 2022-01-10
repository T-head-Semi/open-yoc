<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section" v-for="(item, index) in lotties" :key="'lottie' + index">
        <div style="flex-direction: row;justify-content:space-between;width:100%;padding-top:20px;">
          <text class="section-title">动画{{ index + 1 }}</text>
          <div style="flex-direction: row;align-self:flex-end">
          <fl-button type="info" :textStyle="{fontSize:'20px'}" @click="startLottie(index)" style="margin:0 20px">启动动画</fl-button>
          <fl-button type="info" :textStyle="{fontSize:'20px'}" @click="stopLottie(index)" style="margin-right:20px;">结束动画</fl-button>
          </div>
        </div>

        <fl-lottie
          class="lottie"
          ref="lottie"
          :width="400"
          :height="280"
          :animationData="item"
          :loopCount="1"
          :autoplay="false"
        />
      </div>
    </div>
  </demo-scroller>
</template>
<script>
import { FlLottie, FlButton } from "falcon-ui";
// import LOTTIE1 from "./lotties/lottie.json";
import LOTTIE2 from "./lotties/lottie2.json";
import LOTTIE3 from "./lotties/lottie3.json";
// import LOTTIE4 from "./lotties/lottie4.json";
import LOTTIE5 from "./lotties/lottie5.json";
import LOTTIE6 from "./lotties/lottie6.json";

const LOTTIES = [/*LOTTIE1, */LOTTIE2, LOTTIE3, /*LOTTIE4, */LOTTIE5, LOTTIE6];
export default {
  components: { FlLottie, FlButton },
  computed: {
    lotties() {
      return LOTTIES;
    }
  },
  methods: {
    startLottie(index) {
      for (let i = 0; i < LOTTIES.length; i++) {
        this.$refs.lottie[index].stopAnim();
      }
      this.$refs.lottie[index].startAnim();
    },
    stopLottie(index) {
      this.$refs.lottie[index].stopAnim();
    }
  }
};
</script>

<style lang="less" scoped>
@import "base.less";
.lottie{
  margin-top:10px;
}
.main-wrapper{
  margin-bottom:0;
}
</style>

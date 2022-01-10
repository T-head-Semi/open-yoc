<template>
  <div class="scene-wrapper" v-if="current">
    <slider
      infinite="true"
      class="clock-slider"
      :index="clockItemIndex"
      @change="clockIndexChange"
      previousMargin="100px"
      nextMargin="100px"
    >
      <div :class="'clock-item-wrapper' + (clockItemIndex === 0 ? ' clock-item-wrapper-selected' : '')">
        <clock-item-1 :current="clockItemIndex === 0" />
      </div>
      <div :class="'clock-item-wrapper' + (clockItemIndex === 1 ? ' clock-item-wrapper-selected' : '')">
        <clock-item-2 :current="clockItemIndex === 1" />
      </div>
      <div :class="'clock-item-wrapper' + (clockItemIndex === 2 ? ' clock-item-wrapper-selected' : '')">
        <clock-item-3 :current="clockItemIndex === 2" />
      </div>
    </slider>
  </div>
</template>

<script>
import ClockItem1 from "./clock-item1.vue";
import ClockItem2 from "./clock-item2.vue";
import ClockItem3 from "./clock-item3.vue";
export default {
  components: { ClockItem1, ClockItem2, ClockItem3 },
  props:{
    current:{
      type:Boolean,
      default:false
    }
  },
  data() {
    return {
      clockItemIndex: 0,
    };
  },
  methods: {
    clockIndexChange(e) {
      this.clockItemIndex = e.index;
    },
  },
};
</script>

<style lang="less" scoped>
@import "./scene.less";
.scene-wrapper {
  width: 100%;
  height: 380px;
  border-radius: 0;
  background-color: transparent;
  align-items: center;
  justify-content: center;
}
.clock-slider {
  width: 800px;
  height: 100%;
  justify-content: center;
  align-items: center;
}
.clock-item-wrapper {
  width: 600px;
  height: 100%;
  padding: 0;
  transform: scale(0.8);
  .transition()
}
.clock-item-wrapper-selected{
  transform: scale(1);
}
</style>

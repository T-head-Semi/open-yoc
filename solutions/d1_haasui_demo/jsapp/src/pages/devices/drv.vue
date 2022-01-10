<template>
  <div class="page">
    <div class="tab-list">
      <fl-icon name="back" class="nav-back" @click="onBack" />
      <text
        v-for="(item, index) in tabs"
        :key="index"
        :class="'tab-item' + (index === selectedIndex ? ' tab-item-selected' : '')"
        @click="tabSelected(index)"
        >{{ item }}</text
      >
    </div>
    <div v-if="selectedIndex === 0" class="vertical-layout">
      <div class="seek-group">
        <fl-seekbar
          v-model="power"
          useBlockStyle
          :length="720"
          @change="powerChange"
        >
          <text class="seek-title">强度</text>
          <text class="seek-percent">{{ power }}%</text>
        </fl-seekbar>
      </div>

      <div class="seek-group">
        <fl-seekbar
          :height="120"
          useBlockStyle
          :length="720"
          @change="durationChange"
          v-model="duration"
        >
          <text class="seek-title">时长</text>
          <text class="seek-percent">{{ parseInt((((MAX_DURATION / 100) * duration) / 1000) * 10) / 10 }}s</text>
        </fl-seekbar>
      </div>
      <div class="btn-wrap">
        <FlButton class="btn" @click="reset">重置</FlButton>
        <FlButton class="btn" @click="perform" type="primary">触发震动</FlButton>
      </div>
    </div>
    <scroller v-else class="scroller">
      <div class="preset-wrap">
        <div class="btn-preset-item" v-for="(name, index) in presets" :key="index">
          <FlButton class="btn-preset" @click="performPreset(index)">{{name}}</FlButton>
        </div>
      </div>
    </scroller>
    <!-- <fl-button @click="click1">测试震动1</fl-button>
    <fl-button @click="click2">测试震动2</fl-button>
    <fl-button @click="click3">测试震动3</fl-button> -->
  </div>
</template>
<script>
import { FlButton, FlIcon, FlSeekbar } from "falcon-ui";
import drv from "$jsapi/drv";
const MAX_DURATION = 5000;
const DEFAULT_POWER = 65;
const DEFAULT_DURATION = 30;

export default {
  components: { FlButton, FlIcon, FlSeekbar },
  name: "drv",
  data() {
    const PRESETS = [
      "强硬",
      "标准",
      "轻柔",
      "次强",
      "次弱",
      "极弱",
      "轻松",
      "淡定",
      "松软",
      "二阶强调",
      "二阶标准",
      "三阶强调",
      "减弱",
      "渐强",
      "持强",
      "连续强",
      "强收敛",
      "收敛",
      "弱收敛",
      "极弱收敛"
    ];
    return {
      MAX_DURATION: MAX_DURATION,
      tabs: ["自定义震动", "预设震动"],
      selectedIndex: 0,
      power: DEFAULT_POWER,
      duration: DEFAULT_DURATION,
      presets:PRESETS
    };
  },
  methods: {
    reset() {
      //重置
      this.power = DEFAULT_POWER;
      this.duration = DEFAULT_DURATION;
    },
    perform() {
      //触发震动接口
      const duration = (MAX_DURATION / 100) * this.duration;
      const power = parseInt((this.power / 100) * 128) + 127; //从128开始震动
      drv.playByRtp({ power, duration });
    },
    performPreset(index) {
      drv.playByLib({ library: 6, waveindex: index + 1 });
    },
    onBack() {
      this.$page.finish();
    },
    tabSelected(index) {
      this.selectedIndex = index;
    },
    powerChange(p) {
      this.power = p;
    },
    durationChange(d) {
      this.duration = d;
    }
  }
};
</script>
<style lang="less" scoped>
@import "base.less";
.nav-back {
  line-height: 48px;
  margin-right: 20px;
}
.vertical-layout {
  row-gap: @gap-normal;
}
.tab-list {
  flex-direction: row;
  margin: 20px 40px 20px 0;
}
.tab-item {
  color: @text-color;
  opacity: 0.3;
  font-size: 32px;
  margin-right: 48px;
}
.tab-item-selected {
  opacity: 1;
}
.seek-group {
  align-items: center;
}
.seek-title {
  position: absolute;
  color: @white;
  left: 30px;
  top: 0;
  font-size: 28px;
  line-height: 120px;
}
.seek-percent {
  position: absolute;
  color: @white;
  right: 30px;
  top: 0;
  font-size: 32px;
  line-height: 120px;
}
.btn-wrap {
  flex-direction: row;
  justify-content: space-between;
  width: 720px;
  align-self: center;
  column-gap: @gap-normal;
}
.btn {
  flex: 1;
}
.scroller {
  align-items: center;
  margin-bottom: 100px;
}
.preset-wrap {
  flex-direction: row;
  flex-wrap: wrap;
  width: 720px;
}
.btn-preset-item {
  width: 50%;
  // flex: 1 0 50%;
}
.btn-preset {
  margin: @gap-normal/2;
}
</style>

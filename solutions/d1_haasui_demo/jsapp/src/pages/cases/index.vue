<template>
  <div class="page">
    <div class="tab-list">
      <fl-icon name="back" class="nav-back" @click="onBack" />
      <scroller
        class="tab-scroller"
        scroll-direction="horizontal"
        show-scrollbar="false"
        :index="selectedTabIndex"
        @change="tabIndexChanged"
      >
      <text
        v-for="(item, index) in scenes"
        :key="index"
        :class="'tab-item' + (index === selectedIndex ? ' tab-item-selected' : '')"
        @click="tabSelected(index)"
        >{{ item }}</text
      >
    </div>
    <AirConditioning :style="{display: (selectedIndex == 0 ? 'flex': 'none')}" :current="selectedIndex == 0" />
    <Light :style="{display: (selectedIndex == 1 ? 'flex': 'none')}" :current="selectedIndex == 1"/>
    <Clock :style="{display: (selectedIndex == 2 ? 'flex': 'none')}" :current="selectedIndex == 2"/>
    <Media :style="{display: (selectedIndex == 3 ? 'flex': 'none')}" :current="selectedIndex == 3" />
    <Voice :style="{display: (selectedIndex == 4 ? 'flex': 'none')}" :current="selectedIndex == 4" />
    <!-- <component :is="tabContent" /> -->
  </div>
</template>

<script>
import { FlIcon } from "falcon-ui";
import Media from "./components/media.vue";
import Voice from "./components/voice.vue";
import AirConditioning from "./components/air-conditioning.vue";
import Cashier from "./components/cashier.vue";
import Clock from "./components/clock.vue";
import Light from "./components/light.vue";

const COMPONENTS = [AirConditioning, Light, Clock, Media, Voice];

const component = {
  components: { FlIcon, AirConditioning, Light, Clock, Media, Voice},
  name: "page",
  data() {
    return {
      scenes: ["智能空调", "调光面板", "智能闹钟", "媒体播放", "语音AI", ""],
      selectedIndex: 0
    };
  },
  computed: {
    tabContent() {
      return COMPONENTS[this.selectedIndex];
    }
  },
  methods: {
    tabSelected(index) {
      this.selectedIndex = index;
    },

    onBack() {
      this.$page.finish();
    }
  }
};

export default component;
</script>

<style lang="less" scoped>
@import "base.less";
.nav-back {
  line-height: 48px;
  margin-right: 20px;
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
</style>

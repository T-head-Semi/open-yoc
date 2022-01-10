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
    <DbusDemo :style="{display: (selectedIndex == 3 ? 'flex': 'none')}" :current="selectedIndex == 3" />
    <!-- <component :is="tabContent" /> -->
  </div>
</template>

<script>
import { FlIcon } from "falcon-ui";
import DbusDemo from "./components/switch_dbus.vue";
import AirConditioning from "./components/air-conditioning.vue";
import Cashier from "./components/cashier.vue";
import Clock from "./components/clock.vue";
import Light from "./components/light.vue";

const COMPONENTS = [AirConditioning, Light, Clock, DbusDemo];

const component = {
  components: { FlIcon, AirConditioning, Light, Clock, DbusDemo },
  name: "page",
  data() {
    return {
      scenes: ["智能空调", "调光面板", "智能闹钟", "开关(基于DBUS)", ""],
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

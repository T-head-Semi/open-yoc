<template>
  <div class="page">
    <fl-icon name="back" class="nav-back" @click="onBack" />
    <scroller
      class="tab-scroller"
      scroll-direction="horizontal"
      show-scrollbar="false"
    >
      <text
        class="tab-item"
        v-for="(item, index) in groups"
        :key="'tab_' + index"
        :class="
          'tab-item' + (index === selectedTabIndex ? ' tab-item-selected' : '') + (index === 0 ? ' tab-item-first' : '')
        "
        @click="tabSelected(index)"
        :ref="'tab-item-' + index"
        >{{ item.title }}</text
      >
    </scroller>
    <slider ref="slider" class="component-slider" :index="selectedTabIndex" @change="sliderChanged" scrollable="false">
      <div class="component-group" v-for="(item, index) in groups" :key="'cg' + index">
        <div class="component-item" v-for="(component, index) in item.components" :key="'comp' + index" @click="onItemClick(component)">
          <text class="component-name">{{component}}</text>
          <text class="component-name-bg" lines="1">{{component}}</text>
        </div>
      </div>
    </slider>
  </div>
</template>

<script>
import { FlIcon } from "falcon-ui";
import { componentsGroup } from "../../share.js";
export default {
  name: "index",
  components: { FlIcon },
  data() {
    return {
      // groups:componentsGroup,
      selectedTabIndex: 0,
    };
  },
  computed:{
    groups(){
      return componentsGroup;
    }
  },
  methods: {
    tabSelected(index) {
      this.$refs.slider.slideTo(index)
    },
    sliderChanged(e){
      this.selectedTabIndex = e.index;
    },
    onItemClick(component){
      $falcon.navTo(component);
    },
    onBack(){
      this.$page.finish();
    }
  },
};
</script>

<style lang="less" scoped>
@import "base.less";
.page {
  // background-color: #282932;
  padding-top:20px;
  position:relative;
}
.nav-wrap{
  flex-direction: row;
  width:100%;
}
.nav-back{
  line-height:48px;
  position:absolute;
  left:0px;
  top:20px;
}
.tab-scroller {
  // padding-left: 16px;
  margin-left: 88px;;
  // margin-right: 16px;
}
.tab-item {
  font-size: @font-size-title-medium;
  color: @text-color;
  opacity: 0.3;
  padding-left:20px;
  padding-right:20px
}
.tab-item-first{
  padding-left:10px;
}
.tab-item-selected {
  opacity: 1;
}
.component-slider{
  width:100%;
  height:415px;
}
.component-group{
  flex-direction: column;
  flex-wrap: wrap;
  height:440px;
  width:100%;
  justify-content: flex-start;
  align-items: flex-start;
  padding: 20px 30px 0 30px;
}
.component-item{
  height: 180px;
  width: 180px;
  background-color: @card-background-color;
  border-radius: @radius-normal;
  margin: @gap-normal/2;
  overflow: hidden;
}
.component-name{
  font-size: @font-size-title-small;
  color: @text-color;
  margin-left: @space-normal;
  margin-top: @space-normal;
  font-weight: bold;
}
.component-name-bg{
  font-size: 96px;
  color: @text-color;
  opacity:0.1;
  left:70px;
  top:70px;
  width:200px;
  position:absolute;
  font-weight: bold;
}
</style>

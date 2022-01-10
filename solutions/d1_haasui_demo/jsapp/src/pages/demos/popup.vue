<template>
  <demo-scroller>
    <div class="main-wrapper">
      <!-- <text class="main-desc">弹出层容器,可自定义弹出层内内容.</text> -->
      <div class="section">
        <text class="section-title">基础用法</text>
        <div class="demo-block demo-row">
          <fl-button
            class="btn1"
            @click="showPop(false, false, 'center')"
            >居中弹出</fl-button
          >
          <fl-button
            class="btn1"
            @click="showPop(false, false, 'top')"
            >顶部弹出</fl-button
          >
          <fl-button
            class="btn1"
            @click="showPop(false, false, 'left')"
            >左侧弹出</fl-button
          >
          <fl-button
            class="btn1"
            @click="showPop(false, false, 'right')"
            >右侧弹出</fl-button
          >
          <fl-button
            class="btn1"
            @click="showPop(false, false, 'bottom')"
            >底部弹出</fl-button
          >
        </div>
      </div>

      <div class="section">
        <text class="section-title">内置动画</text>
        <div class="demo-block demo-row">
          <fl-button
            class="btn1"
            @click="showPop(true, true, 'center')"
            >居中弹出</fl-button
          >
          <fl-button
            class="btn1"
            @click="showPop(true, true, 'top')"
            >顶部弹出</fl-button
          >
          <fl-button
            class="btn1"
            @click="showPop(true, true, 'left')"
            >左侧弹出</fl-button
          >
          <fl-button
            class="btn1"
            @click="showPop(true, true, 'right')"
            >右侧弹出</fl-button
          >
          <fl-button
            class="btn1"
            @click="showPop(true, true, 'bottom')"
            >底部弹出</fl-button
          >
        </div>
      </div>

      <div class="section">
        <text class="section-title">自定义动画</text>
        <div class="demo-block demo-row">
          <fl-button
            class="btn1"
            @click="showPop('fl-zoom-in-center', true, 'center')"
            >中间展开</fl-button
          >
          <fl-button
            class="btn1"
            @click="showPop('rotate', true, 'center')"
            >旋转进入</fl-button
          >
        </div>
      </div>
    </div>

    <fl-popup
      v-model="show"
      :overlay-transition="overlayTransition"
      :position="position"
      :transition="transition"
      :handleClose="handlePopupClose"
      @open="onPopupOpen"
      @close="onPopupClose"
    >
      <div :class="popContentClass">
        <text class="text">浮层内容</text>
      </div>
    </fl-popup>
  </demo-scroller>
</template>
<script>
import { FlPopup } from "falcon-ui";
import { FlButton } from "falcon-ui";

export default {
  name: "Popup",
  components: { FlPopup, FlButton },
  data() {
    return {
      show: false,
      transition: false,
      overlayTransition: false,
      position: "center",
      popContentClass: "demo-popup-content"
    };
  },
  methods: {
    showPop(transition, overlayTransition, position) {
      this.transition = transition;
      this.overlayTransition = overlayTransition;
      this.position = position;
      this.show = true;

      this.popContentClass = "demo-popup-content";
      if (position === "left" || position === "right") {
        this.popContentClass += " demo-popup-content-h";
      } else if (position === "top" || position == "bottom") {
        this.popContentClass += " demo-popup-content-v";
      }
    },
    handlePopupClose() {
      // console.log('handle popup close');
      return false;
      // return new Promise((resolve, reject) => {
      //   setTimeout(() => {
      //     resolve(false)
      //   }, 2000);
      // });
    },
    onPopupOpen() {
      console.log("popup open event");
    },
    onPopupClose() {
      console.log("popup close event");
    }
  }
};
</script>
<style lang="less" scoped>
@import "base.less";
.demo-popup-content {
  width: 50%;
  height: 30%;
  align-items: center;
  justify-content: center;

  background-color: @card-background-color;
  border-radius: @border-radius-normal;
  padding: @padding-normal;
}

.demo-popup-content-h {
  height: 100%;
}

.demo-popup-content-v {
  width: 100%;
}

.transition(@property, @duration, @function, @delay:0) {
  transition-property: @property;
  transition-duration: @duration;
  transition-timing-function: @function;
  transition-delay: @delay;
}

.fl-zoom-in-center-enter-active {
  .transition(all, 300ms, cubic-bezier(0.55, 0, 0.1, 1));
}
.fl-zoom-in-center-enter {
  opacity: 0;
  transform: scaleX(0);
}

.fl-zoom-in-center-enter-to {
  opacity: 1;
  transform: scaleX(1);
}

.rotate-enter {
  transform: rotate(-360deg);
  opacity: 0;
}
.rotate-enter-to {
  transform: rotate(0deg);
  opacity: 1;
}
.rotate-enter-active {
  transition-property: opacity, transform;
  transition-duration: 0.5s;
}
.btn1 {
  width: 326px;
}
</style>

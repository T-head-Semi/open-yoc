<template>
  <div class="page">
    <simple-time
      class="time"
      :twinkling="true"
      :textStyle="{ fontSize: '38px', fontWeight: 'bold' }"
      :colonStyle="{ width: '16px', textAlign: 'center' }"
    />
    <div class="scroller" scroll-direction="horizontal" show-scrollbar="false">
      <div class="item" style="position: relative; width: 328px; height: 376px">
        <home-item
          @click="cardRoate = !cardRoate"
          v-bind="items[0]"
          :titleStyle="{ fontWeight: 'bold' }"
          :class="'rotate-item' + (cardRoate ? ' home-item-rotated' : '')"
        />
        <div
          @click="cardRoate = !cardRoate"
          :class="'card-qrcode rotate-item rotate-item2' + (cardRoate ? '' : ' home-item-rotated')"
        >
          <div class="qr-wrapper">
            <qrcode v-if="layzyLoadQrCode" style="width: 100%; height: 100%;" value="https://haas.iot.aliyun.com/haasui" />
          </div>
          <text class="qr-desc">扫码了解详情</text>
          <div class="qr-back-wrap">
            <text class="qr-back">返回</text>
            <fl-icon class="qr-back" name="arrow-right" />
          </div>
        </div>
      </div>
      <div class="item-group-outer">
        <home-item v-bind="items[1]" class="item item-line1" style="flex: auto;" @click="$falcon.navTo('cases')" />
        <div class="item-line2" style="flex: auto">
          <home-item v-bind="items[2]" @click="$falcon.navTo('uihome')" class="item" style="flex: 1" />
          <home-item v-bind="items[3]" @click="$falcon.navTo('drv')" class="item" style="flex: 1" />
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import SimpleTime from "@/components/SimpleTime.vue";
import HomeItem from "./components/HomeItem.vue";
import { FlQrcode, FlIcon } from "falcon-ui";

const HOME_ITEMS = [
  {
    bg: require("../../images/left.png"),
    bgPrimary: true,
    bgStyle: {
      'background-repeat': 'no-repeat',
      'background-position': 'left bottom',
    },
    title: "HaaS UI",
    desc: "HaaS平台轻量级应用显示框架",
    link: "查看更多",
  },
  {
    bg: require("../../images/right-top.png"),
    bgStyle: {
      'background-repeat': 'no-repeat',
      'background-position': 'right bottom',
    },
    title: "应用场景",
  },
  {
    bg: require("../../images/right-bottom-1.png"),
    bgStyle: {
      'background-repeat': 'no-repeat',
      'background-position': 'right bottom',
    },
    title: "UI 组件",
  },
  {
    bg: require("../../images/right-bottom-2.png"),
    bgStyle: {
      'background-repeat': 'no-repeat',
      'background-position': 'right bottom',
    },
    title: "触控反馈",
  }
];

export default {
  name: "index",
  components: { SimpleTime, HomeItem ,FlQrcode, FlIcon },
  watch: {
    cardRoate() {
      if (this.cardRoate) {
        this.layzyLoadQrCode = true
      }
    }
  },
  data() {
    return {
      message: "",
      items: HOME_ITEMS,
      cardRoate: false,
      layzyLoadQrCode: false,
    };
  }
};
</script>

<style lang="less" scoped>
@import "base.less";
.time {
  position: absolute;
  left: 40px;
  top: 20px;
}
.scroller {
  width: 720px;
  margin: 78px 40px 0 40px;
  flex-direction: row;
  overflow:visible;
}
.item {
  margin: @gap-compact/2;
  overflow: visible;
  border-radius: @border-radius-normal;
}
.rotate-item {
  .transition(all, .35s);
  width: 100%;
  height: 100%;
  transform: rotateY(0deg);
  position: absolute;
  left: 0;
  top: 0;
  opacity: 1;
  border-radius: @border-radius-normal;
}
.rotate-item2 {
  transform: rotateY(360deg);
}
.home-item-rotated {
  transform: rotateY(180deg);
  opacity: 0;
}
.card-qrcode {
  background-color: @card-background-color;
  align-items: center;
  justify-content: center;
}
.item-group-outer {
  flex: 1;
  flex-direction: column;
}
.item-line2 {
  flex-direction: row;
}
.qr-wrapper{
  padding:8px;
  background-color: @white;
  width: 224px;
  height: 224px;
}
.qr-desc{
  font-size:18px;
  color: @text-color;
  margin-top:16px;
}
.qr-back-wrap{
  position:absolute;
  right:24px;
  bottom:26px;
  flex-direction: row;
}
.qr-back{
  font-size:20px;
  color: @text-color;
  line-height: 20px;
}
</style>

<template>
  <div class="scene-wrapper" v-if="current">
    <div class="qr-wrapper">
      <image class="qrcode" :src="require('../images/qr-alipay.png')" />
      <!-- <text class="alipay-tip">支付宝扫一扫付款</text> -->
    </div>
    <div class="goods-wrapper">
      <!-- <scroller class="goods-list" scroll-direction="vertical">
        <div class="goods-item">
          <text class="goods-title">UNO男士5合一护理面霜</text>
          <div class="good-info">
            <text class="goods-price">¥ 80.00</text>
            <text class="goods-count">x1</text>
          </div>
        </div>

        <div class="goods-item">
          <text class="goods-title">UNO泡沫洗面奶</text>
          <div class="good-info">
            <text class="goods-price">¥ 46.00</text>
            <text class="goods-count">x1</text>
          </div>
        </div>
      </scroller> -->
      <div class="stat-wrapper">
        <!-- <text class="stat-txt">共2件，合计：</text> -->
        <text class="stat-txt stat-txt-y">¥</text>
        <text class="stat-txt stat-txt-price-total">126.00</text>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  props:{
    current:{
      type:Boolean,
      default:false
    }
  },
  data() {
    return {};
  },
  methods: {},
};
</script>

<style lang="less" scoped>
@import "./scene.less";
.scene-wrapper {
  flex-direction: column;
  justify-content:center;
  padding-top:10px;
}
.qr-wrapper {
  align-items: center;
  margin-top:10px;
}
.qrcode {
  width: 242px;
  height: 242px;
}
.alipay-tip {
  color: @text-color;
  font-size: 24px;
}
.goods-wrapper {
  // margin-top: 8px;
  justify-content: center;
  // margin-bottom: 35px;
  // margin-right:25px;
}
.goods-list {
}
.goods-item {
  flex-direction: row;
  justify-content: space-between;
  margin: 24px 0;
  padding-right:24px;
  width:388px;
}
.goods-title {
  color: @text-color;
  font-size: 24px;
  width: 230px;
  padding-left:10px;
}
.good-info {
  flex-direction: column;
  align-items: flex-end;
}
.goods-price {
  color: @text-color;
  font-size: 24px;
}
.goods-count {
  color: @text-color;
  font-size: 24px;
}
.stat-wrapper {
  flex-direction: row;
  align-items: flex-end;
  justify-content: center;
  margin-top:16px;
}
.stat-txt {
  color: @text-color;
  font-size:24px;
}
.stat-txt-y {
  color: @primary;
  margin-right:16px;
  font-size:32px;
  margin-bottom: 4px;
}
.stat-txt-price-total {
  color: @primary;
  font-size: 48px;
}
</style>

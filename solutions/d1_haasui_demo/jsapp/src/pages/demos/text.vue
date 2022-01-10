<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">Color</text>
        <div class="demo-block demo-row">
          <div class="demo-row">
            <div class="color-demo-cycle primary-cycle"></div>
            <text class="color-demo-text primary-text">primary</text>
          </div>
          <div class="demo-row">
            <div class="color-demo-cycle" style="background-color: #646e98"></div>
            <text class="color-demo-text" style="color: #646e97">#646E97</text>
          </div>
          <div class="demo-row">
            <div class="color-demo-cycle" style="background-color: rgb(134, 143, 171)"></div>
            <text class="color-demo-text" style="color: rgb(134, 143, 171)">rgb(134, 143, 171)</text>
          </div>
          <div class="demo-row">
            <div class="color-demo-cycle" style="background-color: rgba(134, 143, 171, 0.3)"></div>
            <text class="color-demo-text" style="color: rgba(134, 143, 171, 0.3)">rgb(134, 143, 171, 0.3)</text>
          </div>
        </div>
      </div>

      <div class="section">
        <text class="section-title">lines</text>
        <div class="demo-block">
          <text style="lines: 3; font-size: 24px; color: #868fab; width: 100%;">可见第一行, lines: 3
可见第二行,注意断行
可见第三行,回车符分隔
不可见第四行
          </text>
        </div>
      </div>

      <div class="section">
        <text class="section-title">font-size</text>
        <div>
          <text style="font-size: 36px; color: #868fab">特大标题:36px </text>
          <text style="font-size: 32px; color: #868fab">主标题:32px </text>
          <text style="font-size: 28px; color: #868fab">副标题:28px </text>
          <text style="font-size: 24px; color: #868fab">正文标题:24px </text>
        </div>
      </div>

      <div class="section">
        <text class="section-title">font-style</text>
        <div>
          <text style="font-style: normal; font-size: 24px; color: #868fab">normal</text>
          <text style="font-style: italic; font-size: 24px; color: #868fab">italic</text>
        </div>
      </div>

      <div class="section">
        <text class="section-title">font-weight</text>
        <div>
          <text style="font-weight: normal; font-size: 24px; color: #868fab">Normal</text>
          <text style="font-weight: bold; font-size: 24px; color: #868fab">Bold</text>
        </div>
      </div>

      <div class="section">
        <text class="section-title">text-decoration</text>
        <div>
          <text style="text-decoration: underline; font-size: 24px; color: #868fab">underline</text>
          <text style="text-decoration: line-through; font-size: 24px; color: #868fab">line-through</text>
        </div>
      </div>

      <div class="section">
        <text class="section-title">text-align</text>
        <div>
          <text
            style="
              text-align: left;
              width: 672px;
              height: 56px;
              line-height: 56px;
              font-size: 24px;
              color: #868fab;
              background-color: rgba(100, 110, 152, 0.3);
              padding: 0 20px;
              margin-bottom:12px;
            "
            >left</text
          >
          <text
            style="
              padding: 0 20px;
              text-align: center;
              width: 672px;
              height: 56px;
              line-height: 56px;
              font-size: 24px;
              color: #868fab;
              background-color: rgba(100, 110, 152, 0.3);
              margin-bottom:12px;
            "
            >center</text
          >
          <text
            style="
              padding: 0 20px;
              text-align: right;
              width: 672px;
              height: 56px;
              line-height: 56px;
              font-size: 24px;
              color: #868fab;
              background-color: rgba(100, 110, 152, 0.3);
            "
            >right</text
          >
        </div>
      </div>

      <div class="section">
        <text class="section-title">text-overflow</text>
        <div class="demo-block">
          <text style="width: 110px; text-overflow: ellipsis; lines: 1; font-size: 24px; color: #868fab"
            >overflow Lorem ipsum dolor sit amet</text
          >
        </div>
      </div>
    </div>
  </demo-scroller>
</template>
<script>
import { FlButton } from "falcon-ui";

export default {
  name: "Button",
  components: { FlButton },
  data() {
    return {
      clickCount: 0
    };
  },
  mounted() {},
  methods: {
    buttonClicked(e) {
      this.clickCount += 1;
      console.log(e);
    }
  }
};
</script>
<style lang="less" scoped>
@import "base.less";
.color-demo-cycle {
  width: 36px;
  height: 36px;
  border-radius: 18px;
  margin-right: 12px;
}
.color-demo-text {
  font-size: @font-size-content-large;
  line-height: 36px;
}
.primary-cycle {
  background-color: @primary;
}
.primary-text {
  color: @primary;
}
</style>

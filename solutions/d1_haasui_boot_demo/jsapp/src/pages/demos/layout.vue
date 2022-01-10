<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">基础布局</text>
        <div class="layout-block">
          <fl-row class="row">
            <fl-col :span="24" class="col" >
              <text class="col-content1">span:24</text>
            </fl-col>
          </fl-row>
          <fl-row class="row" gutter="20">
            <fl-col span="12" class="col" >
              <text class="col-content2">span:12 gutter:20</text>
            </fl-col>
            <fl-col span="12" class="col">
              <text class="col-content3">span:12 gutter:20</text>
            </fl-col>
          </fl-row>
          <fl-row class="row" gutter="10">
            <fl-col span="8" class="col">
              <text class="col-content1">span:8 gutter:10</text>
            </fl-col>
            <fl-col span="8" class="col">
              <text class="col-content2">span:8 gutter:10</text>
            </fl-col>
            <fl-col span="8" class="col">
              <text class="col-content3">span:8 gutter:10</text>
            </fl-col>
          </fl-row>
        </div>
      </div>

      <!-- <div class="section">
        <text class="section-title">分栏间隔</text>
        <div class="layout-block">
          <fl-row class="row" gutter="20">
            <fl-col span="6" class="col">
              <text class="col-content2">span:6</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">span:6</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">span:6</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">span:6</text>
            </fl-col>
          </fl-row>
        </div>
      </div> -->

      <div class="section">
        <text class="section-title">混合布局</text>
        <div class="layout-block">
          <fl-row class="row" gutter="10">
            <fl-col span="16" class="col">
              <text class="col-content2">span:16</text>
            </fl-col>
            <fl-col span="8" class="col">
              <text class="col-content2">span:8</text>
            </fl-col>
          </fl-row>

          <fl-row class="row" gutter="10">
            <fl-col span="8" class="col">
              <text class="col-content2">span:8</text>
            </fl-col>
            <fl-col span="8" class="col">
              <text class="col-content2">span:8</text>
            </fl-col>
            <fl-col span="4" class="col">
              <text class="col-content2">span:4</text>
            </fl-col>
            <fl-col span="4" class="col">
              <text class="col-content2">span:4</text>
            </fl-col>
          </fl-row>

          <fl-row class="row" gutter="10">
            <fl-col span="4" class="col">
              <text class="col-content2">span:4</text>
            </fl-col>
            <fl-col span="16" class="col">
              <text class="col-content2">span:16</text>
            </fl-col>
            <fl-col span="4" class="col">
              <text class="col-content2">span:4</text>
            </fl-col>
          </fl-row>
        </div>
      </div>

      <div class="section">
        <text class="section-title">分栏偏移</text>
        <div class="layout-block">
          <fl-row class="row">
            <fl-col span="6" class="col">
              <text class="col-content2">span:6</text>
            </fl-col>
            <fl-col span="6" offset="6" class="col">
              <text class="col-content2">offset:6</text>
            </fl-col>
          </fl-row>

          <fl-row class="row">
            <fl-col span="6" offset="6" class="col">
              <text class="col-content2">offset:6</text>
            </fl-col>
            <fl-col span="6" offset="6" class="col">
              <text class="col-content2">offset:6</text>
            </fl-col>
          </fl-row>

          <fl-row class="row">
            <fl-col span="12" offset="6" class="col">
              <text class="col-content2">offset:6</text>
            </fl-col>
          </fl-row>
        </div>
      </div>

      <div class="section">
        <text class="section-title">横向对齐</text>
        <div class="layout-block">
          <fl-row class="row" gutter="10">
            <fl-col span="6" class="col">
              <text class="col-content2">default</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">or</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">start</text>
            </fl-col>
          </fl-row>
          <fl-row class="row" justify="center" gutter="10">
            <fl-col span="6" class="col">
              <text class="col-content2">center</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">center</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">center</text>
            </fl-col>
          </fl-row>
          <fl-row class="row" justify="end" gutter="10">
            <fl-col span="6" class="col">
              <text class="col-content2">end</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">end</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">end</text>
            </fl-col>
          </fl-row>
          <fl-row class="row" justify="space-between" gutter="10">
            <fl-col span="6" class="col">
              <text class="col-content2">space</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">-</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">between</text>
            </fl-col>
          </fl-row>

          <fl-row class="row" justify="space-around" gutter="10">
            <fl-col span="6" class="col">
              <text class="col-content2">space</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">-</text>
            </fl-col>
            <fl-col span="6" class="col">
              <text class="col-content2">around</text>
            </fl-col>
          </fl-row>
        </div>
      </div>

      <div class="section">
        <text class="section-title">纵向对齐</text>
        <div class="layout-block" style="margin-bottom:24px">
          <fl-row class="row" gutter="10" align="top" style="height:100px">
            <fl-col span="6" class="col">
              <text class="col-content2">top</text>
            </fl-col>
          </fl-row>
        </div>
        <div class="layout-block" style="margin-bottom:24px">
          <fl-row class="row" gutter="10" align="center" style="height:100px">
            <fl-col span="6" class="col">
              <text class="col-content2">center</text>
            </fl-col>
          </fl-row>
          </div>
          <div class="layout-block" style="margin-bottom:24px">
          <fl-row class="row" gutter="10" align="bottom" style="height:100px">
            <fl-col span="6" class="col">
              <text class="col-content2">bottom</text>
            </fl-col>
          </fl-row>
          </div>
      </div>
    </div>
  </demo-scroller>
</template>
<script>
import { FlRow, FlCol } from "falcon-ui";

export default {
  name: "Layout",
  components: { FlRow, FlCol },
  data() {
    return {};
  },
  mounted() {},
  methods: {},
};
</script>
<style lang="less" scoped>
@import "base.less";
.layout-block{
  padding: @space-normal;
  background-color: fade(@primary, 30%);
  border-radius: @radius-medium;
  row-gap: 20px;
}
.row {
}
.col {
  height: 60px;
}
.col-content {
  border-radius: 4px;
  font-size: @font-size-content-small;
  color: @white;
  text-align: center;
  line-height: 60px;
}
.col-content1 {
  .col-content();
  background-color: @primary;
}
.col-content2 {
  .col-content();
  background-color: @primary;
}
.col-content3 {
  .col-content();
  background-color: @primary;
}
.row-border{

}
</style>

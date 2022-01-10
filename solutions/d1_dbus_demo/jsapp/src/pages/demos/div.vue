<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">盒子模型</text>
        <div class="demo-block">
          <div class="demo-div">
            <div class="demo-content">
              <text class="text">内容区域</text>
            </div>
          </div>
        </div>
      </div>
    </div>
  </demo-scroller>
</template>
<style lang="less" scoped>
@import "base.less";
.main-wrapper{
  padding-bottom: 0px;
  margin-bottom:0px;
}
.text {
  font-size: @font-size-title-small;
  color: @white;
  text-align: center;
}
.demo-div {
  padding: @padding-normal;
  width: 100%;
  height: 225px;
  .border(@border-normal, fade(@primary, 30%));
  background-color: @card-background-color;
  background-color: fade(@primary, 30%);
  border-radius:@border-radius-normal;
}

.demo-content {
  width:100%;
  height:100%;
  background-color: @primary;
  justify-content: center;
  border-radius: @radius-medium;
}
</style>

<template>
  <!-- box为组件外框 -->
  <div
    :class="{
      'switch-outer': true,
      'switch-outer-active': isActive,
      'switch-outer-disable': disabled,
    }"
    @click="toggle"
    :style="switchStyle"
  >
    <div class="desc">
      <text class="title">{{title}}</text>
      <text class="text">{{isActive ? activeText : inactiveText}}</text>
    </div>
    <div :class="isActive ? 'ball-active': 'ball-inactive'"></div>
  </div>
</template>
<script>

export default {
  name: 'SwitchCard',
  model:{           // v-model双向数据绑定
    prop: 'value',
    event: 'change'
  },
  props: {
    value: {      // 是否选中
      type: [Boolean],
      default: false,
    },
    title: {
      type: String,
      default: '标题'
    },
    activeText: {
      type: String,
      default: '已开启'
    },
    inactiveText: {
      type: String,
      default: '已关闭'
    },
    width: {
      type: [Number, String],
      default: '239px'
    },
    height: {
      type: [Number, String],
      default: '119px'
    },
    disabled: {
      type: Boolean,
      default: false
    },
    beforeChange:{
      type:Function,
      default:null
    },
  },
  data() {
    return {
      innerValue: this.value,
    };
  },
  computed: {
    isActive() {
      return this.innerValue == true
    },
    switchStyle() {
      return {
        width: `${parseInt(this.width)}px`,
        height: `${parseInt(this.height)}px`,
      }
    }
  },
  methods: {
    toggle() {
      if (this.disabled) return

      let nextValue = this._getToggleValue();
      nextValue = this.beforeChange ? this.beforeChange(nextValue) : nextValue;
      this.innerValue = nextValue;
    },
    _getToggleValue() {
      return !this.innerValue
    },
  },
  watch: {
    value() {
      this.innerValue = this.value;
    },
    innerValue() {
      this.$emit('change', this.innerValue)
    }
  }
}
</script>
<style lang="less" scoped>
@import "base.less";

.title {
  font-size: @font-size-title-small;
  color: @white;
  margin-bottom: 10px;
}

.text {
  font-size: @font-size-content-large;
  color: @white;
}

.switch-outer {
  align-items: center;
  justify-content: flex-start;
  padding: 20px 20px 24px 24px;
  background-color: @secondary;
  border-radius: @border-radius-normal;
  flex-direction: row;
  align-items: flex-start;
  justify-content: space-between;
  &-active {
    background-color: @primary;
  }
  &-disable {
    opacity: @disabled-opacity;
  }
}

.ball-active {
  height: 16px;
  width: 16px;
  border-radius: 16px;
  background-color: @light-green;
}

.ball-inactive {
  height: 16px;
  width: 16px;
  border-radius: 16px;
  background-color: @card-background-color;
}

</style>

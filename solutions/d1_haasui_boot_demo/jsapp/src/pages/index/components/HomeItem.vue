<template>
  <div
    class="item-wrap"
    :class="{'item-wrap-primary': bgPrimary, 'item-wrap-normal': !bgPrimary}"
    :style="style"
    @click="$emit('click')"
  >
    <text class="title" :style="{...titleStyle, ...textStyle}">{{ title }}</text>
    <text class="desc" :style="textStyle" v-if="desc">{{ desc }}</text>
    <div class="link-wrap" v-if="link">
      <text class="link" :style="textStyle" v-if="link">{{ link }}</text>
      <fl-icon class="link" :style="textStyle"  name="arrow-right" />
    </div>
  </div>
</template>
<script>
import { FlIcon } from "falcon-ui";
export default {
  components: { FlIcon },
  props: {
    bg: {
      type: String,
      default: null
    },
    bgPrimary: {
      type: Boolean,
      default: false,
    },
    bgStyle: {
      type: Object,
      default: () => {}
    },
    titleStyle: {
      type: Object,
      default: () => {}
    },
    title: {
      type: String,
      default: null
    },
    desc: {
      type: String,
      default: null
    },
    link: {
      type: String,
      default: null
    },
    mode: {
      type: Number,
      default: 0
    }
  },
  computed: {
    textStyle() {
      return {color: this.bgPrimary ? '#FFFFFF': undefined}
    }
  },
  data() {
    const style = Object.assign({}, {
      backgroundImage: `url(${this.bg})`,
      // backgroundSize: "contain",
      ...this.bgStyle,
    });
    return {
      style
    };
  }
};
</script>
<style lang="less" scoped>
@import "base.less";
.item-wrap {
  padding: @space-normal;
  position: relative;
}
.item-wrap-primary {
  background-color: @primary;
}
.item-wrap-normal {
  background-color: @card-background-color;
}
.title {
  color: @text-color;
  font-size: @font-size-title-small;
}
.desc {
  color: @text-color;
  font-size: @font-size-content-small;
  margin-top: 8px;
}
.link-wrap {
  position: absolute;
  right: @space-normal;
  bottom: @space-normal;
  flex-direction: row;
}
.link {
  color: @text-color;
  font-size: @font-size-content-medium;
  letter-spacing: 0px;
  line-height: @font-size-content-medium;
}
</style>

<template>
  <div class="wrapper">
    <text class="greeting">{{ message }}, YoC!</text>
    <hello-world />
    <text class="jump-message" @click="jump">click to the next page</text>
  </div>
</template>

<script>
import HelloWorld from "@/components/HelloWorld.vue";
export default {
  name: "index",
  components: { HelloWorld },
  data() {
    return {
      message: "",
    };
  },
  methods: {
    sayHello() {
      this.message = "Hello";
    },
    jump() {
      $falcon.navTo("page", { from: "index" });
    },
  },
};
</script>

<style lang="less" scoped>
@import "../../styles/base.less";

.wrapper {
  justify-content: center;
  align-items: center;
}
.greeting {
  text-align: center;
  margin-top: 20px;
  font-size: 50px;
  color: #41b883;
}
.jump-message {
  margin: 24px;
  font-size: 32px;
  color: #727272;
}
.message {
  font-size:20px;
  color: #727272;
  margin-top:12px;
}
.btn {
  .shadow();
}

.fl-doc-demo-block {
  &__title {
    margin: 0;
    padding: 32px 16px 16px;
    color: @text-color-default;
    font-size: 14px;
    line-height: 16px;
  }
}
</style>

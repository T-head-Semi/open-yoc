<template>
  <div class="wrapper">
    <text class="greeting">Multi-page application</text>
    <div style="flex-direction:row;margin-top:20px">
      <text class="btn" @click="jump">back</text>
      <text class="btn" @click="finishPage">finish</text>
    </div>
  </div>
</template>

<script>
const component = {
  name: "page",
  data() {
    return {};
  },
  methods: {
    jump() {
      $falcon.navTo("index", { from: "page" });
    },
    finishApp() {
      this.$app.finish();
    },
    finishPage() {
      this.$page.finish();
    },
  },
};

export default component;
</script>

<style scoped>
.wrapper {
  justify-content: center;
  align-items: center;
}
.greeting {
  text-align: center;
  margin-top: 20px;
  font-size: 34px;
  color: #41b883;
}
.btn{
  margin: 10px;
  padding: 0 40px;
  font-size:24px;
  color: #727272;
}
</style>

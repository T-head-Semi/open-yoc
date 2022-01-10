import DemoScroller from './pages/demos/components/demo-scroller.vue';
export class BasePage extends $falcon.Page {
  constructor() {
    super()
  }
  beforeVueInstantiate(Vue) {
    Vue.component('DemoScroller', DemoScroller);
  }
}
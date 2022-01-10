import IndexComponent from './index.vue';

// import testModule from 'test-module';  //quickjs直接扩展接口调用
// import DemoApi from '$jsapi/test'; //通过jsapi方式扩展原生接口调用

class PageIndex extends $falcon.Page {
  /**
   * 构造函数,页面生命周期内只执行一次
   */
  constructor() {
    super();
  }

  /**
   * 页面生命周期:首次启动
   * @param {Object} options 页面启动参数
   */
  onLoad(options) {
    super.onLoad(options);
    this.setRootComponent(IndexComponent);
  }

  /**
   * 页面生命周期:页面重新进入
   * 其他应用或者系统通过$falcon.navTo()方法重新启动页面.可以通过这个回调拿到新启动的参数
   * @param {Object}} options 重新启动参数
   */
  onNewOptions(options) {
    super.onNewOptions(options);
  }

  /**
   * 页面生命周期:页面进入前台
   */
  onShow() {
    super.onShow();
  }
  
  /**
   * 页面生命周期:页面进入后台
   */
  onHide() {
  }

  /**
   * 页面生命周期:页面卸载
   */
  onUnload() {
  }

}
export default PageIndex;
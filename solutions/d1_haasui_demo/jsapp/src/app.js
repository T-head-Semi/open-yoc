import { BasePage } from './base-page.js';
class App extends $falcon.App {
  /**
   * 构造函数,应用生命周期内只构造一次
   */
  constructor() {
    super();
  }

  /**
   * 应用生命周期:应用启动. 初始化完成时回调,全局只触发一次.
   * @param {Object} options 启动参数
   */
  onLaunch(options) {
    super.onLaunch(options);
    this.setViewPort(800);

    console.log(JSON.stringify($falcon.env));

    // 设置页面基类,应用全局的$falcon.Page将被替换成此处指定的BasePage.
    // 继承自$falcon.Page的页面将继承自改基类.
    // 如页面未指定js,直接指向.vue文件,页面创建时会默认创建该类的实例
    $falcon.useDefaultBasePageClass(BasePage);

    // USAGE: add this in meta.options.style
    // "themes": ["theme-dark", "theme-custom1", "theme-custom2"]
    //
    // fixed theme:
    // $falcon.changeTheme('theme-custom2')
    //
    // auto change theme
    // this.triggerChangeThemes()
  }

  triggerChangeThemes() {
    const themes = this.$meta.options.style.themes
    console.log(themes)
    this.curTheme = 0
    if (!themes) {
      console.log(`no themes`)
    } else {
      setInterval(() => {
        this.curTheme = (this.curTheme + 1) % themes.length
        console.log(`change theme to ${this.curTheme}`)
        $falcon.changeTheme(themes[this.curTheme])
      }, 2000)
    }
  }

  /**
   * 应用生命周期,应用启动或应用从后台切换到前台时触发
   */
  onShow() {
    super.onShow();
  }

  /**
   * 应用生命周期:应用退出前或者应用从前台切换到后台时触发
   */
  onHide() {
    super.onHide();
  }

  /**
   * 应用生命周期:应用销毁前触发
   */
  onDestroy() {
    super.onDestroy();
  }
}

export default App;

App({
  onHide() {
    // 手机切后台后心跳不可依赖：控制器尝试STOP，固件仍必须有看门狗。
    if (this.controller) this.controller.background();
  },
  onShow() {
    if (this.controller) this.controller.foreground();
  }
});


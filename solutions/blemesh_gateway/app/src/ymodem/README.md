
使用串口升级需定义 CONFIG_SUPPORT_YMODEM 宏
串口升级有两种模式(通过宏 UART_PORTING_SYNC 区分)
  同步模式：主动调用串口读取函数去读取串口数据 
  异步模式：使用串口中断接收数据                  (UART_PORTING_SYNC=0)



使用示例：

int baud_rate = ymodem_check();               //决定是否进入串口升级模式 并获取串口升级波特率
if(baud_rate > 0){
    ymodem_init(baud_rate);                   //串口升级初始化 主要为串口初始化
    if (ymodem_upgrade() == 0) {              //进入串口升级流程
        extern void boot_sys_reboot(void);    //串口升级完成 重启开发板
        boot_sys_reboot();
    }
}
ymodem_uninit();                              //串口升级uninit 关闭串口等操作
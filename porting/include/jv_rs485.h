
int jv_rs485_init(void);

int jv_rs485_deinit(void);

//获得485设备句柄
int jv_rs485_get_fd();

//485上锁
void jv_rs485_lock();

//解锁
void jv_rs485_unlock();


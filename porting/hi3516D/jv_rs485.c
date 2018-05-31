#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <jv_common.h>
#include <jv_gpio.h>

enum PARITYBIT_ENUM
{
	PAR_NONE,
	PAR_EVEN,
	PAR_ODD
};

static int fd485 = -1;
static pthread_mutex_t mutex485;  	//485总线互斥量

//函数说明 : 打开串口
//参数     : char *cFileName:串口名
//返回值   : 文件描述符,如果返回-1表示打开失败
static int __DecoderOpenCom(const char *pFileName)
{
    if(NULL == pFileName || 0 == pFileName[0])
    {
        return -1;
    }
    else
    {
        return open(pFileName, O_SYNC | O_RDWR | O_NOCTTY);
    }
}

int jv_rs485_init(void)
{
		
	jv_gpio_muxctrl(0x200F007C, 1); 	//9_3 复用为UART1_RXD
	jv_gpio_muxctrl(0x200F0084, 1); 	//9_5 复用为UART1_TXD
	jv_gpio_muxctrl(0x200F0078, 0); 	//9_2 复用为9_2
	jv_gpio_dir_set_bit(9, 5, 1);		//9_5配置为输出
	jv_gpio_dir_set_bit(9, 3, 0);		//9_3配置为输入 
	jv_gpio_dir_set_bit(9, 2, 1);

	jv_gpio_write(9, 2, 1);			//9_2写1控制485输出数据
	
	return 0;
}

int jv_rs485_deinit(void)
{
	printf("jv_rs485_deinit\n");
	if(fd485 > 0)
		close(fd485);
	pthread_mutex_destroy(&mutex485);
	return 0;
}

//获得485设备句柄
int jv_rs485_get_fd()
{
	return fd485;
}

//485上锁
void jv_rs485_lock()
{
	pthread_mutex_lock(&mutex485);
}

//解锁
void jv_rs485_unlock()
{
	pthread_mutex_unlock(&mutex485);
}


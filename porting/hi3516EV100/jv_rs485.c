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
	if (hwinfo.bHomeIPC == 1)
	{
		return -1;
	}
	
	jv_gpio_muxctrl(0x200f00C0,0x3); 	//6_0复用为UARTRX
	jv_gpio_muxctrl(0x200f00C8,0x3); 	//6_2复用为UARTTX
	jv_gpio_muxctrl(0x200f00C4,0x0); 	//6_1 复用为6_1用来控制485读写转换
	jv_gpio_dir_set_bit(6,1,1);		//6_1配置为输出
	jv_gpio_write(6,1,1);			//6_1写1控制485输出数据
	
	fd485 = __DecoderOpenCom("/dev/ttyAMA1");
	pthread_mutex_init(&mutex485, NULL);

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


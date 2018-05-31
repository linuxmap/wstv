#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "termios.h"
#include "jv_common.h"
#include "jv_gpio.h"
#include "jv_uartcomm.h"

#define SERIAL_BAUD			B9600

#define JV_DOORID_BIT_NUM	24

static jv_thread_group_t uartcomm_group;
static int UartFd = -1; 

static unsigned int _jv_doorID;
static unsigned char _jv_doorID_data[JV_DOORID_BIT_NUM] = {0x00};
static unsigned char _jv_doorID_flag = 0;

jv_uartcomm_func_t g_uartcomm_func;

void __jv_uartcomm_rev(void* ptr)
{
	int ret = 0;
	int len = 0;
	unsigned char rbuf[7] = {0x00};
	unsigned int i = 0;

	fd_set readfds;
	struct timeval timeout;

	pthreadinfo_add((char *)__func__);

	while (uartcomm_group.running)
	{
		FD_ZERO(&readfds);
		FD_SET(UartFd, &readfds);
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		ret = select(UartFd+1, &readfds, NULL, NULL, &timeout);
		if(ret < 0)
		{
			perror("select");
			break;
		}
		else if(ret == 0)
		{
			printf("select timeout\n");
			continue;
		}
		else
		{
			if(FD_ISSET(UartFd, &readfds))
			{
				usleep(100 * 1000);
				len = read(UartFd, rbuf, sizeof(rbuf));

				#if 0
				printf("get data %d: ", len);
				
				for (i = 0; i < len; i++)
				{
					printf("0x%02X ", rbuf[i]);
				}
				#endif

				if (rbuf[2] == 0)
				{
					_jv_doorID = rbuf[6];
					_jv_doorID = _jv_doorID << 8;
					_jv_doorID |= rbuf[5];
					_jv_doorID = _jv_doorID << 8;
					_jv_doorID |= rbuf[4];
					_jv_doorID = _jv_doorID << 8;
					_jv_doorID |= rbuf[3];
					
					printf("0x%08X\n", _jv_doorID);
					_jv_doorID_flag = 1;
				}
			}
		}
	}
	close(UartFd);
}

int _jv_uartcomm_doorID_get(unsigned int* data)
{
	if (_jv_doorID_flag == 1)
	{
		*data = _jv_doorID;
		_jv_doorID_flag = 0;
		return JV_DOORID_BIT_NUM;
	}
	return 0;
}

static void __jv_uartcomm_send(unsigned char cmd, unsigned int data)
{
	unsigned char i = 0;
	unsigned char buf[7] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	buf[2] = cmd;
	buf[3] = data;
	buf[4] = data >> 8;
	buf[5] = data >> 16;
	buf[6] = data >> 24;
	/*如果要发送的数据中有FF，则把buf[1]相应位置1并把数据改为0x33*/
	for (i = 2; i < sizeof(buf); i++)
	{
		if (buf[i] == 0xFF)
		{
			buf[1] = buf[1] | (1 << (i - 2));
			buf[i] = 0x33;
		}
	}

	if (UartFd != -1)
	{
		write(UartFd, buf , sizeof(buf));
		printf("cmd %x, data %x; %x %x %x %x %x %x %x\n", 
			cmd, data, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
	}
}

int __jv_uartcomm_sendB(unsigned char* data, unsigned int len)
{
	int i = 0;
	if (UartFd != -1)
	{
		write(UartFd, data, len);
		for (i = 0; i < len; i++)
		{
			printf("data %x\n", data[i]);
		}
	}
	return 1;
}

void jv_uartcomm_init()
{
	int ret = 0;
	struct termios options;
	//int group, bit;

	if (!hwinfo.bSupportMCU433)
	{
		//不支持此项功能
		printf("uartcomm not support\n");
		return;
	}

	// UartFd = open( "/dev/ttyAMA2", O_RDWR | O_NOCTTY | O_NDELAY);
	UartFd = open( "/dev/ttyAMA1", O_RDWR | O_NOCTTY | O_NDELAY);
	if (UartFd == -1)
	{ 
		printf("uart打开错误\n");
		return;
	}
	ret = tcgetattr(UartFd, &options);
	cfsetispeed(&options, SERIAL_BAUD);
	cfsetospeed(&options, SERIAL_BAUD);
	ret = tcsetattr(UartFd, TCSANOW, &options);
	if (ret != 0)
	{
		printf("uart设置错误\n");
		return;
	}

	options.c_cflag &= ~CSIZE;  
	options.c_cflag |= CS8;   
	options.c_cflag &= ~CSTOPB;
	
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~PARODD;	//even
	options.c_iflag &= ~INPCK;	//enable check

	options.c_cflag |= (CLOCAL | CREAD);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //raw

	options.c_oflag &= ~OPOST;
	options.c_oflag &= ~(OCRNL | ONLCR);	//OA 0D

	options.c_iflag &= ~(ICRNL | INLCR);	//OA 0D
	options.c_iflag &= ~(IXON | IXOFF | IXANY);

	//options.c_cc[VTIME] = 0;
	//options.c_cc[VMIN] = 0;

	tcflush(UartFd, TCIOFLUSH);
	ret = tcsetattr(UartFd, TCSANOW, &options);
	if (ret != 0)
	{
		printf("uart设置错误\n");
		return;
	}

    // jv_gpio_muxctrl(0x200F00CC, 0x03);		// GPIO6_3: UART2_RXD
    // jv_gpio_muxctrl(0x200F00D0, 0x03);		// GPIO6_4: UART2_TXD
    jv_gpio_muxctrl(0x200F00C0, 0x03);		// GPIO6_0: UART1_RXD
    jv_gpio_muxctrl(0x200F00C8, 0x03);		// GPIO6_2: UART1_TXD

	uartcomm_group.running = 1;
	pthread_create(&uartcomm_group.thread, NULL, (void*)__jv_uartcomm_rev, NULL);
	
	memset(&g_uartcomm_func, 0, sizeof(g_uartcomm_func));
	g_uartcomm_func.fptr_uartcomm_doorID_get = _jv_uartcomm_doorID_get;
	g_uartcomm_func.fptr_uartcomm_sendB = __jv_uartcomm_sendB;
	
}


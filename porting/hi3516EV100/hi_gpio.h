#ifndef __HI_GPIO_H__ 
#define __HI_GPIO_H__

#define GPIO_SET_DIR        0x1
#define GPIO_GET_DIR        0x12
#define GPIO_READ_BIT       0x3
#define GPIO_WRITE_BIT      0x4
#define GPIO_READ_DATA	    0x5
#define GPIO_WRITE_DATA	    0x6

#define GPIO_SET_DIR_BIT    0x7
#define GPIO_MUX_CTRL       0x8


typedef struct {
	unsigned int  group;
	unsigned int  bitmask;
	unsigned int  value;
}gpio_groupbit_info;

#endif

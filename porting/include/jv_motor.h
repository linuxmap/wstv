#ifndef __HI_MOTOR_H__ 
#define __HI_MOTOR_H__


enum
{
	MOTOR_ID_LR,
	MOTOR_ID_UD,

	MOTOR_ID_CNT,
};

enum
{
	MOTOR_DRV_A,
	MOTOR_DRV_B,
	MOTOR_DRV_C,
	MOTOR_DRV_D,

	MOTOR_DRV_CNT,

	MOTOR_DRV_CLK	= MOTOR_DRV_A,
	MOTOR_DRV_DR,
	MOTOR_DRV_EN,
};

typedef struct
{
	struct
	{
		unsigned int	mux;		// gpio mux addr
		unsigned char	mlv;		// gpio mux level
		unsigned char	grp;		// gpio group
		unsigned char	pin;		// gpio	pin of group
		unsigned char	elv;		// gpio effective level
	}
	drv_pins[MOTOR_DRV_CNT];
	unsigned char		metercnt;	// motor meter count
	unsigned char		reverse;	// whether motor is reverse or not
	unsigned int		maxstep;	// max step range
}motor_conf_t;

enum
{
	MOTOR_DIRECT_NONE	= 0,

	MOTOR_DIRECT_UP		= 1 << 0,
	MOTOR_DIRECT_DOWN	= 2 << 0,
	MOTOR_DIRECT_LEFT	= 1 << 2,
	MOTOR_DIRECT_RIGHT	= 2 << 2,
};


#define MOTOR_MAGIC			'm'

#define MOTOR_SET_CONF		_IOW	(MOTOR_MAGIC, 0x00, motor_conf_t[MOTOR_ID_CNT])
#define MOTOR_SET_START		_IO		(MOTOR_MAGIC, 0x01)
#define MOTOR_SET_STOP		_IO		(MOTOR_MAGIC, 0x02)
#define MOTOR_SET_DIRECT	_IOW	(MOTOR_MAGIC, 0x03, int)
#define MOTOR_SET_DELAY		_IOW	(MOTOR_MAGIC, 0x04, int)
#define MOTOR_SET_UD_STEP	_IOW	(MOTOR_MAGIC, 0x05, int)
#define MOTOR_SET_LR_STEP	_IOW	(MOTOR_MAGIC, 0x06, int)
#define MOTOR_SET_UD_WORK	_IOW	(MOTOR_MAGIC, 0x07, int)
#define MOTOR_SET_LR_WORK	_IOW	(MOTOR_MAGIC, 0x08, int)
#define MOTOR_GET_DONE		_IOR	(MOTOR_MAGIC, 0x09, int)
#define MOTOR_GET_UD_STEP	_IOR	(MOTOR_MAGIC, 0x0a, int)
#define MOTOR_GET_LR_STEP	_IOR	(MOTOR_MAGIC, 0x0b, int)
#define MOTOR_DUMP			_IO		(MOTOR_MAGIC, 0x80)


#endif

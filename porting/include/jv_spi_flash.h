#ifndef	SPI_FLASH_H
#define	SPI_FLASH_H

#include <asm-generic/ioctl.h>

#define	SPI_FLASH_DEVNAME		"/dev/spi_flash"

#define	FLASH_MAGIC		'T'
#define	SPI_WRITE_PROTECT		_IOW(FLASH_MAGIC, 11, int)
#define	SPI_READ_STATUS			_IOW(FLASH_MAGIC, 12, int)

#define	SPI_FLASH_TYPE			_IOW(FLASH_MAGIC, 20, int)


typedef enum {GD, WINBOND, ISSI, MX}flash_type_e;	//flash type

typedef enum flash_protect_partial{
	flash_8M_8M 		= 0x0,		//protect all 8M
	flash_7875M_8M 		= 0x4,		//protect first 7.785M of 8M
	flash_775M_8M 		= 0x8,		//protect first 7.75M of 8M
	flash_75M_8M		= 0xc,		//protect first 7.5M of 8M
	flash_7M_8M			= 0x10,		//protect first 7M of 8M
	flash_6M_8M			= 0x14,		//protect first 6M of 8M
	flash_4M_8M			= 0x18,		//protect first 4M of8M
	flash_0M_8M			= 0x1c,		//protect none of 8M
}FLASH_PROTECT_PARTIAL;


void jv_flash_write_lock_init();
void jv_flash_write_lock();
void jv_flash_write_unlock();
void jv_flash_write_lock_deinit();

#endif

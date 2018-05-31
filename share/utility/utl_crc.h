/*
 * utl_crc.h
 *
 *  Created on: 2015Äê1ÔÂ14ÈÕ
 *      Author: LiuFengxiang
 *		 Email: lfx@jovision.com
 */

#ifndef UTL_CRC_H_
#define UTL_CRC_H_

#ifdef __cplusplus
extern "C" {
#endif

unsigned char ult_crc8(unsigned char * data, int length);

unsigned int utl_crc16(unsigned char *ptr, unsigned char len);

unsigned long utl_crc32(const unsigned char *s, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif /* UTL_CRC_H_ */

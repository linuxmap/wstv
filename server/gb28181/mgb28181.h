/*
 * mgb28181.h
 *
 *  Created on: 2013-11-11
 *      Author: lfx
 */

#ifndef MGB28181_H_
#define MGB28181_H_

#include <gb28181.h>

int mgb28181_init(void);

int mgb28181_set_param(GBRegInfo_t *param);

int mgb28181_get_param(GBRegInfo_t *param);

int mgb28181_deinit(void);

#define mgb_send_data_i gb_send_data_i
#define mgb_send_data_p gb_send_data_p
#define mgb_send_data_a gb_send_data_a
#define mgb_send_alarm gb_send_alarm

#endif /* MGB28181_H_ */

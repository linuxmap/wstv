/*
 ============================================================================
 Name        : sound_wave_dec.h
 Author      : Liuchen
 Version     :
 Copyright   :
 Description : sound wave transmit decode module
 Created on  : 2015-05-21
 ============================================================================
 */

#ifndef  _MODULE_DEC_H_
#define  _MODULE_DEC_H_

typedef void (*sound_wave_dec_done_cb)(void * str);

void * sound_wave_dec_init(int input_step);
int sound_wave_dec_finalize(void *h_module_dec);
int sound_wave_dec_regist_done_cb(void *h_module_dec, sound_wave_dec_done_cb cb);
int sound_wave_dec_w_buf(void *h_module_dec, void *p_data, int len);

#endif

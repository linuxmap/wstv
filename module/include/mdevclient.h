#ifndef DEVCLIENT_H_
#define DEVCLIENT_H_

#include "jv_common.h"
#include "malarmout.h"


/**
 *@brief 向服务器发送报警消息
 *@param alarm     报警信息
 *
 *@return
 */
void mdevSendAlarm2Server(JV_ALARM *alarm);



#endif



#ifndef __YST_FIRMUP_H__
#define __YST_FIRMUP_H__


//扩展远程控制指令,lck20120203
#define EX_UPLOAD_START		0x01
#define EX_UPLOAD_CANCEL	0x02
#define EX_UPLOAD_OK		0x03
#define EX_UPLOAD_DATA		0x04
#define EX_FIRMUP_START		0x05
#define EX_FIRMUP_STEP		0x06
#define EX_FIRMUP_OK		0x07
#define EX_FIRMUP_RET		0x08
#define EX_FIRMUP_REBOOT	0XA0
#define EX_FIRMUP_RESTORE	0xA1
#define EX_FIRMUP_UPDINFO_REQ	0x11	//获取升级所需信息
#define EX_FIRMUP_UPDINFO_RESP	0x21
#define EX_FIRMUP_BACKGROUND	0x22

#define EX_MT_ONEKEY_DIAG	0x30
#define EX_MT_ONEKEY_DIAG_RESP	0x31

void FirmupProc(REMOTECFG *cfg);

#endif


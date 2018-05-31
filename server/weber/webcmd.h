#ifndef _WEBCMD_H_
#define _WEBCMD_H_

typedef enum{
	OPER_NEED_ADMIN,  //不同的CGI操作需要不同的权限才可以执行
	OPER_NEED_USER,
	OPER_NEED_GUEST,
	OPER_ERROR
}Oper_Type_e;

Oper_Type_e _webcmd_checkOperation(char *cmd, char *action);
int _webcmd_checkpower(char *cmd, char *action, char *id, char *passwd);
int webcmd_account(int argc, char *argv[]);
int webcmd_alarm(int argc, char *argv[]);
int webcmd_mdetect(int argc, char *argv[]);
int webcmd_log(int argc, char *argv[]);
int webcmd_privacy(int argc, char *argv[]);
int webcmd_record(int argc, char *argv[]);
int webcmd_storage(int argc, char *argv[]);
int webcmd_stream(int argc, char *argv[]);
int webcmd_network(int argc, char *argv[]);
int webcmd_osd(int argc, char *argv[]);
int webcmd_snapshot(int argc, char *argv[]);

int webcmd_expose(int argc, char *argv[]);
int webcmd_white(int argc, char *argv[]);
int webcmd_gamma(int argc, char *argv[]);
int webcmd_sharpen(int argc, char *argv[]);
int webcmd_ccm(int argc, char *argv[]);
int webcmd_image_adjust(int argc, char *argv[]);

int webcmd_yst(int argc, char *argv[]);
int webcmd_multimedia(int argc, char *argv[]);
int webcmd_ptz(int argc, char *argv[]);
int webcmd_led(int argc, char *argv[]);
int webcmd_adetect(int argc, char *argv[]);
#endif


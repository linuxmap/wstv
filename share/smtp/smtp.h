
#ifndef _SMTP_H_
#define _SMTP_H_

#ifdef __cplusplus
extern "C" {
#endif

int smtp(const char *servername
		, unsigned short port
		, const char *security
		, const char *login
		, const char *passwd
		, const char *from
		, const char *subject
		, const char *text
		, const char *charset
		, const char *attachments //Can be more than one, such as : a.txt,b.txt,c.txt
		, const char *recipients
		);

const char *smtp_get_errmsg(int error, char *errDst);

void smtp_test();

#ifdef __cplusplus
}
#endif

#endif

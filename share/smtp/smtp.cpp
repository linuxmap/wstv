#include "CSmtp.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

#include <unistd.h>

#include "smtp.h"

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
		)
{
	//天啊，为啥在这里就不能catch到异常呢？
	//害得我只能把函数加到类内部
	int ret;
	try{
	CSmtp mail;
	ret = mail.SendDirect(servername
		, port
		, security
		, login
		, passwd
		, from
		, subject
		, text
		, charset
		, attachments //Can be more than one, such as : a.txt,b.txt,c.txt
		, recipients
		);
	}catch(...)
	{
		printf("==========>catched CSmtp\n");
	}
	return ret;
}


const char *smtp_get_errmsg(int error, char *errDst)
{
	ECSmtp e((ECSmtp::CSmtpError)error);
	strcpy(errDst, e.GetErrorText().c_str());
	return errDst;
}

void smtp_test()
{
//	smtp("smtp.jovision.com", 25, "none", "lfx@jovision.com", "123456", "lfx@jovision.com", "warning", "hahahaha", "GB2312", "./startup.sh", "lfx@jovision.com");
//	smtp("smtp.qq.com", 465, "ssl", "20451250@qq.com", "", "20451250@qq.com", "warning", "hahahaha", "GB2312", "./startup.sh", "lfx@jovision.com");
	smtp("smtp.163.com", 465, "ssl", "wavesemail@163.com", "", "wavesemail@163.com", "warning", "hahahaha", "GB2312", "./startup.sh", "lfx@jovision.com");

//	CSmtp mail;
//	mail.testSMTP();

	while(1)
		usleep(1000);
}

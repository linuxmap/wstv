/******************************************************************************

  Copyright (c) 2015 Jovision Technology Co., Ltd. All rights reserved.

  File Name     : ftp_client.h
  Version       : 
  Author        : Qin Lichao <qinlichao@jovision.com>
  Created       : 2015-6-12
  Description   : FTP client, upload or download file from FTP server.
  History       : 
  1.Date        : 2015-6-12
    Author      : Qin Lichao <qinlichao@jovision.com>
    Modification: Created file
******************************************************************************/
/*
错误代码规则

错误跟http协议类似，大致是：
2开头－－成功
3开头－－权限问题
4开头－－文件问题
5开头－－服务器问题

常见的错误信息
-----------------------------------
120 Service ready in NNN minutes.
服务在NNN时间内可用
-----------------------------------
125 Data connection already open; transfer starting.
数据连接已经打开，开始传送数据.
-----------------------------------
150 File status okay; about to open data connection.
文件状态正确，正在打开数据连接.
-----------------------------------
200 Command okay.
命令执行正常结束.
-----------------------------------
202 Command not implemented, superfluous at this site.
命令未被执行，此站点不支持此命令.
-----------------------------------
211 System status, or system help reply.
系统状态或系统帮助信息回应.
-----------------------------------
212 Directory status.
目录状态信息.
-----------------------------------
213 File status. $XrkxmL=
文件状态信息.
-----------------------------------
214 Help message.On how to use the server or the meaning of a particular non-standard command. This reply is useful only to the human user.
帮助信息。关于如何使用本服务器或特殊的非标准命令。
-----------------------------------
215 NAME system type. Where NAME is an official system name from the list in the Assigned Numbers document.
NAME系统类型。
-----------------------------------
220 Service ready for new user.
新连接的用户的服务已就绪
-----------------------------------
221 Service closing control connection.
控制连接关闭
-----------------------------------
225 Data connection open; no transfer in progress.
数据连接已打开，没有进行中的数据传送
-----------------------------------
226 Closing data connection. Requested file action successful (for example, file transfer or file abort).
正在关闭数据连接。请求文件动作成功结束（例如，文件传送或终止）
-----------------------------------
227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
进入被动模式
-----------------------------------
230 User logged in, proceed. Logged out if appropriate.
用户已登入。 如果不需要可以登出。
-----------------------------------
250 Requested file action okay, completed.
被请求文件操作成功完成 63
-----------------------------------
257 "PATHNAME" created.
路径已建立
-----------------------------------
331 User name okay, need password.
用户名存在，需要输入密码
-----------------------------------
332 Need account for login.
需要登陆的账户
-----------------------------------
350 Requested file action pending further inFORMation U
对被请求文件的操作需要进一步更多的信息
-----------------------------------
421 Service not available, closing control connection.This may be a reply to any command if the service knows it must shut down.
服务不可用，控制连接关闭。这可能是对任何命令的回应，如果服务认为它必须关闭
-----------------------------------
425 Can't open data connection.
打开数据连接失败
-----------------------------------
426 Connection closed; transfer aborted.
连接关闭，传送中止。
-----------------------------------
450 Requested file action not taken.
对被请求文件的操作未被执行
-----------------------------------
451 Requested action aborted. Local error in processing.
请求的操作中止。处理中发生本地错误。
-----------------------------------
452 Requested action not taken. Insufficient storage space in system.File unavailable (e.g., file busy).
请求的操作没有被执行。系统存储空间不足。 文件不可用
-----------------------------------
500 Syntax error, command unrecognized. This may include errors such as command line too long.
语法错误，不可识别的命令。 这可能是命令行过长。
-----------------------------------
501 Syntax error in parameters or arguments.
参数错误导致的语法错误
-----------------------------------
502 Command not implemented.
命令未被执行
-----------------------------------
503 Bad sequence of commands.
命令的次序错误。
-----------------------------------
504 Command not implemented for that parameter.
由于参数错误，命令未被执行
-----------------------------------
530 Not logged in.
没有登录
-----------------------------------
532 Need account for storing files.
存储文件需要账户信息!
-----------------------------------
550 Requested action not taken. File unavailable (e.g., file not found, no access).
请求操作未被执行，文件不可用。
-----------------------------------
551 Requested action aborted. Page type unknown.
请求操作中止，页面类型未知
-----------------------------------
552 Requested file action aborted. Exceeded storage allocation (for current directory or dataset).
对请求文件的操作中止。 超出存储分配
-----------------------------------
553 Requested action not taken. File name not allowed
请求操作未被执行。 文件名不允许
-----------------------------------
*/

/**
 *@brief 连接FTP服务器
 *@param host FTP服务器IP地址
 *@param port FTP服务器服务端口
 *@param user FTP服务器登录用户名
 *@param pwd FTP服务器登录密码
 *@return >0 返回连接的socket，-1连接失败
 */
int ftp_connect(char *host, int port, char *user, char *pwd);

/**
 *@brief 关闭FTP连接
 *@param c_sock 连接socket
 *@return 221关闭成功，其他失败
 */
int ftp_quit(int c_sock);

/**
 *@brief 设置传输模式
 *@param c_sock 连接socket
 *@param mode FTP传输模式 : A=ASCII，E=EBCDIC，I=binary
 *@return 0成功，-1失败
 */
int ftp_type(int c_sock, char mode);

/**
 *@brief 改变工作目录
 *@param c_sock 连接socket
 *@param path 要切换到的目录路径
 *@return 0成功，-1失败
 */
int ftp_cwd(int c_sock, char *path);

/**
 *@brief 回到上一层目录
 *@param c_sock 连接socket
 *@return 0成功，其他失败
 */
int ftp_cdup(int c_sock);

/**
 *@brief 创建目录
 *@param c_sock 连接socket
 *@param path 要创建的目录路径
 *@return 0成功，其他失败
 */
int ftp_mkd(int c_sock, char *path);

/**
 *@brief 列表
 *@param c_sock 连接socket
 *@param path 目录路径
 *@param data 文件列表
 *@param data_len 文件列表长度
 *@return 0成功，其他失败
 */
int ftp_list(int c_sock, char *path, void **data, unsigned long long *data_len);

/**
 *@brief 下载文件
 *@param c_sock 连接socket
 *@param s 源文件路径
 *@param d 目标文件路径
 *@param stor_size 文件大小
 *@param stop 立即停止
 *@return 0成功，其他失败
 */
int ftp_retrfile(int c_sock, char *s, char *d ,unsigned long long *stor_size, int *stop);

/**
 *@brief 上传文件
 *@param c_sock 连接socket
 *@param s 源文件路径
 *@param d 目标文件路径
 *@param stor_size 文件大小
 *@param stop 立即停止
 *@return 0成功，其他失败
 */
int ftp_storfile(int c_sock, char *s, char *d ,unsigned long long *stor_size, int *stop);

/**
 *@brief 修改文件名&移动目录
 *@param c_sock 连接socket
 *@param s 源文件路径
 *@param d 目标文件路径
 *@return 0成功，其他失败
 */
int ftp_renamefile(int c_sock, char *s, char *d);

/**
 *@brief 删除文件
 *@param c_sock 连接socket
 *@param s 要删除的文件路径
 *@return 0成功，其他失败
 */
int ftp_deletefile(int c_sock, char *s);

/**
 *@brief 删除目录
 *@param c_sock 连接socket
 *@param s 要删除的目录路径
 *@return 0成功，其他失败
 */
int ftp_deletefolder(int c_sock, char *s);

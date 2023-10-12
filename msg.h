#ifndef __MSG_H__
#define __MSG_H__

#include"pkg.h"
#include"cmd.h"
#include"client.h"

typedef struct ftpmsg // FTP消息 信息结构体
{
    enum CMD_NO type; // 消息类型
    int len;               // 数据长度
    uchar *data;            // 数据内容
}FTPmsg;



int recv_ls(pkgInfo_resp pkg,int sock_fd,char* dir); // 发送ls指令
//接收服务器的cd回复  ,  切换服务器的工作路径
int recv_cd(int sock_fd,char* dir);
// 接收 服务器创建路径 的消息回复
int recv_mkdir(int sockfd, char *dir);
//发送FTP消息
int send_msg(int sockfd, struct ftpmsg *msg);
//接收FTP消息
int recv_msg(int sockfd, struct ftpmsg *msg);
//发送指定的消息类型信号
int send_simple(int sockfd, enum CMD_NO type);
//构建并发送指定的数据包
int send_pkg(const char* cmd,char* content,int sock_fd);

#endif
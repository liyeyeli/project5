#ifndef __CMD_H__
#define __CMD_H__

#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include"../inc/pkg.h"
#include <pthread.h>

#define uchar unsigned char

//IP地址及套接字信息结构体（供函数传递使用）
typedef struct IPinfo
{
    struct sockaddr_in Cli;
    struct sockaddr_in Ser;   
    int Cli_sock,Ser_sock;
}IPinfo;


//获取文件 并发送（通用型）
int send_file(char* filename,int sock_fd);
//接收文件
int recv_file(pkgInfo_resp pkg,int sock_fd,char* filenames);
// 从文件路径获取文件名
char *file_name(char *filename, const char *path);
// 将路径转换为绝对路径
char *full_path(char *path);
//创建目录
int create_dir(char *dir_name);

int cmd_ls(int sockfd,char* dir);
//切换服务器工作目录
int cmd_cd(int sock_fd,char* dir);
int cmd_lcd(char *dir);
int cmd_lls(char* dir);
//服务器创建目录
int cmd_mkdir(int sockfd, char *dir);
//帮助菜单
void cmd_help();

//线程用的客户端命令执行函数
void *client_handler(void *arg) ;

#endif
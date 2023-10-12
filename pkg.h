#ifndef __PKG_H__
#define __PKG_H__

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define uchar unsigned char
#define MAX_LENGTH 1000

//命令号+消息类型
enum CMD_NO
{
	FTP_CMD_LS = 1024,
	FTP_CMD_GET,
	FTP_CMD_PUT,
	FTP_CMD_CD,  
	FTP_CMD_MKDIR, 
	FTP_CMD_BYE,
	
	FILE_END, // 表示文件传输完毕
	FILE_NAME, //发送文件名
	FILE_MODE,
	FILE_SIZE,
	FAILURE,
	SUCCESS,
	FILE_DATA,
	
	FTP_CMD_UNKOWN = 0xFFFFFFFF
};		
//数据包信息结构体(	客户端请求命令格式)
typedef struct pkg1
{
	uchar pkg_start;  	//包头
	int pkg_len;    			//数据长
	enum CMD_NO cmd_no;     	//命令号
	int arg_1_len;    //参数长  
	char *arg_1_data; //参数名
	uchar pkg_end;   	//包尾

}pkgInfo_rqst;
//数据包信息结构体(	服务器回复命令格式)
typedef struct pkg2
{
	uchar pkg_start;  	//包头
	int pkg_len;    			//数据长
	enum CMD_NO cmd_no;     	//命令号
	int resp_len;  				//回复内容的长度
	int result ;				//表示命令成功或失败
	uchar *resp_content; 	//回复的内容
	uchar pkg_end;   	//包尾
}pkgInfo_resp;


//回复数据包
uchar* pkg_resp(const uchar* content,int* len,enum CMD_NO mode);
//构建数据包
uchar* pkg_get(const char* content,int* len,enum CMD_NO mode);
//解析数据包
pkgInfo_rqst handle_pkg_rqst(uchar recv_buf[],int sockfd,int* len);
pkgInfo_resp handle_pkg_resp(uchar recv_buf[],int sockfd,int* len);

//打印数据包命令格式
void print_command(const uchar *cmd, int length);

#endif
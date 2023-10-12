//--------------------数据包处理程序-------------------------
#include"../inc/pkg.h"
#include"../inc/cmd.h"


//构造通信协议数据包格式

/*	构建指定数据包	 
	命令格式:0xc0 pkg_len cmd_no arg_1_len  arg_1_data 0xc0
	@content:传入内容数据arg_1_data
	@mode:指定命令
*/
uchar* pkg_get(const char* content,int* len,enum CMD_NO mode)
{
	int i = 0;
	uchar cmd[1024]={0};
	int arg_1_len = strlen(content); 
	int pkg_len = 12+arg_1_len;
	enum CMD_NO cmd_no = mode;
	cmd[i++]=0xc0;//包头
	cmd[i++]= pkg_len & 0xFF;
	cmd[i++]= (pkg_len >> 8) & 0xFF;
	cmd[i++]= (pkg_len >> 16) & 0xFF;
	cmd[i++]= (pkg_len >> 24) & 0xFF;	
	cmd[i++]= cmd_no & 0xFF;
	cmd[i++]= (cmd_no >> 8) & 0xFF;
	cmd[i++]= (cmd_no >> 16) & 0xFF;
	cmd[i++]= (cmd_no >> 24) & 0xFF;
	//arg_1_len
	cmd[i++]= arg_1_len & 0xFF;
	cmd[i++]= (arg_1_len >> 8) & 0xFF;
	cmd[i++]= (arg_1_len >> 16) & 0xFF;
	cmd[i++]= (arg_1_len >> 24) & 0xFF;
	//arg_1_data
	strncpy((char*)(cmd+i),content,arg_1_len);
	i+=arg_1_len;  
	cmd[i++]= 0xc0;
	uchar* buf = cmd;
	*len =i;
	return buf;
}



/*	回复内容数据包
0xc0   pkg_len(4)  cmd_no(4)   resp_len(4)   result(1)  filenames(名字以空格隔开)  0xc0
@content:回复的内容字符串
@len:供调用的数据包长度
@mode:命令号/消息类型
result -> 占1个字节，表示命令成功或者失败 为1表示成功   为0表示失败
准备所有的文件名(opendir/readdir读取目录，
得到所有的文件名)放在一个足够长的数组中保存起来
*/
uchar* pkg_resp(const uchar* content,int* len,enum CMD_NO mode)
{
	int i = 0;
	//读取目录获取所有的文件名存在filenames
	uchar cmd[1024]={0};
	int x = strlen((const char *)content);
	int pkg_len = 13+x;
	enum CMD_NO cmd_no = mode;
	int resp_len = 1+x;
	unsigned char result = 1;
	cmd[i++]=0xc0;//包头

	cmd[i++]= pkg_len & 0xFF;//数据长
	cmd[i++]= (pkg_len >> 8) & 0xFF;
	cmd[i++]= (pkg_len >> 16) & 0xFF;
	cmd[i++]= (pkg_len >> 24) & 0xFF;	

	cmd[i++]= cmd_no & 0xFF;//命令号
	cmd[i++]= (cmd_no >> 8) & 0xFF;
	cmd[i++]= (cmd_no >> 16) & 0xFF;
	cmd[i++]= (cmd_no >> 24) & 0xFF;

	cmd[i++]= resp_len & 0xFF;//返回参数长
	cmd[i++]= (resp_len >> 8) & 0xFF;
	cmd[i++]= (resp_len >> 16) & 0xFF;
	cmd[i++]= (resp_len >> 24) & 0xFF;
	cmd[i++] =  result;
	strncpy((char*)(cmd+i),(const char*)(content),x);//将返回内容存入(char*)(cmd+i)中
	i+=x;
	cmd[i++]=0xc0;
	uchar* buf = cmd;
	*len = i;
	return buf;
}


/* 	
	解析请求数据包
	@recv_buf:数据包
	@sockfd:发送者套接字
*/
pkgInfo_rqst handle_pkg_rqst(uchar recv_buf[],int sockfd,int* len)
{
	int i = 0;
	pkgInfo_rqst pkg;
	pkg.arg_1_data = malloc(100);//给参数名开一个100大小的空间
	pkg.pkg_start = recv_buf[i++];//分析包头
	if (pkg.pkg_start != 0xc0)//根据应用层协议判断包头是否为0xc0
	{
		// 错误处理：包头不正确
		close(sockfd);
		printf("命令数据包错误\n");
		recv_buf = NULL;
		return pkg;
	}
	//包头正确
		//开始解析
		pkg.pkg_len = (recv_buf[i++]) |
					(recv_buf[i++] << 8) |
					(recv_buf[i++] << 16) |
					(recv_buf[i++] << 24);

		pkg.cmd_no = (recv_buf[i++]) |
						(recv_buf[i++] << 8) |
						(recv_buf[i++] << 16) |
						(recv_buf[i++] << 24);
		pkg.arg_1_len = (recv_buf[i++]) |
						(recv_buf[i++] << 8) |
						(recv_buf[i++] << 16) |
						(recv_buf[i++] << 24);
		strncpy((char *)pkg.arg_1_data, (const char *)(recv_buf + i), pkg.arg_1_len);//接收到的参数名全部存入pkg.arg_1_data中
		i += pkg.arg_1_len;
	//包尾
	pkg.pkg_end = recv_buf[i++];
	recv_buf = NULL;
	*len = i;
	return pkg;
}

/* 	
	解析回复数据包
	@recv_buf:数据包
	@sockfd:发送者套接字
*/
pkgInfo_resp handle_pkg_resp(uchar recv_buf[],int sockfd,int* len)
{
	int i = 0;
	pkgInfo_resp pkg;
	pkg.pkg_start = recv_buf[i++];
	if (pkg.pkg_start != 0xc0)
	{
		// 错误处理：包头不正确
		close(sockfd);
		printf("回复数据包错误\n");
		return pkg;
	}
	//解析...
	pkg.pkg_len = (recv_buf[i++]) |
				(recv_buf[i++] << 8) |
				(recv_buf[i++] << 16) |
				(recv_buf[i++] << 24);
	pkg.cmd_no = (recv_buf[i++]) |
						(recv_buf[i++] << 8) |
						(recv_buf[i++] << 16) |
						(recv_buf[i++] << 24);
	// 解析 resp_len
    pkg.resp_len = recv_buf[i++];
    pkg.resp_len |= recv_buf[i++] << 8;
    pkg.resp_len |= recv_buf[i++] << 16;
    pkg.resp_len |= recv_buf[i++] << 24;	
	// 解析 result
    pkg.result = recv_buf[i++];			
	// 解析 resp_content
    pkg.resp_content = ( uchar *)malloc(pkg.resp_len - 1); // -1 为 result 的长度
    memcpy(pkg.resp_content, recv_buf + i, pkg.resp_len - 1);
	//包尾
	pkg.pkg_end = recv_buf[i++];
	*len = i;
	return pkg;
}

//打印数据包命令格式（测试用）
void print_command(const uchar *cmd, int length) {
    printf("数据包：");
	for (int i = 0; i < length; i++) {
        printf("%02x ", cmd[i]);
    }
    printf("\n");
}

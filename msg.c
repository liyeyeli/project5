//------------------通信程序-----------------------------
#include"../inc/msg.h"
#include"../inc/pkg.h"

/*接收服务器的ls回复
    @pkg:回复信息用的数据包
    @sock_fd:目标套接字
    @dir:指定显示的目录
*/
int recv_ls(pkgInfo_resp pkg,int sock_fd,char* dir)
{
    struct ftpmsg msg;
    msg.type = FTP_CMD_LS;
    recv_msg(sock_fd, &msg);
    pkg = handle_pkg_resp(msg.data,sock_fd,&msg.len);
    if (msg.type == SUCCESS)
    {
        if (!strcmp((char*)pkg.resp_content,""))
        {
            printf("服务器目录%s为空\n",dir);
        }
        else
        {
            printf("服务器目录%s下的内容：\n%s\n", dir, pkg.resp_content);
        }
        return 0;
    }
    else if (msg.type == FAILURE)
    {
        fprintf(stderr, "获取服务器文件列表失败\n");
        return -1;
    }
    else
    {
        fprintf(stderr, "ls命令收到意外消息\n");
        return -1;
    }
}

//接收服务器的cd回复  ,  切换服务器的工作路径
int recv_cd(int sock_fd,char* dir)
{
    struct ftpmsg msg;
    recv_msg(sock_fd, &msg);
    if (msg.type == SUCCESS)
    {
        //printf("切换服务器工作路径成功\n");
        return 0;
    }
    else if (msg.type == FAILURE)
    {
        fprintf(stderr, "切换服务器工作路径失败\n");
        return -1;
    }
    else
    {
        fprintf(stderr, "cd命令收到意外消息\n");
        return -1;
    }
}

// 接收 服务器创建路径 的消息回复
int recv_mkdir(int sockfd, char *dir)
{
    struct ftpmsg msg;
    recv_msg(sockfd, &msg);
    if (msg.type == SUCCESS)
    {
        // printf("服务器创建目录%s成功\n", dir);
    }
    else if (msg.type == FAILURE)
    {
        fprintf(stderr, "服务器创建目录%s失败\n", dir);
    }
    else
    {
        fprintf(stderr, "mkdir命令收到意外消息\n");
    }
    return 0;
}

//发送FTP消息
int send_msg(int sock_fd, struct ftpmsg *msg)
{
    int result;
    // 发送type
    result = send(sock_fd, &msg->type, sizeof(int), 0);
    if (result == -1)
    {
        fprintf(stderr, "send函数出错，type发送失败\n");
        return -1;
    }
    // printf("> 发送信息：type = %d", msg->type);

    // 发送len
    result = send(sock_fd, &msg->len, sizeof(int), 0);
    if (result == -1)
    {
        fprintf(stderr, "send函数出错，len发送失败\n");
        return -1;
    }
    // printf(", len = %d", msg->len);

    // 发送data
    // printf(", data = %-30.30s\n", msg->data);
    if (msg->len == 0)
    {
        return 0;
    }
    int sent = 0;           // 总共已发送的位数
    while (sent < msg->len) // 循环直到data发送完毕
    {
        result = send(sock_fd, msg->data + sent, msg->len - sent, 0);
        // result返回值为-1表示发送失败，否则为本次发送成功的位数
        if (result == -1)
        {
            fprintf(stderr, "send函数出错，data发送失败\n");
            return -1;
        }
        else
        {
            sent += result;
        }
    }
    return 0;
}
//接收FTP消息
int recv_msg(int sock_fd, struct ftpmsg *msg)
{
    int result;
    // 接收type
    result = recv(sock_fd, &msg->type, sizeof(int), 0);
    if (result == -1)
    {
        if (errno == EWOULDBLOCK)
        {
            // 没有收到数据
            printf("没有收到数据\n");
            return 1;
        }
        fprintf(stderr, "recv函数出错，type接收失败\n");
        return -1;
    }
    // printf("< 接收信息：type = %d", msg->type);

    // 接收len
    result = recv(sock_fd, &msg->len, sizeof(int), 0);
    if (result == -1)
    {
        fprintf(stderr, "recv函数出错，len接收失败\n");
        return -1;
    }
    // printf(", len = %d", msg->len);

    // 接收data
    if (msg->len == 0)
    {
        msg->data = NULL;
    }
    else
    {
        msg->data = malloc(msg->len);
        if (msg->data == NULL)
        {
            fprintf(stderr, "malloc函数出错，接收消息时动态内存分配失败\n");
            return -1;
        }
        else
        {
            // printf("分配了%d位\n", msg->len);
        }
        int recvd = 0;           // 总共已接收的位数
        while (recvd < msg->len) // 循环直到data接收完毕
        {
            result = recv(sock_fd, msg->data + recvd, msg->len - recvd, 0);
            if (result == -1)
            {
                fprintf(stderr, "recv函数出错，data接收失败\n");
                return -1;
            }
            else
            {
                recvd += result;
            }
        }
    }
    // printf(", data = %-30.30s\n", msg->data);
    return 0;
}

//发送指定的消息类型信号
int send_simple(int sock_fd, enum CMD_NO type)
{
    struct ftpmsg msg;
    msg.type = type;
    msg.len = 0;
    msg.data = NULL;
    return send_msg(sock_fd, &msg);
}


/*	构建并发送指定的数据包
	@cmd:指定命令
	@content:构建数据包所需的内容参数
	@sock_fd:指定套接字
*/
int send_pkg(const char* cmd,char* content,int sock_fd)
{
	int len=0;
	int ret1=0;
	uchar* send_buf;
	if (!strncmp(cmd, "get",3) || !strncmp(cmd, "put",3))
	{
		if (content == NULL|| !strcmp(content, ""))
		{// 若参数被省略
			fprintf(stderr, "错误：需要参数，正确用法为：命令 <目标文件路径>\n");
			return 0;
		}
		if (!strncmp(cmd, "get",3))
		send_buf = pkg_get(content,&len,FTP_CMD_GET);
		else
		send_buf = pkg_get(content,&len,FTP_CMD_PUT);
		ret1 = send(sock_fd,send_buf,1000,0);
		if (ret1 > 0)//指令发送成功
		{
			//print_command(send_buf,len);//test显示数据包
			return 1;
		}
	}
	else if (!strncmp(cmd, "ls",2))
	{
		if (content == NULL || !strcmp(content, ""))
		{// 若参数被省略，则以当前路径作为参数
			strcpy(content, ".");
		}
		send_buf = pkg_get(content,&len,FTP_CMD_LS);
		ret1 = send(sock_fd,send_buf,1000,0);
	}
	else if (!strncmp(cmd, "cd",2))
	{
		if (content == NULL || !strcmp(content, ""))
		{
			fprintf(stderr, "错误：需要参数，正确用法为：cd <服务器路径>\n");
        	return 0;
		}
		send_buf = pkg_get(content,&len,FTP_CMD_CD);
		ret1 = send(sock_fd,send_buf,1000,0);
	}
    else if (!strncmp(cmd, "mkdir",5))
    {
        if (content == NULL)
        {
            fprintf(stderr, "错误：需要参数，正确用法为：mkdir <服务器路径>\n");
            return -1;
        }
        send_buf = pkg_get(content,&len,FTP_CMD_MKDIR);
		ret1 = send(sock_fd,send_buf,1000,0);
    }
	else if(!strcmp(cmd, "exit") || !strcmp(cmd, "quit") || !strcmp(cmd, "q"))
	{
		if (content == NULL || !strcmp(content, ""))
		{// 若参数被省略，则以当前路径作为参数
			strcpy(content, ".");
		}
		send_buf = pkg_get(content,&len,FTP_CMD_BYE);
		ret1 = send(sock_fd,send_buf,1000,0);
	}
	else
	{
		return 0;
	}
	if (ret1 > 0)//指令发送成功
	{
		//print_command(send_buf,len);//test显示数据包
		return 1;
	}
	return 0;
}
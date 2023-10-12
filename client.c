#include"../inc/client.h"
#include"../inc/pkg.h"
#include"../inc/cmd.h"
#include"../inc/msg.h"
int main(int argc,char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
        return -1;
    }
    //创建套接字
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == sock_fd)
	{
		perror("socket failed!!");
		return -1;
	}

    //2.服务器的地址 
	struct sockaddr_in Ser;
	Ser.sin_family = AF_INET;
	Ser.sin_port = htons(atoi(argv[2]));//"6666"-->6666
	Ser.sin_addr.s_addr = inet_addr(argv[1]);

    //3.根据服务器的IP+PORT发起连接 
	int ret = connect(sock_fd,(struct sockaddr *)&Ser,sizeof(Ser));
	if(-1 == ret)
	{
		perror("connect failed!!");
		close(sock_fd);
		return -1;
	}
	//回复数据包
	pkgInfo_resp pkg;
	char cmd[1024];//输入命令
    while(1)
    {
		fprintf(stderr,"myftp >  ");
		fgets(cmd,1024,stdin);
		// 解析命令，提取文件路径(如果有)
		char command[10];
		char path[100] = ""; 
		sscanf(cmd, "%s %s", command, path);
		if (cmd == NULL) // 指令为空
        {
            continue;
        }
		else if(!strncmp(cmd, "ls",2))//显示服务器当前目录内容
		{	
			if(send_pkg(command,path,sock_fd))
			recv_ls(pkg,sock_fd,path);
		}
		else if (!strncmp(command, "get",3))//从服务器下载文件
		{
			if(send_pkg(command,path,sock_fd))
			recv_file(pkg,sock_fd,path);
		}
		else if (!strncmp(command, "put",3))//上传至服务器
		{
			if(send_pkg(command,path,sock_fd))
			send_file(path,sock_fd);
		}
		else if (!strncmp(command, "cd",2))//切换服务器工作目录
		{
			if(send_pkg(command,path,sock_fd));
			recv_cd(sock_fd,path);
		}
		else if (!strncmp(command, "lcd",3))//切换本地工作目录
		{
			cmd_lcd(path);
		}
		else if (!strncmp(command, "lls",3))//显示本地目录内容
		{
			cmd_lls(path);
		}
		else if (!strncmp(command, "mkdir",5))//显示本地目录内容
		{
			if(send_pkg(command,path,sock_fd));
			recv_mkdir(sock_fd,path);
		}
		else if (!strncmp(command, "help",4))//打开帮助菜单
		{
			cmd_help();
		}
		else if (!strncmp(cmd, "exit",4) || !strncmp(cmd, "quit",4))
		{//退出连接
			if(send_pkg(command,path,sock_fd))
			break;
		}
    }
	close(sock_fd);
}

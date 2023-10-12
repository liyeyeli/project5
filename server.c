#include"../inc/server.h"


//./tcp_server IP PORT
int main(int argc,char *argv[])
{
    struct sockaddr_in Cli,Ser;//定义两个结构体一个客户端一个服务端
    socklen_t Cli_len = sizeof(Cli);
    int confd;
	if(argc != 3)
	{
		printf("Usage:%s <IP> <PORT>\n",argv[0]);
		return -1;
	}
	//1.创建套接字
	int sock_fd = socket(AF_INET,SOCK_STREAM,0);
	if(-1 == sock_fd)
	{
		perror("socket failed!!");
		return -1;
	}
	int on = 1;
	int ret1 = setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));//设置套接字选项，允许重用本地址
	if(ret1 == -1)
	{
		perror("setsockopt SO_REUSEADDR failed!!");
		close(sock_fd);
		return -1;
	}
	ret1 = setsockopt(sock_fd,SOL_SOCKET,SO_REUSEPORT,&on,sizeof(on)); //设置套接字选项，允许重用本端口
	if(ret1 == -1)
	{
		perror("setsockopt SO_REUSEPORT failed!!");
		close(sock_fd);
		return -1;
	}
	//2.绑定服务器的地址 
	Ser.sin_family = AF_INET;//设置服务端的地址使用ipv4协议
	Ser.sin_port = htons(atoi(argv[2]));//"6666"-->6666 端口号，由程序员自定
	Ser.sin_addr.s_addr = inet_addr(argv[1]);//输入ip，由程序员自定

	int ret = bind(sock_fd,(struct sockaddr *)&Ser,sizeof(Ser));//绑定服务器地址
	if(ret == -1)
	{
		perror("bind failed!!");
		close(sock_fd);
		return -1;
	}
	//3.监听套接字
	ret = listen(sock_fd,10);//表示监听，最多同时监听十台设备
	if(ret == -1)
	{
		perror("listen failed!!");
		close(sock_fd);
		return -1;
	}
    printf("服务器创建成功\n");
    while(1)
    {
        // 接受连接
        confd = accept(sock_fd, (struct sockaddr *)&Cli,&Cli_len);//接受请求，用Cli保存客户端的信息，confd为
											//与该客户端的连接套接字描述符.后续与该客户端的数据交换都是通过该连接的套接字描述符
        if (confd == -1)
        {
            if (errno == EWOULDBLOCK)
            {
                // 没有收到连接请求，重新尝试接受连接
                 printf("没有收到连接请求\n");
            }
            else
            {
                fprintf(stderr, "accept函数出错，服务器接受连接失败\n");//stderr为标准IO，表示标准错误，可以将错误信息重定向到标准错误中
																		//fprintf 表示将后面的字符串存入stderr中
                return -1;
            }
        }
        else
        {
            IPinfo* IP = (IPinfo*)malloc(sizeof(IPinfo));//定义一个IP地址及套接字信息结构体（供函数传递使用）
            fprintf(stderr,"连接成功 IP: %s[%d] \n",inet_ntoa(Cli.sin_addr),ntohs(Cli.sin_port));//打印来自客户端的ip地址和端口号
            pthread_t thread_id;//创建一个线程ID
            IP->Cli_sock = confd;
            IP->Ser_sock = sock_fd;
            IP->Cli = Cli;
            IP->Ser = Ser;
            if (pthread_create(&thread_id, NULL, client_handler, (void *)IP) == -1) 
            {
                fprintf(stderr, "pthread_create error\n");
                free(IP);
                continue;
            }
            // Detach thread (optional, if you don't need to join later)
            pthread_detach(thread_id);
        }
    }
	//4.关闭套接字
	close(sock_fd);
}



#include"../inc/cmd.h"
#include"../inc/pkg.h"
#include"../inc/msg.h"
/*===============================执行命令的函数===========================================*/


/*服务器目录下所有文件的名字，各个文件名之间以空格隔开
    @sockfd:目标套接字
    @dir:目标目录
*/
int cmd_ls(int sockfd,char* dir)
{
    DIR *dp;
    struct dirent *entry;
    char result[MAX_LENGTH] = "";

    if ((dp = opendir(dir)) == NULL)
    {
        fprintf(stderr, "显示文件列表失败，无法打开路径：%s\n", dir);
        send_simple(sockfd, FAILURE);
        return -1;
    }
    //chdir(dir);
    while ((entry = readdir(dp)) != NULL)
    {
        // 忽略.和..
        if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
            continue;
        strcat(result, entry->d_name);
        // printf("result += %s\n", entry->d_name);
        strcat(result, "\t");
    }
    struct ftpmsg msg;
    msg.type = SUCCESS;
    msg.data = pkg_resp((uchar*)result,&msg.len,FTP_CMD_LS);
    send_msg(sockfd, &msg);
    printf("显示文件列表成功\n");
    closedir(dp);
    return 0;
}

/*获取文件 并发送（通用型）
    @path:目标文件路径
*/
int send_file(char* path,int sock_fd)
{
    uchar buf[2048], filename[1000];
    int i = 0, fd, nread;
    long sent = 0;
    struct stat statbuf;
    struct ftpmsg msg;

    // 打开源文件
    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "open函数出错，打开源文件失败\n");
        send_simple(sock_fd,FAILURE);
        return -1;
    }
    // 读取源文件属性
    if (stat(path, &statbuf) == -1)
    {
        fprintf(stderr, "stat函数出错，读取源文件属性失败\n");
        return -1;
    }
    // 发送文件名
    msg.type = FILE_NAME;
    uchar* content = malloc(sizeof(path));
    if (content == NULL)
    {
        fprintf(stderr, "malloc函数出错，发送文件内容失败\n");
        return -1;
    }
    file_name((char*)content, path);//将路径中的文件名剥离出来
    file_name((char*)filename, path);
    // 构建回复数据包（规范号回复内容）
    msg.data = pkg_resp(content,&msg.len,FILE_NAME);
    send_msg(sock_fd, &msg);

    // 发送st_mode
    msg.type = FILE_MODE;
    sprintf((char*)content, "%d", statbuf.st_mode);
    // 构建回复数据包（规范号回复内容）
    msg.data = pkg_resp(content,&msg.len,FILE_MODE);
    send_msg(sock_fd, &msg);
    // printf("发送st_mode成功\n");

    // 发送文件大小
    msg.type = FILE_SIZE;
    sprintf((char*)content, "%ld", statbuf.st_size);
    // 构建回复数据包（规范号回复内容）
    msg.data = pkg_resp(content,&msg.len,FILE_SIZE);
    send_msg(sock_fd, &msg);
    // printf("发送文件大小成功\n");

    // 接收答复，文件是否创建成功
    recv_msg(sock_fd, &msg);
    if (msg.type == FAILURE)
    {
        fprintf(stderr, "open函数出错，创建目标文件失败\n");
        return -1;
    }

    // 发送内容
    printf("正在发送文件  %s：\n", filename);
    while ((nread = read(fd, buf, sizeof(buf))) != 0)
    {
        if (nread == -1)
        {
            fprintf(stderr, "read函数出错，读取源文件失败\n");
            return -1;
        }
        
        msg.type = FILE_DATA;
        msg.len = nread;
        msg.data = buf;
        send_msg(sock_fd, &msg);
        sent += nread;
        for (i = i; i < ((float)sent / statbuf.st_size * 50); i++)
        {
            putchar('#');
            fflush(stdout);//清空输出缓冲区
        }
    }
    close(fd);
    // 发送结束标志
    send_simple(sock_fd, FILE_END);
    // 接收答复，文件是否发送成功
    recv_msg(sock_fd, &msg);
    if (msg.type == SUCCESS)
    {
        printf("\n%s发送成功（已发送%ld/%ld字节）\n", filename, atol((char*)msg.data), statbuf.st_size);
    }
    else
    {
        printf("\n%s发送失败（已发送%ld/%ld字节）\n", filename, atol((char*)msg.data), statbuf.st_size);
    }
    return 0;
}

//接收文件
int recv_file(pkgInfo_resp pkg,int sock_fd,char* filenames)
{
	struct ftpmsg msg;
	char *filename;
	mode_t mode;
	int i = 0, fd, nwrite;
	long size, recvd = 0;

	// 接收文件名
	recv_msg(sock_fd, &msg);  
    if(msg.type == FAILURE)
    {
        //fprintf(stderr, "test success\n");//test
        fprintf(stderr, "文件打开失败，找不到此文件或其他原因\n");
        return 0;
    }
	pkg = handle_pkg_resp(msg.data,sock_fd,&msg.len);
	filename = (char*)pkg.resp_content;

	// 接收st_mode
	recv_msg(sock_fd, &msg);
	pkg = handle_pkg_resp(msg.data,sock_fd,&msg.len);
	mode = atoi((char*)pkg.resp_content);

	// 接收文件大小
	recv_msg(sock_fd, &msg);
	pkg = handle_pkg_resp(msg.data,sock_fd,&msg.len);
	size = atol((char*)pkg.resp_content);

	// 创建文件，回复是否成功
	fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, mode % 01000);
	if (fd == -1)
	{
		fprintf(stderr, "open函数出错，创建目标文件失败\n");
		send_simple(sock_fd, FAILURE);
		return -1;
	}
	else
	{
		send_simple(sock_fd, SUCCESS);
	}
	// 接收内容
	printf("正在接收文件%s：\n", filename);
	recv_msg(sock_fd, &msg);
	while (msg.type != FILE_END)
	{
		if ((nwrite = write(fd, msg.data, msg.len)) == -1)
		{
			fprintf(stderr, "write函数出错，写入目标文件失败\n");
			printf("\n%s接收失败（已接收%ld/%ld字节）\n", filename, recvd, size);
			msg.type = FAILURE;
			msg.len = sizeof(long);
			sprintf((char*)msg.data, "%ld", recvd);
			send_msg(sock_fd, &msg);
			close(fd);
			return -1;
		}
		else
		{
			recvd += nwrite;
			for (i = i; i < ((double)recvd / size * 50); i++)
			{
				putchar('#');
				fflush(stdout);
			}
			recv_msg(sock_fd, &msg);
		}
	}
	msg.type = SUCCESS;
	msg.data = malloc(sizeof(long));
	sprintf((char*)msg.data, "%ld", recvd);
	msg.len = strlen((char*)msg.data);
	send_msg(sock_fd, &msg);
	close(fd);
	printf("\n%s接收成功（已接收%ld/%ld字节）\n", filename, recvd, size);
	return 0;	
}

// 从文件路径获取文件名
char *file_name(char *filename, const char *path)
{
    if (strchr(path, '/') == NULL)
    {
        strcpy(filename, path);
    }
    else
    {
        strcpy(filename, strrchr(path, '/') + 1);
    }
    return filename;
}

// 将路径转换为绝对路径
char *full_path(char *path)
{
    // printf("相对路径：%s\n", path);
    // 如果输入的是相对路径（首字符不是'/'）则转换
    // 绝对路径（path的新值） = 当前路径 + "/" + 相对路径（path原来的值）;
    if (path[0] != '/')
    {
        char *cwd;
        cwd = getcwd(NULL, 0);
        strcat(cwd, "/");
        strcat(cwd, path);
        strcpy(path, cwd);
    }
    // printf("绝对路径：%s\n", path);
    return path;
}

/*切换服务器工作目录
    @sock_fd:目标套接字
    @dir:目标目录
*/
int cmd_cd(int sock_fd,char* dir)
{
    if (chdir(dir) == -1)
    {
        fprintf(stderr, "切换工作路径失败\n");
        send_simple(sock_fd, FAILURE);
        return -1;
    }
    else
    {
        printf("切换工作路径成功\n");
        send_simple(sock_fd, SUCCESS);
        return 0;
    }
}

/*切换本地工作目录
    @dir:目标目录
*/
int cmd_lcd(char *dir)
{
    if (dir == NULL || !strcmp(dir, ""))
    {
        fprintf(stderr, "错误：需要参数，正确用法为：lcd <本地路径>\n");
        return -1;
    }
    if (chdir(dir) == -1)
    {
        fprintf(stderr, "本地路径%s不存在\n", dir);
        return -1;
    }
    return 0;
}
/*显示本地目录内容
    @dir:目标目录
*/
int cmd_lls(char* dir)
{
    DIR *dp;
    struct dirent *entry;
    // 若参数被省略，则以当前路径作为参数
    if (dir == NULL || !strcmp(dir, ""))
    {
        strcpy(dir, ".");
    }
    if ((dp = opendir(dir)) == NULL)
    {
        fprintf(stderr, "无法打开路径：%s\n", dir);
        return -1;
    }
    printf("本地目录%s下的内容：\n", dir);
    //chdir(dir);
    while ((entry = readdir(dp)) != NULL)
    {
        // 忽略.和..
        if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
            continue;
        printf("%s\t", entry->d_name);
    }
    printf("\n");
    closedir(dp);
    return 0;
}

//服务器创建目录
int cmd_mkdir(int sockfd, char *dir)
{
    if (create_dir(dir) == -1)
    {
        fprintf(stderr, "创建目录%s失败\n", dir);
        send_simple(sockfd, FAILURE);
        return -1;
    }
    else
    {
        printf("创建目录%s成功\n", dir);
        send_simple(sockfd, SUCCESS);
        return 0;
    }
}
//创建目录
int create_dir(char *dir_name)
{
    // 保存当前工作路径
    char pwd[MAX_LENGTH];
    getcwd(pwd, MAX_LENGTH);

    char *path = full_path(dir_name);//转换绝对路径
    //DIR *dp;
    char *dir;
    dir = strtok(path, "/");
    chdir("/");
    while (dir != NULL)
    {
        if ((chdir(dir)) == -1)
        {
            mkdir(dir, 0777);
            // printf("创建目录%s/%s成功\n", getcwd(NULL, 0), dir);
            chdir(dir);
        }
        dir = strtok(NULL, "/");
    }
    // 切换到原工作路径
    chdir(pwd);
    return 0;
}


//帮助菜单
void cmd_help()
{
    printf("\n");
    printf("lcd <本地路径>          切换本地工作路径\n");
    printf("cd <服务器路径>         切换服务器工作路径\n");
    printf("lls [<本地路径>]        显示本地指定路径或当前工作路径下的内容\n");
    printf("ls [<服务器路径>]       显示服务器指定路径当前工作路径下的内容\n");
    printf("mkdir <服务器路径>      在服务器上新建目录\n");
    //printf("rmdir <服务器路径>      在服务器上删除指定的目录（及目录下所有文件）\n");
    printf("put <本地文件路径>      向服务器上传指定文件\n");
    printf("get <服务器文件路径>    从服务器下载指定文件\n");
    printf("exit或quit             退出程序\n");
    printf("help                   显示此帮助信息\n");
    printf("\n");
}


//线程并发用的客户端命令执行函数
void *client_handler(void *arg) 
{
    IPinfo* IP = (IPinfo*)arg;
    //数据包
    pkgInfo_rqst pkg;//定义一个数据包信息结构体(客户端请求命令格式)
    pkgInfo_resp pkg_r;//定义一个数据包信息结构体(服务器回复命令格式)
    int connected = 1;
    while(connected)
    {
        printf(">  ");//test
        //  开始通信//接收数据包
        int len=0;
        pkg.cmd_no = 0;//命令号清零
        uchar* recv_buf = (uchar*)malloc(1024);
        int ret1 = recv(IP->Cli_sock,recv_buf,1023,0);//用recv_buf存放从客户端发来的信息
        if(ret1 > 0)//判断是否接收成功
        {
            printf("recv %s[%d]: \n",inet_ntoa(IP->Cli.sin_addr),ntohs(IP->Cli.sin_port));//打印接收的客户端的ip地址和端口号  
            pkg = handle_pkg_rqst(recv_buf,IP->Cli_sock,&len);        //解析数据包
            //printf("cmd_no=%d\n",pkg.cmd_no);//test
            print_command(recv_buf,len);//for test以16进制输出，判断数据包是否正确
        }     
        switch (pkg.cmd_no)
        {//各种命令操作// 构建回复数据包（规范号回复内容）
        case FTP_CMD_LS:
            cmd_ls(IP->Cli_sock,pkg.arg_1_data);
            break;
        case FTP_CMD_GET:
            send_file(pkg.arg_1_data,IP->Cli_sock);
            break;
        case FTP_CMD_PUT:
            recv_file(pkg_r,IP->Cli_sock,pkg.arg_1_data);
            break;
        case FTP_CMD_CD:
            cmd_cd(IP->Cli_sock,pkg.arg_1_data);
            break;
        case FTP_CMD_MKDIR:
            cmd_mkdir(IP->Cli_sock,pkg.arg_1_data);
            break;    
        case FTP_CMD_BYE:
            fprintf(stderr,"客户端连接已断开，正在等待重新连接\n");
            pkg.cmd_no = 0;
            connected = 0;
            break;
        default:
            break;
        }
    }
    // Clean up
    close(IP->Cli_sock);
    pthread_exit(NULL);
}
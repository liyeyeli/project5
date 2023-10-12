CC:=gcc 
CFLAGS = -O0 -Wall -g -lpthread
SERVER:=FTPserver
#CLIENT:=FTPclient
INCS:=-I../inc
LIBS:=-L../lib
#LIBS+=-lxxx		#如果链接了第三方库,则在此次增加库的名称格式 -l库名 
#INCS+=				#如果其它目录中有需要搜索的头文件,则在此处增加格式 -I头文件路径

CSRCS:=$(wildcard *.c) #选中所有.c
SER_SRC:=$(filter-out client.c, $(CSRCS))#排除特定
CLI_SRC:=$(filter-out server.c, $(CSRCS))#排除特定
#CSRCS+=$(wildcard ./xxx/*.c)	#可以查找指定目录下的文件，返回 带路径的文件名的字符串表列
SER_COBJS:=$(patsubst %.c,%.o,$(SER_SRC))
CLI_COBJS:=$(patsubst %.c,%.o,$(CLI_SRC))

#服务器程序
$(SERVER):$(SER_COBJS)
	$(CC) $^ -o $@ $(CFLAGS) $(LIBS)
	@echo 服务器程序$(SERVER)创建完成
%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCS)
	
#客户端程序
$(CLIENT):$(CLI_COBJS)
	$(CC) $^ -o $@ $(CFLAGS) $(LIBS)
	@echo 客户端程序$(CLIENT)创建完成
%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCS)
	


clean:											#没有依赖的目标 称为 伪目标，只有当           make 伪目标名 时才会被编译（执行）
	rm -f *.o $(SERVER)
	rm -f *.o $(CLIENT)

#this is former version
# CC = gcc
# CFLAGS = -O0 -Wall -g -lpthread

# copy:copy.c thread_pool.c
# 	$(CC) $^ -o $@ $(CFLAGS)

# debug:copy.c thread_pool.c
# 	$(CC) $^ -o $@ $(CFLAGS) -DDEBUG

# clean:
# 	$(RM) .*.sw? copy debug *.o

# .PHONY:all clean


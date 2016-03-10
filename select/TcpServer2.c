#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>


#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

// for select
#include <sys/select.h>

#define TCP_SERVER_PORT     6666    /*  服务器的端口  */
#define BUFFER_SIZE         4096
#define IP_SIZE             20
#define MAX_FILENAME_SIZE   256
#define LISTEN_QUEUE        20

/* 服务器接收从客户端传送来的文件  */
void
TcpServerPullFile(
            int         connfd,                     /*  服务器与客户端通讯的套接字文件  */
            struct      sockaddr_in clientAddr,     /*  与之通信的客户端的信息  */
            char        *fileServerRoot)            /*  上传文件的存储路径  */
{
    char    buffer[BUFFER_SIZE];
    char    filename[MAX_FILENAME_SIZE];
    char    fileServerPath[MAX_FILENAME_SIZE]/* = fileServerRoot*/;
    // 定义文件流
    FILE    *stream;

    int     count;              /*  发送文件名的字节数目  */
    int     dataLength;         /*  接收到的数据大小  */
    int     writeLength;        /* 实际写入的数据大小  */
    int     flag = 0;

    bzero(buffer, BUFFER_SIZE);
    /*
     *  向客户端提示输入文件路径提示...
     *
     * strcpy(buffer, "请输入要传输的文件的完整路径：");
    strcat(buffer, "\n");

    send(new_server_socket, buffer, BUFFER_SIZE, 0);
    bzero(buffer, BUFFER_SIZE);
    */

    /*  首先获取客户端发送过来的文件名  */
    count = recv(connfd, buffer, BUFFER_SIZE, 0);

    if(count < 0)
    {
        perror("获取文件名失败...\n");
        exit(1);
    }
    else
    {

        strncpy(filename, buffer, strlen(buffer) > MAX_FILENAME_SIZE ? MAX_FILENAME_SIZE : strlen(buffer));
        strcpy(fileServerPath, fileServerRoot);
        strcat(fileServerPath, filename);
        printf("\n获取客户端发送过来的文件名成功...\n");
        printf("文件名[%s]\n", filename);
        printf("文件存储路径[%s]\n\n", fileServerPath);
    }

    //  服务器接受数据, 首先打开一个文件流
    if((stream = fopen(fileServerPath, "w")) == NULL)
    {
        perror("file open error...\n");
        exit(1);
    }
    else
    {
        bzero(buffer,BUFFER_SIZE);
    }

    printf("正在接收来自%s的文件....\n",inet_ntoa(clientAddr.sin_addr));

    dataLength = 0;


    /*  先将数据接受到缓冲区buffer中，再写入到新建的文件中  */
    while((dataLength = recv(connfd, buffer, BUFFER_SIZE, 0)) > 0)
    {

        flag++;

        if(flag == 1)
        {
            printf("正在接收来自%s的文件....\n", inet_ntoa(clientAddr.sin_addr));
        }

        if(dataLength < 0)
        {
            printf("接收错误i\n");
            exit(1);
        }

        /*  向文件中写入数据  */
        writeLength = fwrite(buffer, sizeof(char), dataLength, stream);

        if(writeLength != dataLength)
        {
             printf("file write failed\n");
             exit(1);
        }
        bzero(buffer,BUFFER_SIZE);
    }

    if(flag > 0)
    {
        printf("%s的文件传送完毕\n", inet_ntoa(clientAddr.sin_addr));
    }
    if(flag==0)
    {
        printf("%s的文件传输失败\n", inet_ntoa(clientAddr.sin_addr));
    }

    fclose(stream);
    //rename("data",inet_ntoa(clientAddr.sin_addr));
    ///  BUG   这里其实有问题
    ///  因为客户端将文件发送完毕后, 服务器是不知道的,
    ///  因此当客户端文件发送完毕后, 服务器会陷入一个死等的循环
    ///  这时一个问题, 但是不是我们代码的重点,
    ///  因为我们的代码, 只是用于学习套接字网络编程
    ///
    ///  这个BUG其实很好处理, 因此我们在网络传输的过程中
    ///  客户端与服务器通信的数据肯定有我们自己的格式或者规范
    ///  比如 [request/response HEAD + LENGTH + DATA]的格式
    ///  要不然连基本的UDP丢包 和 TCP粘包问题都解决不了
    ///  一般情况下, 我们与客户

}



/*  服务器将文件发送到客户端
 *
 *  当用户选择了下载文件后，服务器将执行此操作
 *
 *  */
void TcpServerPushFile(
                    int         connfd,                  /*  服务器与客户端通讯的套接字文件  */
                    struct      sockaddr_in clientAddr,  /*  与之通信的客户端的信息  */
                    char        *filePath)              /*  带发送至客户端的文件路径  */
{
    //send file imformation

    char    buff[BUFFER_SIZE];
    char    filename[MAX_FILENAME_SIZE];
    int     count;
    FILE    *stream;


    /* 先将文件名发送给客户端
     * 2015-4-13 21:38 Modify
     * 发送文件名时只需要发送filePath最后的文件名filename就可以了
     * */
    bzero(buff, BUFFER_SIZE);
    strcpy(filename, strrchr(filePath, '/') + 1);
    strncpy(buff, filename, strlen(filename) > MAX_FILENAME_SIZE ? MAX_FILENAME_SIZE : strlen(filename));
    count = send(connfd, buff, BUFFER_SIZE, 0);
    printf("服务器待发送的文件名[%s]..\n", filename);

    if(count < 0)
    {
        perror("Send file information");
        exit(1);
    }

    /*  服务器开始读取并且发送文件 ： */
    if((stream = fopen(filePath, "rb")) == NULL)
    {
        printf("%s not found!\n", filePath);
    }
    printf("服务器打开文件成功...\n");
    printf("正在向客户端发送文件...\n");
    bzero(buff, BUFFER_SIZE);

    int fileBlockLength = 0;
    while((fileBlockLength = fread(buff, sizeof(char), BUFFER_SIZE, stream)) > 0)
    {
        printf("读取了:%d个数据...\n",fileBlockLength);
        if((count =send(connfd, buff, fileBlockLength, 0)) < 0)
        {
            perror("Send file error...\n");
            perror("向客户端发送文件失败...\n");
            exit(1);
        }

        bzero(buff,BUFFER_SIZE);
    }

    fclose(stream);
    printf("服务器发送文件成功\n");

}

/// 处理客户端的请求信息
void RaiseClientRequest(
        int connFd,     /*  客户端的连接套接字描述符, 用于发送和接收数据  */
        struct sockaddr_in  clientAddr) /*  客户端的信息, 用于显示一些客户端的信息  */
{

    printf("\n\n\n下面将依次测试  接收数据  发送数据  存储文件  推送文件\n\n\n");

    int count;
    char buffer[BUFFER_SIZE];

    //  首先测试接收客户端发送来的数据
    printf("===========recv data start===========\n");
    bzero(buffer, BUFFER_SIZE);
    if((count = recv(connFd, buffer, BUFFER_SIZE, 0)) < 0)
    {
        printf("recv data error from %s error, errno = %d...\n", inet_ntoa(clientAddr.sin_addr), errno);
        printf("接收来自 %s 的数据错误, 错误码errno = %d....\n", inet_ntoa(clientAddr.sin_addr), errno);
    }
    else
    {
        printf("recv %d data : %s\n", count, buffer);
        printf("接收%d个数据 : %s\n", count, buffer);
    }
    printf("===========recv data end=============\n\n\n");


    //  接着测试向客户端发送反馈数据
    printf("===========send data start===========\n");
    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, "I am fine !");
    if((count = send(connFd, buffer, strlen(buffer) + 1, 0)) < 0)
    {
        printf("send data[%s] error[errno = %d]...\n", buffer, errno);
        printf("发送数据[%s] 失败[错误码 = %d]...\n", buffer, errno);
    }
    else
    {
        printf("send data[%s] success...\n", buffer);
        printf("发送数据[%s]成功...\n", buffer);
    }
    printf("===========send data end=============\n\n\n");


    //  首先测试接收客户端发送来的数据
    //printf("===========pull file start============\n");
    //TcpServerPullFile(connFd, clientAddr, "./sdata/"); /*  将客户端发送来的文件存储在./sdata目录下　　*/
    //printf("===========pull file end==============\n");

    //  首先测试接收客户端发送来的数据
    //printf("===========pull file============\n");
    //TcpServerPushFile(connFd, clientAddr, "./sdata/spush"); /*  将客户端发送来的文件存储在./sdata目录下　　*/
    //printf("===========pull file============\n");


}


extern int errno;
int main(int argc, char *argv[])
{
    /**********************************************************
     *
     *  创建并初始化服务器套接字
     *
     **********************************************************/
    struct sockaddr_in      serverAddr;
    int                     socketFd;

    bzero(&serverAddr, sizeof(serverAddr));     /*  全部置零  */

    /* 设置地址相关的属性 */
    serverAddr.sin_family         =   AF_INET;
    serverAddr.sin_addr.s_addr    =   htons(INADDR_ANY);
    serverAddr.sin_port           =   htons(TCP_SERVER_PORT);

    /*  创建套接字  */
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd < 0)
    {
        perror("socket create error\n");
        exit(-1);
    }
    else
    {
        printf("socket create success...\n");
        printf("创建套接字成功[errno = %d]...\n", errno);
    }

    /*  绑定端口  */
    /**********************************************************
     *
     *  命名服务器的套接字, 进行BIND端口绑定
     *
     **********************************************************/
    if(bind(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) > 0)
    {
        perror("bind error\n");
        exit(1);
    }
    else
    {
        printf("server bind port %d success...\n", TCP_SERVER_PORT);
        printf("服务器绑定端口%d成功...\n", TCP_SERVER_PORT);
    }

    /*  开始监听绑定的端口  */
    /**********************************************************
     *
     *  开始监听服务器绑定的端口
     *
     **********************************************************/
    //if(listen(socketFd, LISTEN_QUEUE))
    //  系统中每一个端口最大的监听队列的长度
    //  这是个全局的参数,默认值为128
    //  http://blog.csdn.net/taolinke/article/details/6800979
    if(listen(socketFd, SOMAXCONN))
    {
        printf("Server listen error[errno = %d]...\n", errno);
        exit(-1);
    }
    else
    {
        printf("Server listen success...\n");
        printf("服务器开始监听...\n");
    }


    //  values for select
    int             pos, maxpos, maxfd, selectFd;
    int             nready, client[FD_SETSIZE];
    fd_set          rset, allset;
    struct  timeval timeout;

    //  initial "select" elements
    maxfd  = socketFd;                     //新增listenfd，所以更新当前的最大fd
    maxpos = -1;
    for(pos = 0; pos < FD_SETSIZE; pos++)
    {
        client[pos] = -1;
    }

    FD_ZERO(&allset);
    FD_SET(socketFd, &allset);


    struct sockaddr_in  clientAddr;
    socklen_t           length = sizeof(clientAddr);
    int                 connFd;

    while( 1 )
    {
        rset = allset;   //rset和allset的搭配使得新加入的fd要等到下次select才会被监听
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;


        if((nready = select(maxfd + 1, &rset, (fd_set *)NULL, (fd_set *)NULL, &timeout)) < 0) //一开始select监听的是监听口
        {
            //  如果有timeout设置，那么每次select之前都要再重新设置一下timeout的值
            //  因为select会修改timeout的值。
            perror("select error...\n");
            exit(-1);
        }
        else if(nready == 0)
        {
            printf(".");
            fflush(stdout);
            continue;
        }

        //  如果有timeout设置，那么每次select之前都要再重新设置一下timeout的值
        //  因为select会修改timeout的值。
        if(FD_ISSET(socketFd, &rset))
        {


            if((connFd = accept(socketFd, (struct sockaddr*)&clientAddr, &length)) == -1)
            {
                perror("accept error...\n");
                continue;
            }
            else
            {
                printf("获取到从客户端%s : 端口%d的连接, 套接字描述符%d...\n",
                        inet_ntoa(clientAddr.sin_addr),
                        ntohs(clientAddr.sin_port),
                        connFd);
            }

            // 新加入的描述符，还没判断是否可以或者写
            // 所以后面使用rset而不是allset
            FD_SET(connFd, &allset);
            printf("line = %d\n", __LINE__);
            if (connFd > maxfd)  //  maxfd是为了下次select，作为参数使用
            {
                maxfd = connFd;
            }

            printf("line = %d\n", __LINE__);
            //FD_CLR(socketFd, &rset);

            printf("line = %d, nready = %d\n", __LINE__, nready);
            if(--nready <= 0)  //  nready用来辅助计数，这样就不要遍历整个client数组
            {
                continue;
            }
            printf("line = %d\n", __LINE__);

            //  client[]维护着当前连接着的所有客户端描述符
            for(pos = 0; pos < FD_SETSIZE; pos++)
            {
                if (client[pos] < 0)
                {
                    client[pos] = connFd;    //找一个最小的插进入，并且缓存在client中，这样就不需要遍历所有fd，包括为0位的，来查看是否ISSET
                    break;
                }
            }

            if(pos == FD_SETSIZE)
            {
                printf("too many clients\n");
                exit(0);
            }

            if(pos > maxpos)        //  更新maxPos
            {
                maxpos = pos;
            }
        }

        //  至此, client[0~maxPos]中存储了当前所有的客户端套接字描述符
        printf("maxpos = %d\n", maxpos);
        for(pos = 0; pos <= maxpos; pos++)
        {
            if( (selectFd = client[pos]) < 0)
            {
                continue;
            }
            printf("当前活动的连接pos = %d,fd = %d\n", pos, selectFd);

            // 此时socketFd存储了当前活动的客户端套接字
            if(FD_ISSET(selectFd, &rset))
            {
                //单进程的环境下，不可以阻塞在这里，可以选择非阻塞，线程，超时.也就无法防范拒绝服务的攻击
                ////比较适合短连接的情况

                //单进程不使用fork的情况！
                //test fork
                //          if((childpid=fork())==0)
                //          {
                //              close(socketFd);
                ////////////////////////////////////////////////////////////////////////
                //
                //  这里填写服务器的处理代码
                //
                ////////////////////////////////////////////////////////////////////////
                printf("开始与客户端通信\n");
                RaiseClientRequest(connFd, clientAddr);

                close(connFd);

            }
            //             else if (childpid<0)
            //             printf("fork error: %s\n",strerror(errno));
            //close(connfd);
            //test fork

            FD_CLR(selectFd, &allset);                  //清除，表示已被处理
            client[pos] = -1;

            printf("can read : %d, %d, %d\n", pos, selectFd, nready);
            if(--nready <= 0)      //nready用来辅助计数，这样就不要遍历整个client数组
            {
                break;
            }
        }
    }


    return EXIT_SUCCESS;
}




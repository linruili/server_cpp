//server.cpp
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <dirent.h>
#include <sys/stat.h>

#define SERVPORT 8080   // 服务器监听端口号, 原来监听过3333端口
#define BACKLOG 10  // 最大同时连接请求数
#define MAXDATASIZE 100

using namespace std;
void remove_dir(char *path);
int buffToInteger(char* buffer);

int main()
{
    char recv_dir[256] = "../receive";
    char img_name[256];
    char compass_filename[256];
    remove_dir(recv_dir);
    mkdir(recv_dir, 0777);

    int imgSize = 640*480;
//    if (!img.isContinuous())
//        img = img.clone();
    uchar sockData[imgSize];
    char buf[MAXDATASIZE];
    double compass;
    int recvbytes;
    int doubleSize;

    int sock_fd,client_fd;  // sock_fd：监听socket；client_fd：数据传输socket
    int sin_size;
    struct sockaddr_in my_addr; // 本机地址信息
    struct sockaddr_in remote_addr; // 客户端地址信息

    long flag = 1;

    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket创建出错！");
        exit(1);
    }

    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));

    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(SERVPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero),8);
    if(bind(sock_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind出错！");
        exit(1);
    }
    if(listen(sock_fd, BACKLOG) == -1) {
        perror("listen出错！");
        exit(1);
    }


    while(1)
    {
        sin_size = sizeof(struct sockaddr_in);
        if((client_fd = accept(sock_fd, (struct sockaddr *)&remote_addr, (socklen_t*)&sin_size)) == -1)
        {
            perror("accept出错");
            continue;
        }
        printf("received a connection from %s:%u\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));

        if(!fork())
        {
            string s = "2.333#3.44\r";
            if(send(client_fd, (const void *)s.c_str(), s.length(), 0) == -1)
                perror("send出错！");
            exit(0);
        }


        sprintf(compass_filename, "%s/compass.txt", recv_dir);
        remove(compass_filename);
        FILE *compass_file = fopen(compass_filename, "a");

        int frame_count = 1;
        while(frame_count > 0)
        {
            sprintf(img_name, "%s/%03d.jpg", recv_dir, frame_count);
            ++frame_count;
            char rec_int[4];
            if((recvbytes=recv(client_fd, rec_int, sizeof(int), 0)) == -1)
            {
                perror("recv出错！");
                return 0;
            }
            imgSize = buffToInteger(rec_int);
            cout<<"imgSize: "<<imgSize<<endl;
            if(imgSize==0)//结束的标志
            {
                frame_count = 0;
                break;
            }

            int total_rec_bytes = 0;
            for(int i=0; i<imgSize; i+=recvbytes)
            {
                recvbytes = recv(client_fd, sockData + i, imgSize - i, 0);
                cout<<"receive bytes:"<<recvbytes<<endl;
                total_rec_bytes += recvbytes;
                if(recvbytes == -1)
                {
                    perror("recv出错！");
                    return 0;
                }
            }
            sockData[total_rec_bytes] = '\0';
            cout<<"receive total bytes:"<<total_rec_bytes<<endl;

            vector<uchar> img_data(sockData, sockData+total_rec_bytes);
            cv::Mat img = cv::imdecode(img_data, CV_LOAD_IMAGE_COLOR);
            cv::imwrite(img_name, img);
            cout<<"saved image "<<frame_count-1<<"-----------------"<<endl;

            //receive compass
            if((recvbytes=recv(client_fd, rec_int, sizeof(int), 0)) == -1)
            {
                perror("recv出错！");
                return 0;
            }
            doubleSize = buffToInteger(rec_int);
            cout<<"doubleSize: "<<doubleSize<<endl;

            if((recvbytes=recv(client_fd, buf, doubleSize, 0)) == -1)
            {
                perror("recv出错！");
                return 0;
            }
            buf[recvbytes] = '\0';
            double compass = atof(buf);
            cout<<"compass: "<<compass<<endl;
            fprintf(compass_file, "%f\n", compass);
        }

        fclose(compass_file);
        close(client_fd);
    }
    return 0;
}

void remove_dir(char *path) {

    struct dirent *entry = NULL;
    DIR *dir = NULL;
    dir = opendir(path);
    if (dir) {
        printf("Remove %s\n", path);
        while (entry = readdir(dir)) {
            DIR *sub_dir = NULL;
            FILE *file = NULL;
            char abs_path[1024] = {0};
            if (*(entry->d_name) != '.') {
                sprintf(abs_path, "%s/%s", path, entry->d_name);
                if (sub_dir = opendir(abs_path)) {
                    closedir(sub_dir);
                    remove_dir(abs_path);
                } else {
                    if (file = fopen(abs_path, "r")) {
                        fclose(file);
                        remove(abs_path);
                    }
                }
            }
        }
        remove(path);
    }
}

int buffToInteger(char* buffer)
{
    int a = (int)(buffer[0]<<24 | (buffer[1]&0xFF)<<16 | (buffer[2]&0xFF)<<8 | (buffer[3]&0xFF));
    return a;
}

//记录接受各类数据结构的方法
void backup()
{
    int client_fd;
    int recvbytes;
    char buf[MAXDATASIZE];
    int imgSize;
    //int imgSize = img.total()*img.elemSize();
    uchar sockData[imgSize];

    //发送
    if(!fork())
    {
        if(send(client_fd, "Hello, you are connected!\r", 26, 0) == -1)
            perror("send出错！");
        exit(0);
    }


    //接收double
    if((recvbytes=recv(client_fd, buf, MAXDATASIZE, 0)) == -1) {
        perror("recv出错！");
        return ;
    }
    buf[recvbytes] = '\0';
    double compass = atof(buf);
    cout<<compass<<endl;


    //接收int
    char rec_int[4];
    if((recvbytes=recv(client_fd, rec_int, sizeof(int), 0)) == -1)
    {
        perror("recv出错！");
        return ;
    }
    int count = buffToInteger(rec_int);
    cout<<count<<endl;


    //接收图片
    for(int i=0; i<imgSize; i+=recvbytes) {
        if((recvbytes = recv(client_fd, sockData + i, imgSize - i, 0) == -1))
        {
            perror("recv出错！");
            return ;
        }
    }
    cv::Mat img(64, 48, CV_8UC4);//注意这里宽和高的顺序和client相反
    //cv::imdecode()
    //可能byte和uchar有正负的问题
    memcpy(img.data, sockData, imgSize*sizeof(uchar));
    cv::imwrite("img.jpg", img);


    //直接读取无压缩bitmap
    for(int i=0; i<imgSize; i+=recvbytes) {
        if((recvbytes = recv(client_fd, sockData + i, imgSize - i, 0) == -1))
        {

            perror("recv出错！");
            return ;
        }
    }
    cv::Mat img2(64, 48, CV_8UC4);
    memcpy(img2.data, sockData, imgSize*sizeof(uchar));
    cv::imwrite("img2.jpg", img2);


}
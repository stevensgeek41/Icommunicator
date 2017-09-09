
/**************************************************/
/*名称：数据结构定义
/*描述：初始化服务器监听套接字
/*作成日期：2017-9-3
/*参数：  VOID
/*返回值：
/*作者：刘珺
/***************************************************/

#ifndef ICC_PROJECT_MAIN_H
#define ICC_PROJECT_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <time.h>

#define PORT 1234
#define FILE_PORT 1235
#define BUFF_SIZE 1024
#define MAX_ONLINE 300
#define PASSWORD_MAX_LEN 15
#define NICKNAME_MAX_LEN 20
#define MESSAGE_MAX_LEN 100
#define IP_ADDRESS 16
#define ID_LEN 7
#define MAX_MESSAGE 300
#define MAX_FRIENDS 10
#define IPATHX 20
#define IPATHY 30
#define YU 100




////////////////////////////////MYSQL
MYSQL mysql;                                //用于链接数据库到mysql的变量
MYSQL_RES *res;                           //指向res的指针变量,结果集存放在res数组中
MYSQL_ROW row;                          //定义row变量,用于存放列数组
char *query;                                  //sql查询语句
int queryFlag = 0;            //实为表示真假的bool变量
int portnum = 0,  bodynum= 0;                               //portrait编号及body_image编号
char sqlCommand[256];                //用于存储格式化后的字符串
char password[10] = "shw";           //登陆时服务器返回的密码
char password1[10] = "shw";         //注册时服务器返回的密码
char ip[45] = "192.168.0.0.1";       //服务器返回的ip
////////////////////////////////MYSQL


////////////////////////////////data structure
struct ClientInfo
{
    int id;
    char nickname[NICKNAME_MAX_LEN];
    char ipAddress[IP_ADDRESS];
    int portnum;
    int bodynum;
    int onlinestatus;
};

struct ClientsInfo
{
    int totalNum;
    struct ClientInfo clientInfoArray[MAX_FRIENDS];
};

struct HeadInfo
{
    /*
     * flag 0: register
     * flag 1: login
     * flag 2: logout
     * flag 3: sendrequire
     * flag 4: send message
     * flag 5: alter info
     * flag 6: add friend
     * flag 7: send file
     */
    int flag;
};

struct RegistInfo
{
    char password[PASSWORD_MAX_LEN];
    char nickname[NICKNAME_MAX_LEN];
};

struct LoginInfo
{
    int id;
    char password[PASSWORD_MAX_LEN];
};

struct LogoutInfo
{
    int id;
};


struct SendRequireInfo
{
    int toid;
};

struct MessageInfo
{
    int senderID;
    int receiveID;
    char message[MAX_MESSAGE];
    int emojLen[20];
    int counts;
    char ipath[IPATHX][IPATHY];
};

struct AlterInfo
{
    int id;
    int alterFlag;
    int nickname[NICKNAME_MAX_LEN];
    int portnum;
    int bodynum;
};

struct AddFriendInfo
{
    int senderID;
    int receiveID;
};

struct TailInfo
{

    int flag;
};





////////////////////////////////data structure

#endif //ICC_PROJECT_MAIN_H
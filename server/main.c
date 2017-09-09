#include "main.h"


int sockfd;

struct HeadInfo headInfo;
struct RegistInfo registInfo;
struct LoginInfo loginInfo;
struct ClientsInfo clientsInfo;
struct SendRequireInfo sRI;
struct AlterInfo alterInfo;
struct LogoutInfo logoutInfo;
struct MessageInfo messageInfo;
struct AddFriendInfo addFriendInfo;

////////////////////////////////function declare
int setSocket(void);
int isOnLine(int id);
int regist(char *password, char *nickname);
int isLogMatch(int id, char password[PASSWORD_MAX_LEN]);
int sendIPAndOnlineStatus(int id, char *ip);
int logout(int id);
void findIpById(int id, char *ip);
void sendBackClientsInfo(int id);
void changeClientPersonInfo(struct AlterInfo *alterInfo, int flag);


int registerServer(struct sockaddr_in *client, socklen_t *clnt_addr_size);
int loginServer(struct sockaddr_in *client, socklen_t *clnt_addr_size);

int sendMessageServer(struct sockaddr_in *client, struct sockaddr_in *toClient, socklen_t *clnt_addr_size);
int alterInfoServer(struct sockaddr_in *client, socklen_t *clnt_addr_size);
int addFriendServer(struct sockaddr_in *client, socklen_t *clnt_addr_size);
int sendfile();



struct sockaddr_in clients[YU];

int main()
{
    sockfd = setSocket();

    struct sockaddr_in client, toClient;
    int clnt_addr_size = sizeof(client);


    //转发信息
    toClient.sin_family = AF_INET;
    toClient.sin_port = htons(PORT);


    while(1)
    {
        ssize_t infoIen = recvfrom(sockfd, &headInfo, sizeof(headInfo), 0, (struct sockaddr *)&client, &clnt_addr_size);
        printf("headinfo from %s and head_flag is %d\n",inet_ntoa(client.sin_addr), headInfo.flag);

        //info type
        switch (headInfo.flag) {
            //register
            case 0:
            {
                registerServer(&client, &clnt_addr_size);
                break;
            }
            //login
            case 1:
            {
                loginServer(&client, &clnt_addr_size);

                break;
            }
            //logout
            case 2:
            {
                recvfrom(sockfd, &logoutInfo, sizeof(logoutInfo), 0, (struct sockaddr *)&client, &clnt_addr_size);
                //访问数据库，下线
                logout(logoutInfo.id);
                break;
            }
            //sendMessage
            case 3:
            {
//                recvfrom(sockfd, &sRI, sizeof(sRI), 0, (struct sockaddr *)&client, &clnt_addr_size);
//
//                //判断该id是否在线
//                int res = isOnLine(sRI.toid);
//                sendto(sockfd, &res, sizeof(res), 0, (struct sockaddr *)&client, clnt_addr_size);
//                //在线,继续发送信息
//                if(res){
                    sendMessageServer(&client, &toClient, &clnt_addr_size);
//                }
                break;
            }

            //alter client information
            case 5:
            {
                alterInfoServer(&client, &clnt_addr_size);
                break;
            }
            //添加好友
            case 6:
            {
                addFriendServer(&client, &clnt_addr_size);
                break;
            }
            //发送文件
            case 7:
            {
                int receiveID;
                char receiveIP[IP_ADDRESS];
                recvfrom(sockfd, &receiveID, sizeof(receiveID), 0, (struct sockaddr *)&client, &clnt_addr_size);
                findIpById(receiveID, receiveIP);
                sendto(sockfd, receiveIP, sizeof(receiveIP), 0, (struct sockaddr *)&client, clnt_addr_size);
            }

        }
        memset(&headInfo, -1, sizeof(headInfo));

    }

    return 0;
}

/**************************************************/
/*名称：setSocket
/*描述：初始化服务器监听套接字
/*作成日期：2017-8-31
/*参数：  VOID
/*返回值：serv_sock fd
/*作者：刘珺
/***************************************************/
int setSocket(void)
{
    mysql_init(&mysql);                      //初始化mysql变量

    int serv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv_addr;

    int opt = SO_REUSEADDR;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);  //端口
    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("serv_sock bind error");
        return -1;
    }

    return serv_sock;
}

/**************************************************/
/*名称：registerServer
/*描述：headinfo-flag-regist
/*作成日期：2017-9-4
/*参数：  client sizeof(client)
/*返回值： 1
/*作者：刘珺
/***************************************************/
int registerServer(struct sockaddr_in *client, socklen_t *clnt_addr_size)
{
    recvfrom(sockfd, &registInfo, sizeof(registInfo), 0, (struct sockaddr *)client, clnt_addr_size);
    printf("已接收注册信息.......\n");

    int clientID = regist(registInfo.password, registInfo.nickname);

    sendto(sockfd, &clientID, sizeof(clientID), 0, (struct sockaddr *)client, *clnt_addr_size);
    printf("已返回注册者的ID.....: %d \n",clientID);
    return 1;
}

/**************************************************/
/*名称：loginServer
/*描述：headinfo-flag-login
/*作成日期：2017-9-4
/*参数：  client sizeof(client)
/*返回值： 1
/*作者：刘珺
/***************************************************/
int loginServer(struct sockaddr_in *client, socklen_t *clnt_addr_size)
{

    recvfrom(sockfd, &loginInfo, sizeof(loginInfo), 0, (struct sockaddr *)client, clnt_addr_size);
    printf("已接收登录信息.......\n");



    int islog = isLogMatch(loginInfo.id, loginInfo.password);
    if(islog){

        printf("信息匹配，验证在线状态.....\n");
        if(isOnLine(loginInfo.id)){
            printf("已在线！\n");
            islog = 2;          //已在线， 不允许
        }
    }

    //匹配返回1 不匹配返回0
    sendto(sockfd, &islog, sizeof(islog), 0, (struct sockaddr *)client, *clnt_addr_size);
    printf("已返回log状态....%d \n",islog);

    if (islog == 1){
        //上线，记录ip
        memcpy(&clients[loginInfo.id%YU], client, sizeof(clients[loginInfo.id%YU]));


        sendIPAndOnlineStatus(loginInfo.id, inet_ntoa(client->sin_addr));
        printf("已更新在线状态.....\n");
        //返回client自己和friends的信息
        printf("努力连接数据库获取联系人信息......\n");
        sendBackClientsInfo(loginInfo.id);


//        int x = 2;
//        sendto(sockfd, &x, sizeof(x), 0, (struct sockaddr *)client, *clnt_addr_size);
        printf("获取总人数....%d\n",clientsInfo.totalNum);

//        int totalNum = clientsInfo.totalNum;
//        sendto(sockfd, &totalNum, sizeof(totalNum), 0, (struct sockaddr *)client, *clnt_addr_size);
//
//        char buff[100] = {0};
//
//
//        int i=0;
//        for(;i<totalNum;i++){
//
//            memcpy(buff, &clientsInfo.clientInfoArray[i],sizeof(clientsInfo.clientInfoArray[i])+1);
//
//            int size = sendto(sockfd, buff, sizeof(buff) , 0, (struct sockaddr *)client, *clnt_addr_size);
//            printf("发送信息大小: %d\n",size);
//        }


        int size = sendto(sockfd, &clientsInfo, sizeof(clientsInfo), 0, (struct sockaddr *)client, *clnt_addr_size);
        printf("%d 返回联系人信息.....\n", size);


        //固定大小
        //free(clientsInfo.clientInfoArray);
    }


}


/**************************************************/
/*名称：sendMessageServer
/*描述：headinfo-flag-sendMessage
/*作成日期：2017-9-4
/*参数：  client sizeof(client)
/*返回值： 1
/*作者：刘珺
/***************************************************/
int sendMessageServer(struct sockaddr_in *client, struct sockaddr_in *toClient, socklen_t *clnt_addr_size)
{
    recvfrom(sockfd, &messageInfo, sizeof(messageInfo), 0, (struct sockaddr *)client, clnt_addr_size);
    printf("接收信息box.....\n");
    int toid =  messageInfo.receiveID;
    char toip[IP_ADDRESS];
    findIpById(toid, toip);
    printf("生成目的ip..... %s\n", toip);


//    toClient->sin_addr.s_addr = inet_addr(toip);

    client->sin_addr.s_addr = inet_addr(toip);



    printf("服务器转发的信息: %s\n",messageInfo.message);

    //clients[messageInfo.receiveID%10]
    int res =sendto(sockfd, &messageInfo, sizeof(messageInfo), 0, (struct sockaddr *)&clients[messageInfo.receiveID%YU], *clnt_addr_size);

//    int res =sendto(sockfd, &messageInfo, sizeof(messageInfo), 0, (struct sockaddr *)client, *clnt_addr_size);
    printf("nsize: %d\n", res);
    if(res == -1)
        printf("信息转发失败\n");
    printf("信息box已转发.....\n");
    //重置message box
    memset(&messageInfo, 0, sizeof(messageInfo));
}


int alterInfoServer(struct sockaddr_in *client, socklen_t *clnt_addr_size)
{
    recvfrom(sockfd, &alterInfo, sizeof(alterInfo), 0, (struct sockaddr *)&client, clnt_addr_size);
    printf("收到个人信息修改请求.....\n");
    changeClientPersonInfo(&alterInfo, alterInfo.alterFlag);
    printf("新信息写入数据库.....\n");
}


/**************************************************/
/*名称：addFriendServer
/*描述：headinfo-flag-addfriend
/*作成日期：2017-9-4
/*参数：  client sizeof(client)
/*返回值： 1
/*作者：刘珺
/***************************************************/
int addFriendServer(struct sockaddr_in *client, socklen_t *clnt_addr_size)
{
    recvfrom(sockfd, &addFriendInfo, sizeof(addFriendInfo), 0, (struct sockaddr *)client, clnt_addr_size);
    int sender = addFriendInfo.senderID;
    int receiver = addFriendInfo.receiveID;

    //数据库添加好友
    query = "insert into personalfriends(username,friendsname)  values(%d,%d) ";
    sprintf(sqlCommand, query,sender,receiver);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("(addFriendServer)add friends succeed! step1");
    }                                                                                             //加好友
    query = "insert into personalfriends(username,friendsname)  values(%d,%d) ";
    sprintf(sqlCommand, query,receiver,sender);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("(addFriendServer)add friends succeed! step2");
    }
}


/**************************************************/
/*名称：sendfile
/*描述：headinfo-flag-addfriend
/*作成日期：2017-9-4
/*参数：  client sizeof(client)
/*返回值： 1
/*作者：刘珺
/***************************************************/
int sendfile()
{

}


/**************************************************/
/*名称：isOnLine
/*描述：判断该用户是否在线
/*作成日期：2017-8-31
/*参数：  id
/*返回值：在线返回1 不在线返回0
/*作者： 沈翰文
/***************************************************/
int isOnLine(int id)
{
    if (!mysql_real_connect(&mysql,"localhost", "root", "19970507", "iccdataset1",0,NULL,0)){
        printf( "(isOnLine)Error connecting to database: %s\n",mysql_error(&mysql));
    }else{
        printf("(isOnLine)Connection successful!\n");
    }
    query = "select onlinestatus from personalinfo where username = %d";
    sprintf(sqlCommand, query,id);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("[%s] made!\n", query);
    }
    res = mysql_store_result(&mysql);
    row = mysql_fetch_row(res);
    int flag5 = atoi(row[0]);
    mysql_close(&mysql);
    mysql_free_result(res);
    sleep(1);
    if(flag5 == 1)
    {
        return 1;
    }else
    {
        return 0;
    }
}


/**************************************************/
/*名称：regist
/*描述：数据库为用户随机生成一个id，并将用户输入的password和id存入数据库
/*作成日期：2017-9-1
/*参数：  password
/*返回值： id
/*作者： 沈翰文
/***************************************************/
int regist(char *password, char *nickname)
{
    int id1;
    if (!mysql_real_connect(&mysql,"localhost", "root", "19970507", "iccdataset1",0,NULL,0)){
        printf( "(register)Error connecting to database: %s\n",mysql_error(&mysql));
    }else{
        printf("(register)Connection successful!\n");
    }                                                    //若没有链接成功则返回错误信息,否则输出成功链接

    //判重
    srand((unsigned) time(NULL));
    id1 = rand() % 900000 + 100000;
    query = "select username from personalkeyword where username = %d";
    sprintf(sqlCommand,query,id1);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("[%s] made!\n", query);
    }
    res = mysql_store_result(&mysql);
    while( row = mysql_fetch_row(res)){
        mysql_free_result(res);
        id1 = rand() % 900000 + 100000;
        query = "select username from personalkeyword where username = %d";
        sprintf(sqlCommand, query,id1);
        query = sqlCommand;
        queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
        if (queryFlag){
            printf("Error making query: %s\n", mysql_error(&mysql));
        }else{
            printf("[%s] made!\n", query);
        }
        res = mysql_store_result(&mysql);
    }

    query = "insert into personalkeyword(username, keyword) values(%d, '%s')";
    sprintf(sqlCommand, query,id1,password);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("[%s] made!\n", query);
    }
    query = "insert into personalinfo(username,nickname) values(%d, '%s')";
    sprintf(sqlCommand, query,id1,nickname);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("[%s] made!\n", query);
    }
    mysql_close(&mysql);

    return id1;
}

/**************************************************/
/*名称：isLogMatch
/*描述：判断用户是否登录成功，成功返回1，失败返回0
/*作成日期：2017-9-1
/*参数：  id, password
/*返回值： flag
/*作者： 沈翰文
/***************************************************/
int isLogMatch(int id, char password[PASSWORD_MAX_LEN])
{
    int flag = 0;
    if (!mysql_real_connect(&mysql,"localhost", "root", "19970507", "iccdataset1",0,NULL,0)){
        printf( "(logMatch)Error connecting to database: %s\n",mysql_error(&mysql));
    }else{
        printf("(logMatch)Connection successful!\n");
    }
    query = "select * from personalkeyword where username = %d and keyword = '%s'";
    sprintf(sqlCommand, query,id,password);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("[%s] made!\n", query);
    }
    res = mysql_store_result(&mysql);
    if(row = mysql_fetch_row(res)){     //查询是否成功
        flag = 1;
    }else{
        flag = 0;
    }
    mysql_free_result(res);   //释放结果集
    mysql_close(&mysql);

    return flag;
}


/**************************************************/
/*名称：sendIPAndOnlineStatus
/*描述：更新在线状态，记录ip地址
/*作成日期：2017-9-1
/*参数：  id, password
/*返回值： 1 & 0
/*作者： 沈翰文
/***************************************************/
int sendIPAndOnlineStatus(int id, char *ip)
{
    if (!mysql_real_connect(&mysql,"localhost", "root", "19970507", "iccdataset1",0,NULL,0)){
        printf( "(sendIPAndOnlineStatus)Error connecting to database: %s\n",mysql_error(&mysql));
    }else{
        printf("(sendIPAndOnlineStatus)Connection successful!\n");
    }
    query = "update personalinfo set onlinestatus = 1, ipaddress = '%s' where username = %d";
    sprintf(sqlCommand, query,ip,id);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("[%s] made!\n", query);
    }
    mysql_close(&mysql);
}

/**************************************************/
/*名称：logout
/*描述：更新在线状态
/*作成日期：2017-9-1
/*参数：  id
/*返回值： 1 & 0
/*作者： 沈翰文
/***************************************************/
int logout(int id)
{
    if (!mysql_real_connect(&mysql,"localhost", "root", "19970507", "iccdataset1",0,NULL,0)){
        printf( "(logout)Error connecting to database: %s\n",mysql_error(&mysql));
    }else{
        printf("(logout)Connection successful!\n");
    }

    printf("登出成功!\n");
    query = "update personalinfo set onlinestatus = 0 where username = %d";
    sprintf(sqlCommand, query,id);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("[%s] made!\n", query);
    }
    mysql_close(&mysql);
    //更新onlinestatus
}

/**************************************************/
/*名称：findIpById
/*描述：通过id找ip
/*作成日期：2017-9-1
/*参数：  id
/*返回值： 1 & 0
/*作者： 沈翰文
/***************************************************/
void findIpById(int id, char *ip)
{
    if (!mysql_real_connect(&mysql,"localhost", "root", "19970507", "iccdataset1",0,NULL,0)){
        printf( "(findIpById)Error connecting to database: %s\n",mysql_error(&mysql));
    }else{
        printf("(findIpById)Connection successful!\n");
    }
    query = "select ipaddress from personalinfo where username = %d";
    sprintf(sqlCommand, query,id);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
    if (queryFlag){
        printf("Error making query: %s\n", mysql_error(&mysql));
    }else{
        printf("[%s] made!\n", query);
    }
    res = mysql_store_result(&mysql);
    row = mysql_fetch_row(res);

    strcpy(ip,row[0]);

    mysql_free_result(res);

    mysql_close(&mysql);


}

/**************************************************/
/*名称：sendBackClientsInfo
/*描述：将个人信息&好友信息返回
/*作成日期：2017-9-1
/*参数：  id
/*返回值： 1 & 0
/*作者： 沈翰文 & 刘珺
/***************************************************/
void sendBackClientsInfo(int id) {
    char nicknames[NICKNAME_MAX_LEN];
    int onlinestatus;

    if (!mysql_real_connect(&mysql, "localhost", "root", "19970507", "iccdataset1", 0, NULL, 0)) {
        printf("(sendBackClientsInfo)Error connecting to database: %s\n", mysql_error(&mysql));
    } else {
        printf("(sendBackClientsInfo)Connection successful!\n");
    }
    printf("返回个人信息数据库连接成功!\n");

    query = "select pi.username fname, pi.portrait fport, pi.body_image fbody_image, pi.ipaddress fip, pi.nickname fnname, pi.onlinestatus fol from iccdataset1.personalinfo pi, iccdataset1.personalfriends pf where pf.friendsname = pi.username and pf.username = %d";
    sprintf(sqlCommand, query, id);
    query = sqlCommand;
    printf("%d\n", (int) strlen(query));
    queryFlag = mysql_real_query(&mysql, query, (unsigned int) strlen(query));
    if (queryFlag) {
        printf("Error making query: %s\n", mysql_error(&mysql));
    } else {
        printf("query success!\n");
    }
    res = mysql_store_result(&mysql);
    clientsInfo.totalNum = mysql_num_rows(res);

    clientsInfo.totalNum++;

    mysql_free_result(res);
//    printf("增长联系人...: %d\n", clientsInfo.totalNum);
//    clientsInfo.clientInfoArray = (struct ClientInfo *)malloc(sizeof(struct ClientInfo) * clientsInfo.totalNum);

    query = "select portrait,body_image,username,nickname,ipaddress,onlinestatus from personalinfo where username = %d";
    sprintf(sqlCommand, query, id);
    query = sqlCommand;
    queryFlag = mysql_real_query(&mysql, query, (unsigned int) strlen(query));
    if (queryFlag) {
        printf("Error making query: %s\n", mysql_error(&mysql));
    } else {
        printf("query success!\n");
    }
    res = mysql_store_result(&mysql);
    row = mysql_fetch_row(res);


    portnum = atoi(row[0]);
    bodynum = atoi(row[1]);
    int idTemp = atoi(row[2]);
    strcpy(nicknames, row[3]);

    strcpy(ip, row[4]);

    onlinestatus = atoi(row[5]);

    mysql_free_result(res);


    clientsInfo.clientInfoArray[0].bodynum = bodynum;
    clientsInfo.clientInfoArray[0].portnum = portnum;
    stpcpy(clientsInfo.clientInfoArray[0].nickname, nicknames);
    clientsInfo.clientInfoArray[0].id = idTemp;
    stpcpy(clientsInfo.clientInfoArray[0].ipAddress, ip);
    clientsInfo.clientInfoArray[0].onlinestatus = onlinestatus;

    printf("个人信息:\n");
    printf("ownportrait = %d body_image = %d id = %d nickname = %s ip = %s onlinestatus = %d\n",
           clientsInfo.clientInfoArray[0].portnum, clientsInfo.clientInfoArray[0].bodynum,
           clientsInfo.clientInfoArray[0].id,
           clientsInfo.clientInfoArray[0].nickname, clientsInfo.clientInfoArray[0].ipAddress,
           clientsInfo.clientInfoArray[0].onlinestatus);

    query = "select pi.username fname, pi.portrait fport, pi.body_image fbody_image, pi.ipaddress fip, pi.nickname fnname, pi.onlinestatus fol from iccdataset1.personalinfo pi, iccdataset1.personalfriends pf where pf.friendsname = pi.username and pf.username = %d";
    sprintf(sqlCommand, query, id);
    query = sqlCommand;

    printf("strlen(query):%d\n", (int) strlen(query));
    queryFlag = mysql_real_query(&mysql, query, (unsigned int) strlen(query));
    res = mysql_store_result(&mysql);

    if (queryFlag) {
        printf("Error making query: %s\n", mysql_error(&mysql));
    } else {
        printf("query made!\n");
    }
    //res = mysql_store_result(&mysql);
    int t = 1;
    while (row = mysql_fetch_row(res)) {


        idTemp = atoi(row[0]);
        portnum = atoi(row[1]);
        bodynum = atoi(row[2]);
        strcpy(nicknames, row[4]);

        strcpy(ip, row[3]);
        onlinestatus = atoi(row[5]);


        clientsInfo.clientInfoArray[t].id = idTemp;
        clientsInfo.clientInfoArray[t].portnum = portnum;
        clientsInfo.clientInfoArray[t].bodynum = bodynum;


        strcpy(clientsInfo.clientInfoArray[t].ipAddress, ip);

        strcpy(clientsInfo.clientInfoArray[t].nickname, nicknames);

        clientsInfo.clientInfoArray[t].onlinestatus = onlinestatus;
        t++;
    }                                     //存储朋友信息

    int i;
    printf("努力打印好友信息.....\n");
    for (i = 1; i < t; i++) {
        printf("friend%d username = %d,portrait = %d,body_iamge = %d, nickname = %s, ipaddress = %s onlinestatus = %d\n",
               i, clientsInfo.clientInfoArray[i].id, clientsInfo.clientInfoArray[i].portnum,
               clientsInfo.clientInfoArray[i].bodynum, clientsInfo.clientInfoArray[i].nickname,
               clientsInfo.clientInfoArray[i].ipAddress, clientsInfo.clientInfoArray[i].onlinestatus);
    }
    mysql_free_result(res);

    mysql_close(&mysql);
}

void changeClientPersonInfo(struct AlterInfo *alterInfo, int flag)
{
    switch(flag){
        case 1:

            query = "update personalinfo set nickname = '%s' where username = %d";
            sprintf(sqlCommand, query,alterInfo->nickname, alterInfo->id);
            query = sqlCommand;
            queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
            if (queryFlag){
                printf("Error making query: %s\n", mysql_error(&mysql));
            }else{
                printf("[%s] made!\n", query);
            }
            printf("更改昵称成功!\n");                                                                               //昵称
            break;

        case 2:

            query = "update personalinfo set portrait = %d where username = %d";
            sprintf(sqlCommand, query,alterInfo->portnum, alterInfo->id);
            query = sqlCommand;
            queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
            if (queryFlag){
                printf("Error making query: %s\n", mysql_error(&mysql));
            }else{
                printf("[%s] made!\n", query);
            }
            printf("更改头像成功!\n");
            break;                                                                                                             //头像

        case 3:

            query = "update personalinfo set body_image = %d where username = %d";
            sprintf(sqlCommand, query,alterInfo->bodynum,alterInfo->id);
            query = sqlCommand;
            queryFlag = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
            if (queryFlag){
                printf("Error making query: %s\n", mysql_error(&mysql));
            }else{
                printf("[%s] made!\n", query);
            }
            printf("更新个人形象!\n");
            break;                                                                                                          //QQ秀
    }
}




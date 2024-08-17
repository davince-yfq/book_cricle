#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>    
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <sys/epoll.h>
#include <sqlite3.h>
#include <arpa/inet.h>
#include <time.h>

//开启数据库
sqlite3 *db=NULL;
int data=-1;
char *msg;//存放数据库错误信息

typedef struct user
{
    char name[48];
    char password[48];
    char email[124];
}User;

typedef struct book
{
    char puchaser[48];
    char bookname[48];
    char price[48];
    char master[48];
}Book;

typedef struct msg
{
    short type;//1.注册 2.登入 3.插入书籍信息 4.删除书籍 5.搜索书籍 6.修改书籍 7.交易 8.购买请求 9.私聊 10.群聊 11.找回密码 12.查交易数据
    char data[1022];
}Msg;

typedef struct queryNode
{    
     char ** ppResult;//指向字符串数组的指针，每一个元素是一个字符串
     int row;//行
     int col;//列
}QN;

typedef struct rivse
{
    char type[48];
    char bname[48];
    char data[48];
    char master[48];
}Rivse;

typedef struct loginfo
{
    int fd;
    char name[48];
}loginfo;
loginfo carr[521];
int arrcount=0;

typedef struct sgChat
{
    char selfname[48];
    char duiname[48];
    char data[1000];
}sgChat;

typedef struct pChase
{
    char purchaser[48];
    char name[48];
    char price[48];
    char master[48];
    char time[100];
}pChase;

//字符串IP转数字iP
unsigned int StrIptoInt(char *p)
{
    unsigned int ip;
    inet_pton(AF_INET,p,&ip);
    return ip;
}
//数字IP转字符串IP
char *IntIPtoStr(unsigned int p)
{
    char *ip=malloc(16);
    inet_ntop(AF_INET,&p,ip,16);
    return ip;
}
//地址结构体信息录入函数
struct sockaddr_in getaddr(char *szip,uint16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=StrIptoInt(szip);
    addr.sin_port=htons(port);
    return addr;
}
//判断是否出错函数
void juge(char *s,int p)
{
    if(p==-1)
    {
        perror(s);
        return ;
    }
}
QN get_table(sqlite3 *db,char *sql);
void user_insert(int fd,User u)//用户注册函数
{

    char *sql="CREATE TABLE User(Name, password, email);";//插入表头
    sqlite3_exec(db,sql,0,0,&msg);
    if (msg != 0)
    {
        // 如果是因为表格已经存在而出错就不需要退出程序，后面可以继续操作表格
        if (strcmp("table User already exists", msg) != 0)
        {
            printf("执行创建表格出错%s\n", msg);
            return ;
        }
    }
    char buf1[512];
    sprintf(buf1,"select * from User where Name='%s';",u.name);//判断是否又重复用户名
    QN qn=get_table(db,buf1);
    Msg m;
    if(qn.row!=0)
    {
        m.type=666;
        write(fd,&m,sizeof(m));
    }
    else{
        char buf[512];
    sprintf(buf,"INSERT INTO User VALUES('%s','%s','%s');",u.name,u.password,u.email);//将用户信息写入命令
    sqlite3_exec(db,buf,0,0,&msg);
        m.type=777;
        write(fd,&m,sizeof(m));
    }
    
}

QN get_table(sqlite3 *db,char *sql)//查询数据库函数
{
    QN qn;
    char *msg=0;
    int data=sqlite3_get_table(db,sql,&qn.ppResult,&qn.row,&qn.col,&msg);
    if(data<0)
    {
        printf("%s\n",msg);
    }
    return qn;
}

void printfQN(QN qn)//查询数据打印函数
{
    for(size_t i=qn.col;i<qn.row*qn.col+qn.col;i++)
    {
        printf("%s\n",qn.ppResult[i]);
    }
}

int cmp(User u)//登入比较函数
{   
    char buf[512];
    sprintf(buf,"SELECT * FROM User where Name='%s' and password='%s';",u.name,u.password);
    QN qn=get_table(db,buf);
    if(qn.row!=0)
    {
        printfQN(qn);
        printf("登录成功！\n");
        return 1;
    }
    printf("登入失败！\n");
    return 0;
}

void bookdetail(Book b)//书籍信息插入函数
{
    char *sql="CREATE TABLE Book(Name, price, master);";//插入表头
    sqlite3_exec(db,sql,0,0,&msg);
    if (msg != 0)
    {
        // 如果是因为表格已经存在而出错就不需要退出程序，后面可以继续操作表格
        if (strcmp("table Book already exists", msg) != 0)
        {
            printf("执行创建表格出错%s\n", msg);
            return ;
        }
    }
    char buf[512];
    sprintf(buf,"INSERT INTO Book VALUES('%s','%s','%s');",b.bookname,b.price,b.master);//将用户信息写入命令
    sqlite3_exec(db,buf,0,0,&msg);
}

int bookdelte(Book b)//删除指定书籍
{
    char buf1[512];
    sprintf(buf1,"SELECT * FROM Book where Name='%s' and master='%s';",b.bookname,b.master);
    QN qn=get_table(db,buf1);
    printfQN(qn);
    if(qn.row!=0)
    {
        char buf[512];
        sprintf(buf,"DELETE FROM Book WHERE Name='%s' and master='%s';",b.bookname,b.master);//将书名信息写入命令
        sqlite3_exec(db,buf,0,0,&msg);
        return 1;
    }
    else{
        return 0;
    }
}

int check(int fd)  //书籍数据查询函数
{
    char buf[512]="SELECT * FROM Book;";
    QN qn=get_table(db,buf);
    Msg msg;
    Book book;
    msg.type=105;

    // qn.ppResult是一个二维数组，每一行代表一条记录，每一列代表一个字段
    for(size_t i=1; i<=qn.row; i++)  // 遍历每一行
    {
        strcpy(book.bookname, qn.ppResult[i * qn.col + 0]);  // 书名在第一列
        strcpy(book.price, qn.ppResult[i * qn.col + 1]);    // 价格在第二列
        strcpy(book.master, qn.ppResult[i * qn.col + 2]);   // 作者在第三列

        // 将book结构体的内容复制到msg中，然后发送
        memcpy(&msg.data, &book, sizeof(Book));
        write(fd, &msg, sizeof(msg));
    }
    msg.type=109;
    write(fd,&msg,sizeof(msg));
    return 0;
}

int bookrivse(int fd,Rivse r)
{
    char buf[512];
    char buf1[512];
    sprintf(buf1,"select * from Book where Name='%s' and master='%s';",r.bname,r.master);
    QN q=get_table(db,buf1);
    Msg m;
    if(q.row!=0)
    {
        // msg=NULL;
    sprintf(buf,"UPDATE Book SET %s='%s' WHERE Name='%s' and master='%s';",r.type,r.data,r.bname,r.master);
    sqlite3_exec(db, buf, 0, 0, &msg);
    m.type=107;
    write(fd,&m,sizeof(m));
    // if(msg==NULL)
    // return 1;
    // else return 0;
    }else{
            m.type=108;
    write(fd,&m,sizeof(m));
    }
    
}

int trade(Book b)  //交易函数
{
    time_t t;
    struct tm *local_time;    
    time(&t);
    
    local_time=localtime(&t);
    char buf1[512];
    sprintf(buf1,"SELECT * FROM Book where Name='%s' and master='%s';",b.bookname,b.master);
    QN qn=get_table(db,buf1);
    printfQN(qn);
    
    if(qn.row!=0)
    {
        strcpy(b.price,qn.ppResult[4]);//获取价格
        char buf[512];
        sprintf(buf,"DELETE FROM Book WHERE Name='%s' and master='%s';",b.bookname,b.master);//将书名信息写入命令
        sqlite3_exec(db,buf,0,0,&msg);
        //***********************************交易记录输入数据库
        char *sql="CREATE TABLE Purchase(purchaser,Name, price, master,tradeTime);";//插入表头
        sqlite3_exec(db,sql,0,0,&msg);
        if (msg != 0)
        {
        // 如果是因为表格已经存在而出错就不需要退出程序，后面可以继续操作表格
        if (strcmp("table Purchase already exists", msg) != 0)
        {
            printf("执行创建表格出错%s\n", msg);
            return -1;
        }
        }
        char buf1[512];
        char time1[512];
        sprintf(time1,"%d.%d.%d--%d.%d.%d",local_time->tm_year+1900,local_time->tm_mon+1,local_time->tm_mday,local_time->tm_hour,local_time->tm_min,local_time->tm_sec);
        sprintf(buf1,"INSERT INTO Purchase VALUES('%s','%s','%s','%s','%s');",b.puchaser,b.bookname,b.price,b.master,time1);//将用户信息写入命令
        sqlite3_exec(db,buf1,0,0,&msg);
        //**********************************
        return 1;
    }
    else{
        return 0;
    }
}

void sgchat(Msg msg)  //私聊函数
{
    int fd;
    sgChat *sg=(sgChat *)msg.data;
    for(int i=0;i<arrcount;i++)         //获取对方通信号
    {
        if(strcmp(carr[i].name,sg->duiname)==0)
        {
            fd=carr[i].fd;
            break;
        }
    }
    write(fd,&msg,sizeof(msg));
}

void qchat(Msg msg)   //群聊函数
{
    sgChat *sg=(sgChat *)msg.data;
    for(int i=0;i<arrcount;i++)         //获取对方通信号
    {
        if(strcmp(carr[i].name,sg->selfname)!=0)
        {
            write(carr[i].fd,&msg,sizeof(msg));
        }
    }
}

int backpwd(User user)   //密码找回函数
{
    char buf1[512];
    sprintf(buf1,"select * from User where Name='%s';",user.name);
    QN q=get_table(db,buf1);
    if(q.row!=0)
    {
         char buf[512];
    msg=NULL;
    sprintf(buf,"UPDATE User SET password='%s' WHERE Name='%s';",user.password,user.name);
    sqlite3_exec(db, buf, 0, 0, &msg);
    if(msg==NULL)
    return 1;
    else return 0;
    }else return 0;
   
}

void checktrade(int fd,Msg msg)  //查找自身交易记录
{
    char buf[512];
    printf("%s\n",msg.data);
    sprintf(buf,"SELECT * FROM Purchase where purchaser='%s';",msg.data);
    QN qn=get_table(db,buf);
    msg.type=115;
    pChase pc;
    // qn.ppResult是一个二维数组，每一行代表一条记录，每一列代表一个字段
    for(size_t i=1; i<=qn.row; i++)  // 遍历每一行
    {
        strcpy(pc.purchaser, qn.ppResult[i * qn.col + 0]);  // 购买者在第一列
        strcpy(pc.name, qn.ppResult[i * qn.col + 1]);    // 书名在第二列
        strcpy(pc.price, qn.ppResult[i * qn.col + 2]);   // 价格在第三列
        strcpy(pc.master, qn.ppResult[i * qn.col + 3]);   // 主任在第三列
        strcpy(pc.time, qn.ppResult[i * qn.col + 4]);   // 购买时间在第三列

        // 将Pchase结构体的内容复制到msg中，然后发送
        memcpy(&msg.data, &pc, sizeof(pc));
        write(fd, &msg, sizeof(msg));
    }
    msg.type=116;  //退出标志
    write(fd,&msg,sizeof(msg));
}

int main(int argc, char const *argv[])
{
    data=sqlite3_open("User.db",&db);
    if(data<0)
    {   

        printf("create user_db error：%s\n",sqlite3_errmsg(db));
        return -1;
    }else printf("create user_db successfully\n");

    int fd,efd,optval,ret;
    int count;          //记录发生可读事件的数量
    int accfd;
    struct sockaddr_in saddr,caddr;
    socklen_t len=sizeof(caddr);
    struct epoll_event event;
    struct epoll_event events[512];

    //创建套接字文件
    fd=socket(AF_INET,SOCK_STREAM,0);
    juge("socket",fd);
    //设置允许地址和端口被重新设置
    optval=-1;
    ret=setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    juge("setsockopt",ret);

    //获取服务器的地址和端口
    saddr=getaddr("192.168.100.129",9010);
    ret=bind(fd,(struct sockaddr*)&saddr,sizeof(saddr));
    juge("bind",ret);

    //启动监听器
    ret=listen(fd,1000);
    juge("listening",ret);
    printf("listenfd = %d server init success\n", fd);

    //创建epoll实例
    int epfd=epoll_create(512);
    juge("epoll create",epfd);

    event.events=EPOLLIN;
    event.data.fd=fd;
    ret=epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    juge("epoll ctl",ret);

    while(1)
    {
        //检测是否有资源准备就绪
        count=epoll_wait(epfd,events,512,1500000);
        printf("有%d个可读事件\n",count);
        if(count==-1)
        {
            perror("epoll_wait");
            return -1;
        }else if(count==0)
        {
            fprintf(stderr,"epoll_wait timeout..\n");
            continue;
        }
        for(int i=0;i<count;i++)
        {
            efd=events[i].data.fd;//获取准备就绪集合中的文件描述符
            if(events[i].events==EPOLLIN)
            {
                //读事件
                if(efd==fd)
                {
                    accfd=accept(fd,(struct sockaddr*)&caddr,&len);
                    juge("accpet",accfd);
                    //添加新的套接字至epoll
                    event.events=EPOLLIN;
                    event.data.fd=accfd;
                     ret=epoll_ctl(epfd,EPOLL_CTL_ADD,accfd,&event);
                     juge("epoll_ctl",ret);
                }
                else
                {
                    Msg msg;
                    ret=read(efd,&msg,sizeof(msg));
                    if(ret<0)
                    {
                        perror("read");
                        return -1;
                    }else if(ret==0)
                    {
                        event.data.fd=efd;
                        ret=epoll_ctl(epfd,EPOLL_CTL_DEL,efd,&event);
                        juge("epoll_ctl",ret);
                        close(efd);
                        break;
                    }
                    else
                    {
                        if(msg.type==1)//进入注册
                        {
                            User *user;
                            user=(User *)msg.data;
                            user_insert(accfd,*user);
                        }
                        else if(msg.type==2)//验证登入信息
                        {
                            User *user;
                            user=(User *)msg.data;
                            int j=cmp(*user);
                            if(j==0)
                            {
                                msg.type=102;//登入失败标志
                                write(accfd,&msg,sizeof(msg));
                            }else if(j==1)
                            {
                                msg.type=101;//登入成功标志
                                printf("%d  %s 登入成功！\n",accfd,user->name);
                                carr[arrcount].fd=accfd;   //将登入的通信接口加入结构体数组
                                strcpy(carr[arrcount].name,user->name);
                                printf("--%d %s--\n",carr[arrcount].fd,carr[arrcount].name);
                                arrcount++;
                                write(accfd,&msg,sizeof(msg));
                            }
                        }
                        else if(msg.type==3)//插入书籍信息
                        {
                            Book *book;
                            book=(Book *)msg.data;
                            bookdetail(*book);
                        }
                        else if(msg.type==4)//删除书籍
                        {
                            Book *book;
                            book=(Book *)msg.data;
                            ret=bookdelte(*book);
                            if(ret==0){
                                msg.type=103;//没找到要删书籍的标志
                                write(accfd,&msg,sizeof(msg));
                            }else if(ret==1)
                            {
                                msg.type=104;//删除成工的标志
                                write(accfd,&msg,sizeof(msg));
                            }
                        }
                        else if(msg.type==5)
                        {
                            check(accfd);
                            //msg.type=105;  //书籍信息传输标志
                            // memcpy(msg.data,&qn,sizeof(qn));
                            // write(accfd,&msg,sizeof(msg));
                        }
                        else if(msg.type==6)  //修改书籍数据
                        {
                            Rivse *r=(Rivse *)msg.data;
                            ret=bookrivse(accfd,*r);
                            // if(ret==1)
                            // {
                            //     msg.type=107;//修改成功标志
                            //     write(accfd,&msg,sizeof(msg));
                            // }else if(ret==0)
                            // {
                            //     msg.type=108;//修改失败标志
                            //     write(accfd,&msg,sizeof(msg));
                            // }
                        }
                        else if(msg.type==7)  //交易
                        {
                            Book *b=(Book *)msg.data;
                            ret=trade(*b);
                            if(ret==1)
                            {
                                msg.type=109;//购买成功标志
                                write(accfd,&msg,sizeof(msg));
                            }
                            else if(ret==0)
                            {
                                msg.type=110;//修改失败标志
                                write(accfd,&msg,sizeof(msg));
                            }
                        }
                        else if(msg.type==9)  //私聊
                        {
                            sgchat(msg);
                        }
                        else if(msg.type==10)  //群聊
                        {
                            qchat(msg);
                        }
                        else if(msg.type==11)  //密码找回
                        {
                            User *user=(User *)msg.data;
                            ret=backpwd(*user);
                            if(ret==1)
                            {
                                msg.type=200;  //修改成功
                                write(accfd,&msg,sizeof(msg));
                            }else if(ret==0)
                            {
                                msg.type=201; //修改失败
                                write(accfd,&msg,sizeof(msg));
                            }
                        }
                        else if(msg.type==12) //查找自身交易记录
                        {
                            checktrade(accfd,msg);
                        }
                    }
                }
            }
        }
    }
    //服务器退出，将fd从epoll中删除
    event.data.fd=fd;
    ret=epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&event);
    juge("epoll_ctl",ret);
    close(fd);
    close(epfd);
    sqlite3_close(db);//关闭数据库
    return 0;
}

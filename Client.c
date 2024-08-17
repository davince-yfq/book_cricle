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
#include <pthread.h>

int fd;
char cmpname[48];//与输入信息比较，禁止录入他人姓名
//改：1.查询出错

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
    short type;
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
//注册函数
void enroll()
{
    User user;
    printf("--请输入信息(姓名,密码,邮箱)--\n");
    scanf("%s%s%s",user.name,user.password,user.email);
    Msg msg;
    msg.type=1;
    memcpy(msg.data,&user,sizeof(user));
    write(fd,&msg,sizeof(msg));
    read(fd,&msg,sizeof(msg));
    if(msg.type==666)
    {
        printf("用户名重复，注册失败！\n");
    }
    else if(msg.type==777)
    {
        printf("创建成功！\n");
    }
}
//用户注册登入界面
void interface01()
{
    printf("*********************************\n");
    printf("*          Book-Circle           *\n");
    printf("*********************************\n");
    printf("*          1. 注册               *\n");
    printf("*          2. 登入               *\n");
    printf("*          3. 找回密码           *\n");
    printf("*          4. 返回               *\n");
    printf("*********************************\n");
    printf("请输入选项>> ");
}
//登入函数
int login()
{
    User user;
    memset(&user,0,sizeof(user));
    printf("--请输入姓名和密码--\n");
    scanf("%s%s",user.name,user.password);
    Msg msg;
    msg.type=2;
    memcpy(msg.data,&user,sizeof(user));
    write(fd,&msg,sizeof(msg));
    read(fd,&msg,sizeof(msg));
    if(msg.type==102)
    {
        printf("登入失败！\n");
        return 0;
    }else if(msg.type==101)
    {
        printf("登入成功！\n");
        strcpy(cmpname,user.name);
        return 1;
    }
}

void interface02()//用户功能界面
{
    printf("*********************************\n");
    printf("*          Book-Circle           *\n");
    printf("*********************************\n");
    printf("*          1. 书籍管理            *\n");
    printf("*          2. 交易管理            *\n");
    printf("*          3. 交流管理            *\n");
    printf("*          4. 返回                *\n");
    printf("*********************************\n");
    printf("请输入你的选择>>\n");
}

void interface03()//书籍功能界面
{
    printf("*********************************\n");
    printf("*           书籍管理             *\n");
    printf("*********************************\n");
    printf("*          1. 发布书籍            *\n");
    printf("*          2. 删除书籍            *\n");
    printf("*          3. 搜索书籍            *\n");
    printf("*          4. 修改数据            *\n");
    printf("*          5. 返回                *\n");
    printf("*********************************\n");
    printf("请输入你的选择>>\n");
}

void publish()//发布书籍函数
{
    Book book;
    printf("请输入书籍信息(书籍名/价格/主人)\n");
    scanf("%s%s%s",book.bookname,book.price,book.master);
    if(strcmp(cmpname,book.master)!=0)
    {
        printf("请正确输入自己姓名\n");
        return ;
    }
    Msg msg;
    msg.type=3;
    memcpy(msg.data,&book,sizeof(book));
    write(fd,&msg,sizeof(msg));
}

void delbook()//删除书籍函数
{
    Book book;
    memset(&book,0,sizeof(book));
    printf("--请输入要删除书籍的名字--\n");
    scanf("%s",book.bookname);
    strcpy(book.master,cmpname);
    Msg msg;
    msg.type=4;
    memcpy(msg.data,&book,sizeof(book));
    write(fd,&msg,sizeof(msg));
    read(fd,&msg,sizeof(msg));
    if(msg.type==103)
    {
        printf("未找到相应书籍!无法删除他人书籍信息\n");
    }else if(msg.type==104){
        printf("删除成功！\n");
    }
}

void checkbook()//书籍查询函数
{
    Msg msg;
    Book *book;
    memset(&msg,0,sizeof(msg));
    msg.type=5;
    write(fd,&msg,sizeof(msg));
    while(1)
    {
        read(fd,&msg,sizeof(msg));
        if(msg.type==109)
        break;
        book=(Book *)msg.data;
        printf("书名:%-20s价格:%-4s主人:%-5s\n",book->bookname,book->price,book->master);
    } 
}

void bookrivse()//书籍信息修改
{
    Rivse r;
    printf("请输入要修改的书名>>\n");
    scanf("%s",r.bname);
    getchar();
    printf("请输入要修改的类型>>\n");
    scanf("%s",r.type);
    getchar();
    printf("请输入要修改的内容>>\n");
    scanf("%s",r.data);
    strcpy(r.master,cmpname);
    Msg msg;
    msg.type=6;
    memcpy(msg.data,&r,sizeof(r));
    write(fd,&msg,sizeof(msg));
    read(fd,&msg,sizeof(msg));
    if(msg.type==107)
    {
        printf("修改成功！\n");
    }else if(msg.type==108){
        printf("修改失败，数据错误或不能修改他人数据！\n");
    }
}

void trade()  //交易函数
{
    Book book;
    memset(&book,0,sizeof(book));
    printf("--请输入想要购买的书名及其主人--\n");
    scanf("%s%s",book.bookname,book.master);
    if(strcmp(book.master,cmpname)==0)
    {
        printf("无法购买自己的书籍！\n");
        return ;
    }
    strcpy(book.puchaser,cmpname);
    Msg msg;
    msg.type=7;
    memcpy(msg.data,&book,sizeof(book));
    write(fd,&msg,sizeof(msg));
    read(fd,&msg,sizeof(msg));
    if(msg.type==110)
    {
        printf("未找到相应书籍!\n");
    }else if(msg.type==109){
        printf("购买成功！\n");
    }
}

void interface04()//聊天管理界面
{
    printf("*********************************\n");
    printf("*           交流管理             *\n");
    printf("*********************************\n");
    printf("*           1. 私聊             *\n");
    printf("*           2. 群聊             *\n");
    printf("*           3. 返回             *\n");
    printf("*********************************\n");
    printf("请输入你的选择>>\n");
}

void *work(void *p)//线程接收消息
{
    Msg msg;
    sgChat *sg;
    while(1)
    {
        int ret=read(fd,&msg,sizeof(msg));
        if(ret==0) break;
        sg=(sgChat *)msg.data;
        if(msg.type==9 || msg.type==10)
        {
            printf("[%s]:%s\n",sg->selfname,sg->data);
        }
    }
}

void singlechat()//私聊函数
{
    pthread_t p;
    if(pthread_create(&p,NULL,work,NULL)!=0)
    {
        perror("pthread_create");
        return;
    }
    char buf[512];
    sgChat sg;
    printf("--请输入私聊对象的名称--\n");
    scanf("%s",sg.duiname);
    Msg msg;
    msg.type=9;
    while (1)
    {
        scanf("%s",buf);
        if(strcmp(buf,"quit")==0)
        {
            break;
        }
        strcpy(sg.selfname,cmpname);
        strcpy(sg.data,buf);
        memcpy(msg.data,&sg,sizeof(sg));
        write(fd,&msg,sizeof(msg));
    }
    
    pthread_cancel(p);
}

void qunchat()//群聊函数
{
    pthread_t p;
    if(pthread_create(&p,NULL,work,NULL)!=0)
    {
        perror("pthread_create");
        return;
    }
    char buf[512];
    sgChat sg;
    Msg msg;
    msg.type=10;
    while (1)
    {
        scanf("%s",buf);
        if(strcmp(buf,"quit")==0)
        {
            break;
        }
        strcpy(sg.selfname,cmpname);
        strcpy(sg.data,buf);
        memcpy(msg.data,&sg,sizeof(sg));
        write(fd,&msg,sizeof(msg));
    }
    
    pthread_cancel(p);
}

void backpassword()//密码找回函数
{
    User user;
    Msg msg;
    char pwd[48],pwd1[48];
    printf("--请输入用户名--\n");
    scanf("%s",user.name);
    printf("--请输入新密码--\n");
    scanf("%s",pwd);
    printf("--请再次确认密码--\n");
    scanf("%s",pwd1);
    if(strcmp(pwd1,pwd)==0)
    {
        strcpy(user.password,pwd1);
        msg.type=11;
        memcpy(msg.data,&user,sizeof(user));
        write(fd,&msg,sizeof(msg));
    }
    else{
        printf("--密码确认错误--\n");
        return ;
    }
    read(fd,&msg,sizeof(msg));
    if(msg.type==200)
    {
        printf("修改成功！\n");
    }else if(msg.type==201)
    {
        printf("修改失败,没有找到用户名！\n");
    }
}

void interface05() //交易管理界面
{
    printf("*********************************\n");
    printf("*           交易管理             *\n");
    printf("*********************************\n");
    printf("*           1. 交易             *\n");
    printf("*           2. 查询交易数据      *\n");
    printf("*           3. 返回             *\n");
    printf("*********************************\n");
    printf("请输入你的选择>>\n");
}

void checktrade()  //查询交易数据
{
    Msg msg;
    msg.type=12;
    pChase *p;
    strcpy(msg.data,cmpname);
    write(fd,&msg,sizeof(msg));
    while(1)
    {
        read(fd,&msg,sizeof(msg));
        if(msg.type==116)
        break;
        p=(pChase *)msg.data;
        printf("购买者:%-10s书名:%-4s    价格:%-3s主人:%-5s时间:%-10s\n",p->purchaser,p->name,p->price,p->master,p->time);
    } 
}

int main(int argc, char const *argv[])
{
    int ret;
    char c;
    int loop=1;//用户功能循环控制
    int loop1=1;//主界面循环控制
    int loop2=1;//书记管理界面循环控制
    int loop3=1;//交流界面循环控制
    int loop4=1;//交易管理循环控制
    fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in saddr;
    saddr=getaddr("192.168.100.129",9010);
    if(connect(fd,(struct sockaddr*)&saddr,sizeof(saddr))==-1)
    {
        perror("connect error");
        return -1;
    }

    while(loop1)//用户主界面
    {
        int sel;
        interface01();
        scanf("%d",&sel);
        switch (sel)
        {
        case 1:
        enroll();//用户注册
            break;
        case 2:
        if(login()==1)//用户登入
            {
                loop=1;
                int sel2;
                while(loop)//用户功能界面
                {
                    interface02();
                    scanf("%d",&sel2);
                    switch(sel2)
                    {
                        int sel3,sel4,sel5;
                        case 1:
                            loop2=1;
                            while(loop2)//书籍功能界面
                            {
                                interface03();
                                scanf("%d",&sel3);
                                switch(sel3)
                                {
                                case 1:
                                publish();   //书籍信息插入
                                break;
                                case 2:
                                delbook();      //书籍删除
                                break;
                                case 3:
                                    checkbook();    //查询数据
                                break;
                                case 4:
                                bookrivse();   //数据修改
                                break;
                                case 5:
                                loop2=0;       //返回
                                break;
                                default:
                                printf("--please enter the true selection--\n");
                                break;
                                }
                            }
                            
                            break;
                        case 2:   //交易管理
                        loop4=1;
                        while(loop4)
                        {
                            interface05();
                            scanf("%d",&sel4);
                            switch(sel4)
                            {
                                case 1:  //交易
                                trade();
                                break;
                                case 2:  //查询交易数据
                                checktrade();
                                break;
                                case 3:  //返回
                                loop4=0;
                                break;
                                default:
                                printf("--please enter the true selection--\n");
                                break;
                            }
                        }
                        
                        break;
                        case 3:   //交流管理
                        loop3=1;
                        while(loop3)
                        {
                            interface04();  //交流界面
                        scanf("%d",&sel5);
                        switch(sel5)
                        {
                            case 1://私聊
                            singlechat();
                            break;
                            case 2://群聊
                            qunchat();
                            break;
                            case 3://返回
                            loop3=0;
                            break;
                            default:
                            printf("--please enter the true selection--\n");
                            break;
                        }
                        }
                        
                        break;
                        case 4:
                        loop=0;
                        break;
                        default:
                        printf("--please enter the true selection--\n");
                        break;
                    }
                }
            }
            break;
        case 3://找回密码
        backpassword();
        break;
        case 4:
        loop1=0;
        break;
        default:
        printf("--please enter the true selection--\n");
            break;
        }
    }

    close(fd);
    return 0;
}

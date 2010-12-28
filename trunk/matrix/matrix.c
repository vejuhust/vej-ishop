#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/uio.h>
#include <syslog.h>


#define LOCK_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) /*锁文件访问权限*/
#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) /*数据文件访问权限*/
#define LOCK_FILE   "/var/run/iShopDaemon.pid"              /*锁文件*/
#define FIFO_IN     "/tmp/iShopFIFOinput"                   /*FIFO输入管道*/
#define FIFO_OUT    "/tmp/iShopFIFOoutput"                  /*FIFO输出管道*/
#define DATA_FW     "iShopWebData"                          /*一级链表数据文件*/
#define DATA_FS     "iShopShopData"                         /*二级链表数据文件*/
#define DATA_FO     "iShopOrderData"                        /*三级链表数据文件*/
#define ID_MAX      22                                      /*ID类字符串长度*/
#define NAME_MAX    32                                      /*名称类字符串长度*/
#define URL_MAX     52                                      /*URL字符串长度*/
#define RECENT_MAX  42                                      /*RECENT字符串长度*/
#define TMPSTR_MAX  102400                                  /*临时字符串长度*/
#define MSG_MAX     102400                                  /*输入指令串长度*/
#define WEB_MAX     1024                                    /*一级链表最大节点数*/
#define SHOP_MAX    1024                                    /*二级链表最大节点数*/
#define WRITEV_MAX  1024                                    /*writev最大项目数*/
#define WAIT_SEC    1800                                    /*指令等待时间上限*/


extern int errno;                                   /*错误代号变量*/

char  * passwd         = "19920506";                /*WXY指令用密码*/
long    run            = 1;                         /*1接受下一条指令，0退出*/
long    save           = 1;                         /*1已经保存，0直接退出*/

long    web_num_max    = 0;                         /*当前最大的网站内部编号*/
long    order_num_max  = 0;                         /*当前最大的交易内部编号*/

long    cntw           = 0;                         /*Website节点个数*/
long    cnts           = 0;                         /*Shop节点个数*/
long    cnto           = 0;                         /*Order节点个数*/

struct web_t       * web_head   = NULL;             /*Website链头节点指针*/
struct web_t       * web_tail   = NULL;             /*Website链尾节点指针*/


/*日期类型struct结构*/
struct date_t {
    long             day;                       /*日*/
    long             month;                     /*月*/
    long             year;                      /*年*/
};

/*Website链节点struct结构*/
struct web_t {
    char             web_name  [NAME_MAX];      /*网站名称*/
    char             url       [URL_MAX];       /*网站地址*/
    long             web_num;                   /*网站内部编号*/
    long             cnt;                       /*网站店铺总数*/
    struct web_t   * next;                      /*后继Website链节点*/
    struct shop_t  * sub;                       /*Shop子链首节点*/
};

/*Shop链节点struct结构*/
struct shop_t {
    long             web_num;                   /*所属网站内部编号*/
    char             shop_id   [ID_MAX];        /*店铺编号*/
    char             shop_name [NAME_MAX];      /*店铺名称*/
    char             owner     [NAME_MAX];      /*负责人姓名*/
    long             shop_area;                 /*负责人所在地区*/
    long             bank;                      /*店铺开户银行*/
    struct shop_t  * next;                      /*后继Shop链节点*/
    struct order_t * sub;                       /*Order子链首节点*/
};

/*Order链节点struct结构*/
struct order_t {
    long             web_num;                   /*所属网站内部编号*/
    char             shop_id   [ID_MAX];        /*所属店铺编号*/
    long             order_num;                 /*交易内部编号*/
    long             pay;                       /*支付类型*/
    long double      money;                     /*交易金额*/
    struct date_t    date;                      /*交易日期*/
    long             order_area;                /*支付人所在地区*/
    char             recent    [RECENT_MAX];    /*最近一次修改时间*/
    struct order_t * next;                      /*后继Order链节点*/
};

/*店铺年度报表struct结构*/
struct state_t {
    long             web_num;                   /*所属网站内部编号*/
    char             shop_id   [ID_MAX];        /*店铺编号*/
    long             cnt;                       /*当年的交易次数*/
    long double      sum;                       /*当年的交易金额*/
};

/*网站龙虎榜struct结构*/
struct top5_t {
    long             web_num;                   /*网站内部编号*/
    char             web_name  [NAME_MAX];      /*网站名称*/
    long             cnt;                       /*网站下属各个店铺当月总交易次数*/
};


/*所在地区代号*/
char * area_list[] = {
    NULL,
    "Anhui",                                    /*01 安徽省*/
    "Beijing",                                  /*02 北京市*/
    "Chongqing",                                /*03 重庆市*/
    "Fujian",                                   /*04 福建省*/
    "Gansu",                                    /*05 甘肃省*/
    "Guangdong",                                /*06 广东省*/
    "Guangxi",                                  /*07 广西壮族自治区*/
    "Guizhou",                                  /*08 贵州省*/
    "Hainan",                                   /*09 海南省*/
    "Hebei",                                    /*10 河北省*/
    "Heilongjiang",                             /*11 黑龙江省*/
    "Henan",                                    /*12 河南省*/
    "Hong Kong",                                /*13 香港特别行政区*/
    "Hubei",                                    /*14 湖北省*/
    "Hunan",                                    /*15 湖南省*/
    "Inner Mongolia",                           /*16 内蒙古自治区*/
    "Jiangsu",                                  /*17 江苏省*/
    "Jiangxi",                                  /*18 江西省*/
    "Jilin",                                    /*19 吉林省*/
    "Liaoning",                                 /*20 辽宁省*/
    "Macau",                                    /*21 澳门特别行政区*/
    "Ningxia",                                  /*22 宁夏回族自治区*/
    "Qinghai",                                  /*23 青海省*/
    "Shaanxi",                                  /*24 陕西省*/
    "Shandong",                                 /*25 山东省*/
    "Shanghai",                                 /*26 上海市*/
    "Shanxi",                                   /*27 山西省*/
    "Sichuan",                                  /*28 四川省*/
    "Taiwan",                                   /*29 台湾地区*/
    "Tianjin",                                  /*30 天津市*/
    "Tibet",                                    /*31 西藏自治区*/
    "Xinjiang",                                 /*32 新疆维吾尔自治区*/
    "Yunnan",                                   /*33 云南省*/
    "Zhejiang",                                 /*34 浙江省*/
    NULL
};

/*开户银行代号*/
char * bank_list[] = {
    NULL,
    "Agricultural Bank of China",               /*01*/
    "Bank of Beijing",                          /*02*/
    "Bank of China",                            /*03*/
    "Bank of Communications",                   /*04*/
    "Bank of Jilin",                            /*05*/
    "Bank of Jinzhou",                          /*06*/
    "Bank of Ningbo",                           /*07*/
    "Bank of Shanghai",                         /*08*/
    "Bohai Bank",                               /*09*/
    "China Bohai Bank",                         /*10*/
    "China CITIC Bank",                         /*11*/
    "China Construction Bank",                  /*12*/
    "China Development Bank",                   /*13*/
    "China Everbright Bank",                    /*14*/
    "China Merchants Bank",                     /*15*/
    "China Minsheng Bank",                      /*16*/
    "China Zheshang Bank",                      /*17*/
    "Dalian Bank",                              /*18*/
    "Evergrowing Bank",                         /*19*/
    "Exim Bank of China",                       /*20*/
    "Guangdong Development Bank",               /*21*/
    "Harbin Bank",                              /*22*/
    "Hua Xia Bank",                             /*23*/
    "Industrial and Commercial Bank of China",  /*24*/
    "Industrial Bank Co.",                      /*25*/
    "Ping An Bank",                             /*26*/
    "Postal Savings Bank of China",             /*27*/
    "Shanghai Pudong Development Bank",         /*28*/
    "Shengjing Bank",                           /*29*/
    "Shenzhen City Commercial Bank",            /*30*/
    "Shenzhen Development Bank",                /*31*/
    "Xiamen International Bank",                /*32*/
    "Zhejiang Tailong Commercial Bank",         /*33*/
    NULL
};

/*交易类型代号*/
char * pay_list[] = {
    NULL,
    "Credit Card",                              /*01 信用卡*/
    "Gift Card",                                /*02 代金券*/
    "Bank Account",                             /*03 银行转帐*/
    "Cash on Delivery",                         /*04 货到付款*/
    NULL
};


char         tmpstr [TMPSTR_MAX];               /*临时字符串*/
char         msg    [MSG_MAX];                  /*输入指令串*/

void       * queue  [WEB_MAX];                  /*state_output数据队列*/

struct iovec iov    [WRITEV_MAX];               /*writev写入队列*/


/*用于初始化并提取指令串第一个字符串型数据的宏函数*/
#define get_first_string(FIELD_NAME)                    \
  { i    = 0;                                           \
    tail = strstr(msg, "\n");                           \
    for (head = msg; head != tail; ++head) {            \
        tmpstr[i++] = *head;                            \
    }                                                   \
    tmpstr[i] = '\0';                                   \
    if (NULL == clean_str(tmpstr)) {                    \
        error_data(FIELD_NAME " needed!");              \
        return;                                         \
    } };                                                \


/*用于提取指令串第二个或更高字符串型数据的宏函数*/
#define get_another_string(FIELD_NAME)                  \
  { i    = 0;                                           \
    tail = strstr(tail + 1, "\n");                      \
    for (++head; head != tail; ++head) {                \
        tmpstr[i++] = *head;                            \
    }                                                   \
    tmpstr[i] = '\0';                                   \
    if (NULL == clean_str(tmpstr)) {                    \
        error_data(FIELD_NAME " needed!");              \
        return;                                         \
    } };


/*用于提取并更新字符串型数据的宏函数*/
#define update_string_item(FIELD, LIMIT)                \
  { i = 0;                                              \
    tail = strstr(tail + 1, "\n");                      \
    for (++head; head != tail; ++head) {                \
        tmpstr[i++] = *head;                            \
    }                                                   \
    tmpstr[i] = '\0';                                   \
    if (NULL == clean_str(tmpstr)) {                    \
        strcpy(new->FIELD, old->FIELD);                 \
    }                                                   \
    else {                                              \
        tmpstr[LIMIT-1] = '\0';                         \
        strcpy(new->FIELD, tmpstr);                     \
    } };


/*用于提取并更新长整型数据的宏函数*/
#define update_long_item(FIELD)                         \
  { i = 0;                                              \
    tail = strstr(tail + 1, "\n");                      \
    for (++head; head != tail; ++head) {                \
        tmpstr[i++] = *head;                            \
    }                                                   \
    tmpstr[i] = '\0';                                   \
    if (NULL == clean_str(tmpstr)) {                    \
        new->FIELD = old->FIELD;                        \
    }                                                   \
    else {                                              \
        sscanf(tmpstr, "%ld", &i);                      \
        new->FIELD = i;                                 \
    } };


/*用于提取并更新扩展型双精度浮点型数据的宏函数*/
#define update_float_item(FIELD)                        \
  { i = 0;                                              \
    tail = strstr(tail + 1, "\n");                      \
    for (++head; head != tail; ++head) {                \
        tmpstr[i++] = *head;                            \
    }                                                   \
    tmpstr[i] = '\0';                                   \
    if (NULL == clean_str(tmpstr)) {                    \
        new->FIELD = old->FIELD;                        \
    }                                                   \
    else {                                              \
        long double f;                                  \
        sscanf(tmpstr, "%Lf", &f);                      \
        new->FIELD = f;                                 \
    } };


/*针对GNU特有的strcasestr()的函数原型之声明*/
/*warning: strcasestr is a GNU specific extension to the C standard.*/
char * strcasestr(const char *haystack, const char *needle);


/*创建FIFO管道*/
void create_fifo(void) {

    /*创建FIFO输入管道*/
    if ((mkfifo(FIFO_IN, FILE_MODE) < 0) && (EEXIST != errno)) {

        /*向syslogd输出错误原因*/
        syslog(LOG_ERR, "%m");
        exit(1);
    }

    /*创建FIFO输出管道*/
    if ((mkfifo(FIFO_OUT, FILE_MODE) < 0) && (EEXIST != errno)) {

        /*删除FIFO输入管道*/
        unlink(FIFO_IN);

        /*向syslogd输出错误原因*/
        syslog(LOG_ERR, "%m");
        exit(2);
    }
}


/*删除FIFO管道*/
void delete_fifo(void) {

    /*删除FIFO输入管道*/
    if (0 > unlink(FIFO_IN)) {

        /*向syslogd输出错误原因*/
        syslog(LOG_ERR, "%m");
        exit(3);
    }

    /*删除FIFO输出管道*/
    if (0 > unlink(FIFO_OUT)) {

        /*向syslogd输出错误原因*/
        syslog(LOG_ERR, "%m");
        exit(4);
    }
}


/*向syslogd输出错误信息msg*/
int app_error(char * msg) {
    syslog(LOG_ERR, "%s", msg);
    exit(5);
}


/*删去str字符串的首尾空白字符(空格或制表符)，以及将中间的制表符替换为空格*/
char * clean_str(char * str) {
    char * head = NULL;             /*第一个非空白字符字符位置*/
    char * tail = str;              /*最后一个非空白字符位置*/
    char * old  = str;              /*字符串首地址*/

    /*若字符为空，则返回NULL*/
    if ('\0' == *str) {
        return(NULL);
    }

    /*查找字符串第一个非空白字符的位置*/
    for (; '\0' != *tail; ++tail) {
        if (('\t' != *tail) && (' ' != *tail)) {
            head = tail;
            break;
        }
    }

    /*若字符串仅由空白字符构成，则返回NULL*/
    if (NULL == head) {
        *str = '\0';
        return(NULL);
    }

    /*查找字符串最后一个字符的位置*/
    do {
        ++tail;
    } while ('\0' != *tail);

    /*查找字符串最后一个非空白字符的位置*/
    for (--tail; head != tail; --tail) {
        if (('\t' != *tail) && (' ' != *tail)) {
            break;
        }
    }

    /*直接在原字符串上整理，复制第一个和最后一个非空白字符间的字符，将制表符替换为空格*/
    for (++tail; head != tail; ++head, ++str) {
        if ('\t' == *head) {
            *str = ' ';
        }
        else {
            *str = *head;
        }
    }
    *str = '\0';

    /*返回字符串首地址*/
    return (old);
}


/*返回带操作序列号的时间字符串，精确到微秒*/
char * timestr(void) {
    static int      cnt = 0;
    char          * str;
    struct timeval  t1;
    struct tm     * t2;

    /*获取当前时间*/
    gettimeofday(&t1, NULL);
    t2 = localtime(&t1.tv_sec);
    str = (char *) calloc(100, sizeof(char));

    /*将当前时间整理后写入字符串str*/
    sprintf(str, "%.5d_%4d-%.2d-%.2d %.2d:%.2d:%.2d.%.6ld", ++cnt,
                t2->tm_year + 1900, t2->tm_mon + 1, t2->tm_mday,
                t2->tm_hour, t2->tm_min, t2->tm_sec, (long) t1.tv_usec);

    /*返回字符串首地址*/
    return (str);
}


/*输出调用者(指令): ABC DEF MNO T*/
/*向用于输出的FIFO管道按<type><len><aim: len bytes>格式写入反馈信息*/
void single_echo(char type, long len, void * aim) {
    int    fd;

    /*设置输出序列*/
    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &len;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = aim;
    iov[2].iov_len  = len;

    /*聚集写入FIFO管道*/
    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    writev(fd, iov, 3);
    close(fd);
}


/*输出调用者(指令): ABC DEF GHI L MNO RSTV WXY*/
/*向用于输出的FIFO管道写入存有错误信息的msg字符串*/
void error_data(char * msg) {
    int    fd;
    long   len;
    char   type = 'Z';

    /*设置输出序列*/
    len = strlen(msg);

    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &len;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = msg;
    iov[2].iov_len  = len;

    /*聚集写入FIFO管道*/
    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    writev(fd, iov, 3);
    close(fd);
}


/*遍历Website链，查找网站名称为web_name或网站地址为url或网站内部编号等于web_num的节点*/
/*不考虑地址为not的节点*/
struct web_t * find_web(char * web_name, char * url, long web_num, void * not) {
    struct web_t * web;

    /*遍历Website链*/
    web = web_head;
    while (NULL != web) {
        if ( ( (0 == strcasecmp(web->web_name, web_name)) ||
               (0 == strcasecmp(web->url, url)) ||
               (web->web_num == web_num) ) && (web != not) ) {

            /*查到目标，返回节点地址*/
            return (web);
        }
        web = web->next;
    }

    /*若未能查到，则返回NULL*/
    return (NULL);
}


/*遍历Website节点web的Shop子链，查找店铺编号为shop_id或店铺名称为shop_name的节点*/
/*不考虑地址为not的节点*/
struct shop_t * find_shop(struct web_t * web, char * shop_id,
                                              char * shop_name, void * not) {
    struct shop_t * shop;

    /*遍历Shop链*/
    shop = web->sub;
    while (NULL != shop) {
        if ( ( (0 == strcasecmp(shop->shop_id, shop_id)) ||
               (0 == strcasecmp(shop->shop_name, shop_name)) )
             && (shop != not ) ) {

            /*查到目标，返回节点地址*/
            return (shop);
        }
        shop = shop->next;
    }

    /*若未能查到，则返回NULL*/
    return (NULL);
}


/*遍历Shop节点shop的Order子链，查找交易内部编号等于order_num的节点*/
/*不考虑地址为not的节点*/
struct order_t * find_order(struct shop_t * shop, long order_num, void * not) {
    struct order_t * order;

    /*遍历Order链*/
    order = shop->sub;
    while (NULL != order) {
        if ( ( order->order_num == order_num ) && (order != not ) ) {

            /*查到目标，返回节点地址*/
            return (order);
        }
        order = order->next;
    }

    /*若未能查到，则返回NULL*/
    return (NULL);
}


/*将new节点插入Website链中*/
void add_web_item(struct web_t * new) {
    if (NULL == web_head) {
        web_head = new;
        web_tail = new;
    }
    else {
        web_tail->next = new;
        web_tail       = new;
    }
}


/*将new节点插入Website节点web的Shop子链中，并更新web节点的网站店铺总数*/
void add_shop_item(struct web_t * web, struct shop_t * new) {
    new->next = web->sub;
    web->sub  = new;
    ++(web->cnt);
}


/*将new节点插入Shop节点shop的Order子链中*/
void add_order_item(struct shop_t * shop, struct order_t * new) {
    new->next = shop->sub;
    shop->sub = new;
}


/*指令A: 添加新的Website记录*/
void add_web(char * msg) {
    struct web_t * new;
    char         * head = msg;
    char         * tail;
    long           i    = 0;

    /*提取网站名称web_name*/
    get_first_string("web_name");
    tmpstr[NAME_MAX - 1] = '\0';

    /*为新节点分配空间，存储网站名称web_name*/
    new = (struct web_t *) calloc(1, sizeof(struct web_t));
    strcpy(new->web_name, tmpstr);

    /*提取网站地址url*/
    get_another_string("url");
    tmpstr[URL_MAX - 1] = '\0';

    /*存储网站地址url*/
    strcpy(new->url, tmpstr);

    /*判断网站名称web_name或网站地址url是否与现有数据重复*/
    if (NULL != find_web(new->web_name, new->url, 0, NULL)) {
        error_data("this website record already exists!");
        free(new);
        return;
    }

    /*分配新的网站内部编号web_num*/
    new->web_num = ++web_num_max;

    /*初始化新节点指针*/
    new->next = NULL;
    new->sub  = NULL;
    new->cnt  = 0;

    /*将新节点加入Website链表中*/
    add_web_item(new);

    /*向FIFO输出管道写入反馈信息*/
    single_echo('A', sizeof(struct web_t), new);
}


/*指令B: 添加新的Shop记录*/
void add_shop(char * msg) {
    struct web_t  * web;
    struct shop_t * new;
    char          * head = msg;
    char          * tail;
    long            i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查询该Website节点*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*为新节点分配空间，存储网站内部编号web_num*/
    new = (struct shop_t *) calloc(1, sizeof(struct shop_t));
    new->web_num = i;

    /*提取店铺编号shop_id*/
    get_another_string("shop_id");
    tmpstr[ID_MAX - 1] = '\0';

    /*存储店铺编号shop_id*/
    strcpy(new->shop_id, tmpstr);

    /*提取店铺名称shop_name*/
    get_another_string("shop_name");
    tmpstr[NAME_MAX - 1] = '\0';

    /*存储店铺名称shop_name*/
    strcpy(new->shop_name, tmpstr);

    /*判断店铺编号shop_id或店铺名称shop_name是否与现有数据重复*/
    if (NULL != find_shop(web, new->shop_id, new->shop_name, NULL)) {
        error_data("this shop record already exists!");
        free(new);
        return;
    }

    /*提取负责人姓名owner*/
    get_another_string("owner");
    tmpstr[NAME_MAX - 1] = '\0';

    /*存储负责人姓名owner*/
    strcpy(new->owner, tmpstr);

    /*提取负责人所在地区shop_area*/
    get_another_string("shop_area");

    /*存储负责人所在地区shop_area*/
    i = 0;
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong shop_area!");
        free(new);
        return;
    }
    new->shop_area = i;

    /*提取店铺开户银行bank*/
    get_another_string("bank");

    /*存储店铺开户银行bank*/
    i = 0;
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong bank!");
        free(new);
        return;
    }
    new->bank = i;

    /*初始化新节点指针*/
    new->sub  = NULL;

    /*将新节点加入对应的Shop链表中*/
    add_shop_item(web, new);

    /*向FIFO输出管道写入反馈信息*/
    single_echo('B', sizeof(struct shop_t), new);
}


/*指令C: 添加新的Order记录*/
void add_order(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * new;
    char           * head  = msg;
    char           * tail;
    long             i     = 0;
    long double      money = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查询该Website节点*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*为新节点分配空间，存储网站内部编号web_num*/
    new = (struct order_t *) calloc(1, sizeof(struct order_t));
    new->web_num = i;

    /*提取店铺编号shop_id*/
    get_another_string("shop_id");
    tmpstr[ID_MAX - 1] = '\0';

    /*根据店铺编号shop_id查询该Shop节点*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        free(new);
        return;
    }

    /*存储店铺编号shop_id*/
    strcpy(new->shop_id, tmpstr);

    /*提取支付类型pay*/
    get_another_string("payment");

    /*存储支付类型pay*/
    i = 0;
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong payment!");
        free(new);
        return;
    }
    new->pay = i;

    /*提取交易金额money*/
    get_another_string("money");

    /*存储交易金额money*/
    money = 0;
    if (0 == sscanf(tmpstr, "%Lf", &money)) {
        error_data("wrong money!");
        free(new);
        return;
    }
    new->money = money;

    /*提取并存储交易日期-日date.day*/
    get_another_string("day");
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong day!");
        free(new);
        return;
    }
    new->date.day = i;

    /*提取并存储交易日期-月date.month*/
    get_another_string("month");
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong month!");
        free(new);
        return;
    }
    new->date.month = i;

    /*提取并存储交易日期-年date.year*/
    get_another_string("year");
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong year!");
        free(new);
        return;
    }
    new->date.year = i;

    /*提取支付人所在地区order_area*/
    get_another_string("order_area");

    /*存储支付人所在地区shop_area*/
    i = 0;
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong order_area!");
        free(new);
        return;
    }
    new->order_area = i;

    /*分配新的交易内部编号order_num*/
    new->order_num = ++order_num_max;

    /*设置最近一次修改时间recent*/
    strcpy(new->recent, timestr());

    /*将新节点加入对应的Order链表中*/
    add_order_item(shop, new);

    /*向FIFO输出管道写入反馈信息*/
    single_echo('C', sizeof(struct order_t), new);
}


/*查找并返回该Website节点的上一节点*/
struct web_t * prev_web(struct web_t * aim) {
    struct web_t * tmp;

    /*若为头节点，则返回NULL*/
    if (aim == (tmp = web_head) ) {
        return (NULL);
    }

    /*遍历并查找该节点，返回前驱*/
    while (NULL != tmp) {
        if (tmp->next == aim) {
            return (tmp);
        }
        tmp = tmp->next;
    }

    return (NULL);
}


/*查找并返回该Shop节点的上一节点*/
struct shop_t * prev_shop(struct shop_t * aim) {
    struct shop_t * tmp;

    /*若为头节点，则返回NULL*/
    if (aim == (tmp = find_web("", "", aim->web_num, NULL)->sub) ) {
        return (NULL);
    }

    /*遍历并查找该节点，返回前驱*/
    while (NULL != tmp) {
        if (tmp->next == aim) {
            return (tmp);
        }
        tmp = tmp->next;
    }

    return (NULL);
}


/*查找并返回该Order节点的上一节点*/
struct order_t * prev_order(struct order_t * aim) {
    struct order_t * tmp;

    /*若为头节点，则返回NULL*/
    if (aim == (tmp = find_shop(find_web("", "", aim->web_num, NULL),
                                aim->shop_id, "", NULL)->sub) ) {
        return (NULL);
    }

    /*遍历并查找该节点，返回前驱*/
    while (NULL != tmp) {
        if (tmp->next == aim) {
            return (tmp);
        }
        tmp = tmp->next;
    }

    return (NULL);
}


/*指令D: 更新已有的Website记录*/
void replace_web(char * msg) {
    struct web_t * new;
    struct web_t * old;
    char         * head = msg;
    char         * tail;
    long           i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (old = find_web("", "", i, NULL))) {
        error_data("wrong old web_num!");
        return;
    }

    /*为新节点分配空间*/
    new = (struct web_t *) calloc(1, sizeof(struct web_t));

    /*更新网站名称web_name*/
    update_string_item(web_name, NAME_MAX);

    /*更新网站地址url*/
    update_string_item(url, URL_MAX);

    /*判断网站名称web_name或网站地址url是否与现有的其他数据重复*/
    if (NULL != find_web(new->web_name, new->url, 0, old)) {
        error_data("this website record already exists!");
        free(new);
        return;
    }

    /*复制网站内部编号web_num*/
    new->web_num = old->web_num;

    /*复制网址店铺总数cnt*/
    new->cnt  = old->cnt;

    /*继承原节点指针关系*/
    new->next = old->next;
    new->sub  = old->sub;

    /*原节点前驱指向新节点*/
    if (NULL != prev_web(old)) {
        prev_web(old)->next = new;
    }

    /*若原节点为Website链首节点*/
    if (old == web_head) {
        web_head = new;
    }

    /*若原节点为Website链尾节点*/
    if (old == web_tail) {
        web_tail = new;
    }

    /*释放原节点所占用的堆空间*/
    free(old);

    /*向FIFO输出管道写入反馈信息*/
    single_echo('D', sizeof(struct web_t), new);
}


/*指令E: 更新已有的Shop记录*/
void replace_shop(char * msg) {
    struct web_t  * old_web;
    struct web_t  * web;
    struct shop_t * new;
    struct shop_t * old;
    char          * head = msg;
    char          * tail;
    long            i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong old web_num!");
        return;
    }
    old_web = web;

    /*提取店铺编号shop_id*/
    get_another_string("shop_id");

    /*根据店铺编号shop_id查找此店铺*/
    if (NULL == (old = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong old shop_id!");
        return;
    }

    /*为新节点分配空间*/
    new = (struct shop_t *) calloc(1, sizeof(struct shop_t));

    /*更新网站内部编号web_num*/
    update_long_item(web_num);

    /*根据网站内部编号web_num查找该Website节点*/
    if (NULL == (web = find_web("", "", new->web_num, NULL))) {
        error_data("wrong new web_num!");
        free(new);
        return;
    }

    /*更新店铺编号shop_id*/
    update_string_item(shop_id, ID_MAX);

    /*更新店铺名称shop_name*/
    update_string_item(shop_name, NAME_MAX);

    /*判断店铺编号shop_id或店铺名称shop_name是否与现有的其他数据重复*/
    if (NULL != find_shop(web, new->shop_id, new->shop_name, old)) {
        error_data("this shop record already exists!");
        free(new);
        return;
    }

    /*更新负责人姓名owner*/
    update_string_item(owner, NAME_MAX);

    /*更新负责人所在地区shop_area*/
    update_long_item(shop_area);

    /*更新店铺开户银行bank*/
    update_long_item(bank);

    /*继承原节点指针关系*/
    if (new->web_num == old->web_num) {

        /*若web_num没有改变*/
        new->sub  = old->sub;
        new->next = old->next;

        /*原节点前驱指向新节点*/
        if (NULL != prev_shop(old)) {
            prev_shop(old)->next = new;
        }

        /*若原节点为Shop链首节点*/
        if (old == web->sub) {
            web->sub = new;
        }
    }
    else {

        /*若web_num发生改变，需要加入新的Shop链*/
        new->sub  = old->sub;

        /*原节点前驱指向原节点的后驱*/
        if (NULL != prev_shop(old)) {
            prev_shop(old)->next = old->next;
        }

        /*若原节点为Shop链首节点*/
        if (old_web->sub == old) {
            old_web->sub =  old->next;
        }

        /*将新节点加入目标Website记录的Shop子链表*/
        add_shop_item(web, new);

        /*原节点Website记录的网站店铺总数减一*/
        --(find_web("", "", old->web_num, NULL)->cnt);
    }

    /*更新下属各Order记录的网站内部编号web_num和店铺编号shop_id*/
    struct order_t * order = new->sub;
    while (NULL != order) {
        order->web_num = new->web_num;
        strcpy(order->shop_id, new->shop_id);
        order = order->next;
    }

    /*释放原节点所占用的堆空间*/
    free(old);

    /*向FIFO输出管道写入反馈信息*/
    single_echo('E', sizeof(struct shop_t), new);
}


/*指令F: 更新已有的Order记录*/
void replace_order(char * msg) {
    struct web_t   * web;
    struct shop_t  * old_shop;
    struct shop_t  * shop;
    struct order_t * new;
    struct order_t * old;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong old web_num!");
        return;
    }

    /*提取店铺编号shop_id*/
    get_another_string("shop_id");

    /*根据店铺编号shop_id查找此店铺*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong old shop_id!");
        return;
    }
    old_shop = shop;

    /*提取交易内部编号order_num*/
    get_another_string("order_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据交易内部编号order_num查找此交易*/
    if (NULL == (old = find_order(shop, i, NULL))) {
        error_data("wrong old order_num!");
        return;
    }

    /*为新节点分配空间*/
    new = (struct order_t *) calloc(1, sizeof(struct order_t));

    /*更新网站内部编号web_num*/
    update_long_item(web_num);

    /*根据网站内部编号web_num查找该Website节点*/
    if (NULL == (web = find_web("", "", new->web_num, NULL))) {
        error_data("wrong new web_num!");
        free(new);
        return;
    }

    /*更新店铺编号shop_id*/
    update_string_item(shop_id, NAME_MAX);

    /*根据店铺编号shop_id查找该Shop节点*/
    if (NULL == (shop = find_shop(web, new->shop_id, "", NULL))) {
        error_data("wrong new shop_id!");
        free(new);
        return;
    }

    /*更新支付类型pay*/
    update_long_item(pay);

    /*更新交易金额money*/
    update_float_item(money);

    /*更新交易日期date*/
    update_long_item(date.day);
    update_long_item(date.month);
    update_long_item(date.year);

    /*更新支付人所在地区order_area*/
    update_long_item(order_area);

    /*复制交易内部编号order_num*/
    new->order_num = old->order_num;

    /*更新最近一次修改时间recent*/
    strcpy(new->recent, timestr());

    /*继承原节点指针关系*/
    if ( (new->web_num == old->web_num) &&
         (0 == strcasecmp(new->shop_id, old->shop_id)) ) {

        /*若web_num和shop_id均未发生改变*/
        new->next = old->next;

        /*原节点前驱指向新节点*/
        if (NULL != prev_order(old)) {
            prev_order(old)->next = new;
        }

        /*若原节点为Shop链首节点*/
        if (old == shop->sub) {
            shop->sub = new;
        }
    }
    else {

        /*若web_num或shop_id发生改变，需要加入新的Order链*/
        /*原节点前驱指向原节点的后驱*/
        if (NULL != prev_order(old)) {
            prev_order(old)->next = old->next;
        }

        /*若原节点为Shop链首节点*/
        if (old_shop->sub == old) {
            old_shop->sub =  old->next;
        }

        /*将新节点加入目标Shop记录的Order子链表*/
        add_order_item(shop, new);
    }

    /*释放原节点所占用的堆空间*/
    free(old);

    /*向FIFO输出管道写入反馈信息*/
    single_echo('F', sizeof(struct order_t), new);
}


/*输出调用者(指令): GHI Y*/
void delete_done(char type) {
    int    fd;

    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    write(fd, &type, sizeof(char));
    close(fd);
}


/*从shop节点的Order子链表中删去order节点*/
void free_order(struct shop_t * shop, struct order_t * order) {

    /*order节点前驱指向order节点的后驱*/
    if (NULL != prev_order(order)) {
        prev_order(order)->next = order->next;
    }

    /*若order节点为链表首节点*/
    if (shop->sub == order) {
        shop->sub =  order->next;
    }

    /*释放节点所占用的堆空间*/
    free(order);
}


/*从web节点的Shop子链表中删去shop节点及其附属的各Order节点*/
void free_shop(struct web_t * web, struct shop_t * shop) {
    struct order_t * order;
    struct order_t * next;

    /*遍历删除附属的各Order节点*/
    order = shop->sub;
    while (NULL != order) {
        next  = order->next;
        free_order(shop, order);

        order = next;
    }

    /*Website节点web的网站店铺总数减一*/
    --(web->cnt);

    /*shop节点前驱指向shop节点的后驱*/
    if (NULL != prev_shop(shop)) {
        prev_shop(shop)->next = shop->next;
    }

    /*若shop节点为链表首节点*/
    if (web->sub == shop) {
        web->sub =  shop->next;
    }

    /*释放节点所占用的堆空间*/
    free(shop);
}


/*从Website链表中删去web节点及其附属的各Shop和Order节点*/
void free_web(struct web_t * web) {
    struct shop_t * shop;
    struct shop_t * next;

    /*遍历删除附属的各Shop节点*/
    shop = web->sub;
    while (NULL != shop) {
        next = shop->next;
        free_shop(web, shop);
        shop = next;
    }

    /*若web节点为链表首节点*/
    if (web_head == web) {
        web_head =  web->next;
    }

    /*若web节点为链表尾节点*/
    if (web_tail == web) {
        web_tail =  prev_web(web);
    }

    /*web节点前驱指向web节点的后驱*/
    if (NULL != prev_web(web)) {
        prev_web(web)->next = web->next;
    }

    /*释放节点所占用的堆空间*/
    free(web);
}


/*指令G: 删除已有的Website记录*/
void delete_web(char * msg) {
    struct web_t   * web;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*删除该Website节点及其附属节点*/
    free_web(web);

    /*向FIFO输出管道写入反馈信息*/
    delete_done('G');
}


/*指令H: 删除已有的Shop记录*/
void delete_shop(char * msg) {
    struct web_t  * web;
    struct shop_t * shop;
    char          * head = msg;
    char          * tail;
    long            i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*提取店铺编号shop_id*/
    get_another_string("shop_id");

    /*根据店铺编号shop_id查找此店铺*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    /*删除该Shop节点及其附属节点*/
    free_shop(web, shop);

    /*向FIFO输出管道写入反馈信息*/
    delete_done('H');
}


/*指令I: 删除已有的Order记录*/
void delete_order(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*提取店铺编号shop_id*/
    get_another_string("shop_id");

    /*根据店铺编号shop_id查找此店铺*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    /*提取交易内部编号order_num*/
    get_another_string("order_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据交易内部编号order_num查找此交易*/
    if (NULL == (order = find_order(shop, i, NULL))) {
        error_data("wrong order_num!");
        return;
    }

    /*删除该Order节点*/
    free_order(shop, order);

    /*向FIFO输出管道写入反馈信息*/
    delete_done('I');
}


/*输出调用者(指令): J*/
/*指令J: 列出已有的Website记录*/
void list_web(void) {
    struct web_t * web;
    long           cnt  = 0;
    long           len  = 0;
    int            fd;
    char           buf[URL_MAX];
    char           type = 'J';

    tmpstr[0] = '\0';

    /*遍历Website链表并记录各节点的web_num和web_name域数据*/
    web = web_head;
    while (NULL != web) {
        cnt++;
        sprintf(buf, "%ld\n\0", web->web_num);
        strcat(tmpstr, buf);
        strcat(tmpstr, web->web_name);
        strcat(tmpstr, "\n");
        web = web->next;
    }

    /*设置输出序列*/
    len = strlen(tmpstr);

    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &cnt;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = &len;
    iov[2].iov_len  = sizeof(long);

    iov[3].iov_base = tmpstr;
    iov[3].iov_len  = len;

    /*聚集写入FIFO管道*/
    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    writev(fd, iov, 4);
    close(fd);
}


/*输出调用者(指令): K*/
/*指令K: 列出已有的Shop记录*/
void list_shop(void) {
    struct web_t  * web;
    struct shop_t * shop;
    long            i = 0;
    long            cnt  = 0;
    long            len  = 0;
    int             fd;
    char            buf[URL_MAX];
    char            type = 'K';

    tmpstr[0] = '\0';

    /*遍历Website链表*/
    web = web_head;
    while (NULL != web) {

        /*记录各Website节点的web_num和web_name域数据*/
        cnt++;
        sprintf(buf, "%ld\n\0", web->web_num);
        strcat(tmpstr, buf);
        strcat(tmpstr, web->web_name);
        strcat(tmpstr, "\n");

        /*遍历各Shop子链表*/
        shop = web->sub;
        while (NULL != shop) {

            /*记录各Shop节点的shop_id和shop_name域数据*/
            strcat(tmpstr, shop->shop_id);
            strcat(tmpstr, "\n");
            strcat(tmpstr, shop->shop_name);
            strcat(tmpstr, "\n");

            shop = shop->next;
        }

        web = web->next;
    }

    /*设置输出序列*/
    len = strlen(tmpstr);

    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &cnt;
    iov[1].iov_len  = sizeof(long);

    /*遍历Website链表并记录各Website节点的网站店铺总数*/
    i = 2;
    web = web_head;
    while (NULL != web) {
        iov[i].iov_base = &(web->cnt);
        iov[i].iov_len  = sizeof(long);

        i++;
        web = web->next;
    }

    iov[i].iov_base = &len;
    iov[i].iov_len  = sizeof(long);

    i++;
    iov[i].iov_base = tmpstr;
    iov[i].iov_len  = len;

    i++;

    /*聚集写入FIFO管道*/
    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    writev(fd, iov, i);
    close(fd);
}

/*输出调用者(指令): L*/
/*指令L: 列出指定Shop的所有Order记录*/
void list_order(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    char           * head = msg;
    char           * tail;
    long             i    = 0;
    long             cnt  = 0;
    long             len  = 0;
    int              fd;
    char             type = 'L';

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*提取店铺编号shop_id*/
    get_another_string("shop_id");

    /*根据店铺编号shop_id查找此店铺*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    /*设置输出序列*/
    cnt = 0;
    order = shop->sub;
    while (NULL != order) {
        cnt++;
        order = order->next;
    }

    len = sizeof(struct order_t);

    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &cnt;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = &len;
    iov[2].iov_len  = sizeof(long);

    /*遍历该Shop节点的Order子链表并记录各节点地址*/
    i = 3;
    order = shop->sub;
    while (NULL != order) {
        iov[i].iov_base = order;
        iov[i].iov_len  = len;

        i++;
        order = order->next;
    }

    /*聚集写入FIFO管道*/
    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    writev(fd, iov, i);
    close(fd);
}


/*指令M: 查看已有的Website记录*/
void view_web(char * msg) {
    struct web_t   * web;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*向FIFO输出管道写入查询结果*/
    single_echo('M', sizeof(struct web_t), web);
}


/*指令N: 查看已有的Shop记录*/
void view_shop(char * msg) {
    struct web_t  * web;
    struct shop_t * shop;
    char          * head = msg;
    char          * tail;
    long            i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*提取店铺编号shop_id*/
    get_another_string("shop_id");

    /*根据店铺编号shop_id查找此店铺*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    /*向FIFO输出管道写入查询结果*/
    single_echo('N', sizeof(struct shop_t), shop);
}


/*指令O: 查看已有的Order记录*/
void view_order(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*提取网站内部编号web_num*/
    get_first_string("web_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据网站内部编号web_num查找此网站*/
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*提取店铺编号shop_id*/
    get_another_string("shop_id");

    /*根据店铺编号shop_id查找此店铺*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    /*提取交易内部编号order_num*/
    get_another_string("order_num");
    i = 0;
    sscanf(tmpstr, "%ld", &i);

    /*根据交易内部编号order_num查找此交易*/
    if (NULL == (order = find_order(shop, i, NULL))) {
        error_data("wrong order_num!");
        return;
    }

    /*向FIFO输出管道写入查询结果*/
    single_echo('O', sizeof(struct order_t), order);
}


/*输出调用者(指令): PQRS*/
void state_output(char type, long cnt, long size) {
    int     fd;
    long    i;

    /*设置输出序列*/
    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &cnt;
    iov[1].iov_len  = sizeof(long);

    if (0 != cnt) {
        iov[2].iov_base = &size;
        iov[2].iov_len  = sizeof(long);

        for (i = 0; i < cnt; i++) {
            iov[i + 3].iov_base = queue[i];
            iov[i + 3].iov_len  = size;
        }

        i = cnt + 3;
    }
    else {
        i = 2;
    }

    /*聚集写入FIFO管道*/
    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    writev(fd, iov, i);
    close(fd);
}


/*指令P: 搜索已有的Website记录*/
void search_web(char * msg) {
    struct web_t   * web;
    char           * head  = msg;
    char           * tail;
    long             i     = 0;
    long             cnt   = 0;

    /*提取待查询的关键词*/
    for (tail = strstr(msg, "\n"); head != tail; ++head) {
        tmpstr[i++] = *head;
    }
    tmpstr[i] = '\0';
    clean_str(tmpstr);

    /*遍历Website链表查询并记录web_name或url与关键词匹配的节点*/
    web = web_head;
    while (NULL != web) {
        if ( (NULL != strcasestr(web->web_name, tmpstr)) ||
             (NULL != strcasestr(web->url, tmpstr)) ) {
            queue[cnt] = web;
            cnt++;
        }
        web = web->next;
    }

    /*向FIFO输出管道写入查询结果*/
    state_output('P', cnt, sizeof(struct web_t));
}


/*指令Q: 搜索已有的Shop记录*/
void search_shop(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    char           * head  = msg;
    char           * tail;
    long             i     = 0;
    long             cnt   = 0;

    /*提取待查询的关键词*/
    for (tail = strstr(msg, "\n"); head != tail; ++head) {
        tmpstr[i++] = *head;
    }
    tmpstr[i] = '\0';
    clean_str(tmpstr);

    /*遍历Website链表*/
    web = web_head;
    while (NULL != web) {

        /*遍历各Shop链表查询并记录shop_name、shop_id或owner与关键词匹配的节点*/
        shop = web->sub;
        while (NULL != shop) {
            if ( (NULL != strcasestr(shop->shop_name, tmpstr)) ||
                 (NULL != strcasestr(shop->shop_id, tmpstr))   ||
                 (NULL != strcasestr(shop->owner, tmpstr))     ) {
                queue[cnt] = shop;
                cnt++;
            }
            shop = shop->next;
        }
        web = web->next;
    }

    /*向FIFO输出管道写入查询结果*/
    state_output('Q', cnt, sizeof(struct shop_t));
}


/*指令R: 统计指定年份各店铺的交易次数和金额*/
void state_year(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    char           * head  = msg;
    char           * tail;
    long             i     = 0;
    long             year  = 0;
    long             cnt   = 0;
    struct state_t * state;

    /*提取待查询的年份*/
    for (tail = strstr(msg, "\n"); head != tail; ++head) {
        tmpstr[i++] = *head;
    }
    tmpstr[i] = '\0';
    clean_str(tmpstr);

    if (0 == sscanf(tmpstr, "%ld", &year)) {
        error_data("wrong year!");
        return;
    }

    /*遍历Website链表*/
    web = web_head;
    while (NULL != web) {

        /*遍历Shop子链表*/
        shop = web->sub;
        while (NULL != shop) {

            /*按Shop新建统计报表结构体*/
            state = (struct state_t *) calloc(1, sizeof(struct state_t));
            state->cnt = 0;
            state->sum = 0;

            /*遍历Order子链表*/
            order = shop ->sub;
            while (NULL != order) {

                /*累加相应年份的交易次数和金额*/
                if (order->date.year == year) {
                    ++(state->cnt);
                    state->sum += order->money;
                }
                order = order->next;
            }

            state->web_num = shop->web_num;
            strcpy(state->shop_id, shop->shop_id);
            queue[cnt] = state;

            cnt++;
            shop = shop->next;
        }
        web = web->next;
    }

    /*向FIFO输出管道写入统计结果*/
    state_output('R', cnt, sizeof(struct state_t));
}


/*快速排序，按Order记录数降序排序*/
void quicksort(long l, long r) {
    long    i = l;
    long    j = r;
    void  * tmp;
    long    x = ((struct top5_t *)queue[(l + r) / 2])->cnt;

    do {
        while (((struct top5_t *)queue[i])->cnt > x) {
            i++;
        }

        while (x > ((struct top5_t *)queue[j])->cnt) {
            j--;
        }

        if (i <= j) {
            tmp      = queue[i];
            queue[i] = queue[j];
            queue[j] = tmp;
            i++;
            j--;
        }
    } while (i <= j);

    if (l < j) {
        quicksort(l, j);
    }

    if (i < r) {
        quicksort(i, r);
    }
}


/*指令S: 统计指定月份的网站交易龙虎榜*/
void top_five(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    char           * head  = msg;
    char           * tail;
    long             i     = 0;
    long             year  = 0;
    long             month = 0;
    long             cnt   = 0;
    struct top5_t  * top;

    /*提取待查询的年份*/
    for (tail = strstr(msg, "\n"); head != tail; ++head) {
        tmpstr[i++] = *head;
    }
    tmpstr[i] = '\0';
    clean_str(tmpstr);

    if (0 == sscanf(tmpstr, "%ld", &year)) {
        error_data("wrong year!");
        return;
    }

    /*提取待查询的月份*/
    i = 0;
    ++head;
    for (tail = strstr(tail + 1, "\n"); head != tail; ++head) {
        tmpstr[i++] = *head;
    }
    tmpstr[i] = '\0';
    clean_str(tmpstr);

    if (0 == sscanf(tmpstr, "%ld", &month)) {
        error_data("wrong month!");
        return;
    }

    /*遍历Website链表*/
    web = web_head;
    while (NULL != web) {

        /*按Website新建排名结构体*/
        top = (struct top5_t *) calloc(1, sizeof(struct top5_t));
        top->cnt = 0;

        /*遍历Shop子链表*/
        shop = web->sub;
        while (NULL != shop) {

            /*遍历Order子链表*/
            order = shop ->sub;
            while (NULL != order) {

                /*统计相应年月的交易次数*/
                if ( (order->date.year  == year) &&
                     (order->date.month == month) ) {
                    ++(top->cnt);
                }
                order = order->next;
            }
            shop = shop->next;
        }

        top->web_num = web->web_num;
        strcpy(top->web_name, web->web_name);

        queue[cnt] = top;

        cnt++;
        web = web->next;
    }

    /*对结果按Order记录数降序进行排序*/
    quicksort(0, cnt - 1);

    /*向FIFO输出管道写入统计结果*/
    state_output('S', (cnt > 5) ? 5 : cnt, sizeof(struct top5_t));
}


/*指令T: 统计指定地区的指定月份之贸易关系*/
void trade_balance(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    char           * head   = msg;
    char           * tail;
    long             i      = 0;
    long             year   = 0;
    long             month  = 0;
    long             area_a = 0;
    long             area_b = 0;
    long double      sum    = 0;

    /*提取待查询的年份*/
    get_first_string("year");
    if (0 == sscanf(tmpstr, "%ld", &year)) {
        error_data("wrong year!");
        return;
    }

    /*提取待查询的月份*/
    get_another_string("month");
    if (0 == sscanf(tmpstr, "%ld", &month)) {
        error_data("wrong month!");
        return;
    }

    /*提取待查询的地区A*/
    get_another_string("area_a");
    if (0 == sscanf(tmpstr, "%ld", &area_a)) {
        error_data("wrong area_a!");
        return;
    }

    /*提取待查询的地区B*/
    get_another_string("area_b");
    if (0 == sscanf(tmpstr, "%ld", &area_b)) {
        error_data("wrong area_b!");
        return;
    }

    /*遍历Website链表*/
    web = web_head;
    while (NULL != web) {

        /*遍历Shop子链表*/
        shop = web->sub;
        while (NULL != shop) {

            /*遍历Order子链表*/
            order = shop ->sub;
            while (NULL != order) {

                /*统计指定情况下的贸易收支*/
                if ( (order->date.year  == year) &&
                     (order->date.month == month) ) {
                    if ( (shop->shop_area   == area_a) &&
                         (order->order_area == area_b) ) {
                        sum += order->money;
                    }
                    else if ( (shop->shop_area   == area_b) &&
                              (order->order_area == area_a) ) {
                        sum -= order->money;
                    }
                }

                order = order->next;
            }
            shop = shop->next;
        }
        web = web->next;
    }

    /*向FIFO输出管道写入统计结果*/
    single_echo('T', sizeof(long double), &sum);
}


/*输出调用者(指令): UWX*/
void save_and_load_echo(char type) {
    int     fd;

    /*设置输出序列*/
    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &cntw;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = &cnts;
    iov[2].iov_len  = sizeof(long);

    iov[3].iov_base = &cnto;
    iov[3].iov_len  = sizeof(long);

    /*聚集写入FIFO管道*/
    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    writev(fd, iov, 4);
    close(fd);
}


/*指令U: 将全部数据保存至本地文件*/
void save_data(void) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    int              fd;
    FILE *           fw;
    FILE *           fs;
    FILE *           fo;

    /*计数器清零*/
    cntw = 0;
    cnts = 0;
    cnto = 0;

    /*设置文件模式创建屏蔽字*/
    umask(S_IXUSR | S_IXGRP | S_IXOTH);

    /*创建访问权限合乎要求的一级链表数据文件*/
    fd = open(DATA_FW, O_RDONLY | O_CREAT, FILE_MODE);
    close(fd);

    /*创建访问权限合乎要求的二级链表数据文件*/
    fd = open(DATA_FS, O_RDONLY | O_CREAT, FILE_MODE);
    close(fd);

    /*创建访问权限合乎要求的三级链表数据文件*/
    fd = open(DATA_FO, O_RDONLY | O_CREAT, FILE_MODE);
    close(fd);

    /*用Standard I/O打开刚创建的数据文件*/
    fw = fopen(DATA_FW, "wb");
    fs = fopen(DATA_FS, "wb");
    fo = fopen(DATA_FO, "wb");

    /*遍历Website链表*/
    web = web_head;
    while (NULL != web) {

        /*将该Website节点以二进制的格式写入数据文件*/
        fwrite(web, sizeof(struct web_t), 1, fw);
        ++cntw;

        /*遍历Shop子链表*/
        shop = web->sub;
        while (NULL != shop) {

            /*将该Shop节点以二进制的格式写入数据文件*/
            fwrite(shop, sizeof(struct shop_t), 1, fs);
            ++cnts;

            /*遍历Order子链表*/
            order = shop->sub;
            while (NULL != order) {

                /*将该Order节点以二进制的格式写入数据文件*/
                fwrite(order, sizeof(struct order_t), 1, fo);
                ++cnto;

                order = order->next;
            }

            shop = shop->next;
        }

        web = web->next;
    }

    /*关闭数据文件*/
    fclose(fw);
    fclose(fs);
    fclose(fo);
}


int load_data(void) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    FILE *           fw;
    FILE *           fs;
    FILE *           fo;
    int              n;

    /*计数器清零*/
    cntw = 0;
    cnts = 0;
    cnto = 0;

    /*打开一级链表数据文件*/
    if (NULL == (fw = fopen(DATA_FW, "rb")) ) {
        return(1);
    }

    /*打开二级链表数据文件*/
    if (NULL == (fs = fopen(DATA_FS, "rb")) ) {
        return(1);
    }

    /*打开三级链表数据文件*/
    if (NULL == (fo = fopen(DATA_FO, "rb")) ) {
        return(1);
    }

    /*Website链表初始化清零*/
    web_head    = NULL;
    web_tail    = NULL;
    web_num_max = 0;

    /*新建并读取Website节点*/
    web = (struct web_t *) calloc(1, sizeof(struct web_t));
    n   = fread(web, sizeof(struct web_t), 1, fw);

    /*若一级链表数据文件为空*/
    if (0 == n) {
        free(web);
        return(2);
    }

    while (0 != n) {

        /*初始化Website节点的指针域*/
        web->next = NULL;
        web->sub  = NULL;
        web->cnt  = 0;

        /*更新网站内部编号最大值*/
        if (web->web_num > web_num_max) {
            web_num_max = web->web_num;
        }

        /*将Website节点加入Website链表中*/
        add_web_item(web);
        ++cntw;

        /*新建并读取Website节点*/
        web = (struct web_t *) calloc(1, sizeof(struct web_t));
        n   = fread(web, sizeof(struct web_t), 1, fw);
    }
    free(web);

    /*新建并读取Shop节点*/
    shop = (struct shop_t *) calloc(1, sizeof(struct shop_t));
    n    = fread(shop, sizeof(struct shop_t), 1, fs);

    while (0 != n) {

        /*初始化Shop节点的指针域*/
        shop->sub  = NULL;

        /*将Shop节点加入相应的Shop链表中*/
        add_shop_item(find_web("", "", shop->web_num, NULL), shop);
        ++cnts;

        /*新建并读取Shop节点*/
        shop = (struct shop_t *) calloc(1, sizeof(struct shop_t));
        n    = fread(shop, sizeof(struct shop_t), 1, fs);
    }
    free(shop);

    /*交易内部编号最大值初始化*/
    order_num_max = 0;

    /*新建并读取Order节点*/
    order = (struct order_t *) calloc(1, sizeof(struct order_t));
    n     = fread(order, sizeof(struct order_t), 1, fo);

    while (0 != n) {

        /*更新交易内部编号最大值*/
        if (order->order_num > order_num_max) {
            order_num_max = order->order_num;
        }

        /*将Order节点加入相应的Order链表中*/
        add_order_item(find_shop(find_web("", "", order->web_num, NULL),
                        order->shop_id, "", NULL), order);
        ++cnto;

        /*新建并读取Order节点*/
        order = (struct order_t *) calloc(1, sizeof(struct order_t));
        n     = fread(order, sizeof(struct order_t), 1, fo);
    }
    free(order);

    /*关闭数据文件*/
    fclose(fw);
    fclose(fs);
    fclose(fo);

    return (0);
}


/*输出调用者(指令): V*/
/*指令V: 获取所有Order记录中date.year的最小值和最大值*/
void active_year(void) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    long             year_min = +10000;
    long             year_max = -10000;
    int              fd;
    char             type = 'V';

    /*遍历Website链表*/
    web = web_head;
    while (NULL != web) {

        /*遍历Shop子链表*/
        shop = web->sub;
        while (NULL != shop) {

            /*遍历Order子链表*/
            order = shop ->sub;
            while (NULL != order) {

                /*比较出date.year的最大值*/
                if ( order->date.year > year_max ) {
                    year_max = order->date.year;
                }

                /*比较出date.year的最小值*/
                if ( order->date.year < year_min ) {
                    year_min = order->date.year;
                }

                order = order->next;
            }
            shop = shop->next;
        }
        web = web->next;
    }

    /*若数据不足*/
    if (10000 == year_min) {
        error_data("no order records.");
        return;
    }

    /*设置输出序列*/
    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &year_min;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = &year_max;
    iov[2].iov_len  = sizeof(long);

    /*聚集写入FIFO管道*/
    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    writev(fd, iov, 3);
    close(fd);
}


/*指令W: 重新载入本地数据文件*/
void load_again(char * msg) {
    char    * head = msg;
    char    * tail;
    long      i    = 0;

    /*提取密码*/
    get_first_string("passwd");

    /*若密码正确则载入数据文件，否则报错*/
    if (0 == strcmp(tmpstr, passwd)) {
        load_data();

        /*向FIFO输出管道写入反馈信息*/
        save_and_load_echo('W');
    }
    else {
        error_data("wrong load passwd!");
    }
}


/*指令X: 将全部数据保存至本地文件并关闭Matrix进程*/
void save_shut_passwd(char * msg) {
    char    * head = msg;
    char    * tail;
    long      i    = 0;

    /*提取密码*/
    get_first_string("passwd");

    /*若密码正确则保存数据，否则报错*/
    if (0 == strcmp(tmpstr, passwd)) {
        run  = 0;
        save = 1;
        save_data();

        /*向FIFO输出管道写入反馈信息*/
        save_and_load_echo('X');
    }
    else {
        error_data("wrong save & shutdown passwd!");
    }
}


/*指令Y: 关闭Matrix进程*/
void shut_passwd(char * msg) {
    char    * head = msg;
    char    * tail;
    long      i    = 0;

    /*提取密码*/
    get_first_string("passwd");

    /*若密码正确则将运行变量设置为0，否则报错*/
    if (0 == strcmp(tmpstr, passwd)) {
        run  = 0;
        save = 0;

        /*向FIFO输出管道写入反馈信息*/
        delete_done('Y');
    }
    else {
        error_data("wrong shutdown passwd!");
    }
}


/*从输入的FIFO管道中读取数据存入msg数组*/
char * read_fifo(void) {
    int    fd;
    long   len;

    fd  = open(FIFO_IN, O_RDONLY);
    len = read(fd, msg, MSG_MAX);
    close(fd);

    return (msg);
}


/*SIGALRM信号处理*/
static void time_limit(int signo) {
    /*删除FIFO管道*/
    delete_fifo();

    /*向syslogd输出警告信息*/
    syslog(LOG_WARNING, "%s. time is up.", timestr());

    /*立即返回内核，避免因Standard I/O缓冲冲洗造成的错误*/
    _exit(11);
}


/*对文件描述符为fd的文件整体加锁*/
int lockfile(int fd) {
    struct flock fl;

    fl.l_type   = F_WRLCK;
    fl.l_start  = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len    = 0;

    return(fcntl(fd, F_SETLK, &fl));
}


/*确保当前只有一个进程的副本正在运行*/
int already_running(void) {
    int     fd;
    char    buf[16];

    fd = open(LOCK_FILE, O_RDWR | O_CREAT, LOCK_MODE);

    if (fd < 0) {
        syslog(LOG_ERR, "can't open %s: %s", LOCK_FILE, strerror(errno));
        exit(8);
    }

    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return(1);
        }
        syslog(LOG_ERR, "can't lock %s: %s", LOCK_FILE, strerror(errno));
        exit(9);
    }

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf)+1);
    return (0);
}


/*主函数*/
int main(int argc, char** argv) {
    struct sigaction    act, oact;

    /*初始化为守护进程*/
    if (0 != daemon(1, 1)) {
        perror(NULL);
        exit(10);
    }

    /*参数初始化与syslogd守护进程日志信息的类型*/
    openlog("iShopMatrix", LOG_PERROR, 0);

    /*确保当前只有一个进程的副本正在运行*/
    if (1 == already_running()) {
        syslog(LOG_ERR, "this daemon already running!");
        exit(7);
    }

    /*设置信号动作act*/
    act.sa_handler = time_limit;
    act.sa_flags   = SA_INTERRUPT;
    sigemptyset(&act.sa_mask);

    /*使SIGALRM信号与信号动作act关联*/
    if (sigaction(SIGALRM, &act, &oact) < 0) {
        syslog(LOG_ERR, "%m");
        exit(6);
    }

    /*创建FIFO管道*/
    create_fifo();

    /*载入数据文件，并重建三级十字交叉链表*/
    load_data();

    do {

        /*从输入的FIFO管道中读取指令*/
        read_fifo();

        /*对指令根据首字母分类处理*/
        switch (msg[0]) {
            case 'A' :
                add_web(msg + 1);
                break;
            case 'B' :
                add_shop(msg + 1);
                break;
            case 'C' :
                add_order(msg + 1);
                break;
            case 'D' :
                replace_web(msg + 1);
                break;
            case 'E' :
                replace_shop(msg + 1);
                break;
            case 'F' :
                replace_order(msg + 1);
                break;
            case 'G' :
                delete_web(msg + 1);
                break;
            case 'H' :
                delete_shop(msg + 1);
                break;
            case 'I' :
                delete_order(msg + 1);
                break;
            case 'J' :
                list_web();
                break;
            case 'K' :
                list_shop();
                break;
            case 'L' :
                list_order(msg + 1);
                break;
            case 'M' :
                view_web(msg + 1);
                break;
            case 'N' :
                view_shop(msg + 1);
                break;
            case 'O' :
                view_order(msg + 1);
                break;
            case 'P' :
                search_web(msg + 1);
                break;
            case 'Q' :
                search_shop(msg + 1);
                break;
            case 'R' :
                state_year(msg + 1);
                break;
            case 'S' :
                top_five(msg + 1);
                break;
            case 'T' :
                trade_balance(msg + 1);
                break;
            case 'U' :
                save_data();
                save_and_load_echo('U');
                break;
            case 'V' :
                active_year();
                break;
            case 'W' :
                load_again(msg + 1);
                break;
            case 'X' :
                save_shut_passwd(msg + 1);
                break;
            case 'Y' :
                shut_passwd(msg + 1);
                break;
            default :
                error_data("no such instruction!");
                break;
        }

        /*设置等待时间为WAIT_SEC秒*/
        alarm(WAIT_SEC);
    } while (run);

    /*向syslogd守护进程发送LOG_NOTICE通知信息，说明关闭原因*/
    if (1 == save) {
        syslog(LOG_NOTICE, "%s. shutdown and saved by passwd.", timestr());
    }
    else {
        syslog(LOG_NOTICE, "%s. shutdown by passwd.", timestr());
    }

    /*删除FIFO管道*/
    delete_fifo();

    /*程序正常结束*/
    return (0);
}



#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>


#define TMPFILE     "/tmp/iShopTmpfile"                     /*分析反馈的临时文件*/
#define FIFO_IN     "/tmp/iShopFIFOinput"                   /*FIFO输入管道*/
#define FIFO_OUT    "/tmp/iShopFIFOoutput"                  /*FIFO输出管道*/
#define ID_MAX      22                                      /*ID类字符串长度*/
#define NAME_MAX    32                                      /*名称类字符串长度*/
#define URL_MAX     52                                      /*URL字符串长度*/
#define RECENT_MAX  42                                      /*RECENT字符串长度*/
#define TMPSTR_MAX  102400                                  /*临时字符串长度*/
#define MSG_MAX     102400                                  /*输入指令串长度*/
#define WEB_MAX     1024                                    /*一级链表最大节点数*/
#define SHOP_MAX    1024                                    /*二级链表最大节点数*/
#define ORDER_MAX   1024                                    /*三级链表最大节点数*/


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


char * month_list [] = {
    NULL,
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
    NULL
};

char * month_list_abbr [] = {
    NULL,
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sept",
    "Oct",
    "Nov",
    "Dec",
    NULL
};

extern int     h_errno;                     /*sockets用错误变量，在netdb.h中定义*/
extern char ** environ;                     /*环境表指针*/

/*针对GNU特有的strcasestr()的函数原型之声明*/
/*warning: strcasestr is a GNU specific extension to the C standard.*/
char *strcasestr(const char *haystack, const char *needle);

int  unix_error(char * msg);
void Execve(const char * filename, char * const argv[], char * const envp[]);
void recovery_url(char * str);

const char * m0 = "[[0]]";
const char * m1 = "[[1]]";
const char * m2 = "[[2]]";
const char * m3 = "[[3]]";
const char * m4 = "[[4]]";
const char * m5 = "[[5]]";
const char * m6 = "[[6]]";
const char * m7 = "[[7]]";
const char * m8 = "[[8]]";
const char * m9 = "[[9]]";


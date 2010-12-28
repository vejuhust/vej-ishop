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


#define LOCK_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FILE_MODE   (S_IRUSR | S_IWUSR | 0*S_IRGRP | 0*S_IROTH)
#define LOCK_FILE   "/var/run/iShopDaemon.pid"
#define FIFO_IN     "/tmp/iShopFIFOinput"
#define FIFO_OUT    "/tmp/iShopFIFOoutput"
#define DATA_FW     "iShopWebData"
#define DATA_FS     "iShopShopData"
#define DATA_FO     "iShopOrderData"
#define ID_MAX      22
#define NAME_MAX    32
#define URL_MAX     52
#define RECENT_MAX  42
#define TMPSTR_MAX  102400
#define MSG_MAX     102400
#define WEB_MAX     1024
#define SHOP_MAX    1024
#define WRITEV_MAX  1024
#define WAIT_SEC    1800

extern int errno;

char  * passwd         = "19920506";
long    run            = 1;
long    save           = 1;

long    web_num_max    = 0;
long    order_num_max  = 0;

long    cntw           = 0;
long    cnts           = 0;
long    cnto           = 0;

struct web_t       * web_head   = NULL;
struct web_t       * web_tail   = NULL;

struct date_t {
    long             day;
    long             month;
    long             year;
};

struct web_t {
    char             web_name  [NAME_MAX];
    char             url       [URL_MAX];
    long             web_num;
    long             cnt;
    struct web_t   * next;
    struct shop_t  * sub;
};

struct shop_t {
    long             web_num;
    char             shop_id   [ID_MAX];
    char             shop_name [NAME_MAX];
    char             owner     [NAME_MAX];
    long             shop_area;
    long             bank;
    struct shop_t  * next;
    struct order_t * sub;
};

struct order_t {
    long             web_num;
    char             shop_id   [ID_MAX];
    long             order_num;
    long             pay;
    long double      money;
    struct date_t    date;
    long             order_area;
    char             recent    [RECENT_MAX];
    struct order_t * next;
};

struct state_t {
    long             web_num;
    char             shop_id   [ID_MAX];
    long             cnt;
    long double      sum;
};

struct top5_t {
    long             web_num;
    char             web_name  [NAME_MAX];
    long             cnt;
};

char * area_list[] = {
    NULL,
    "Anhui",                                    /*01*/
    "Beijing",                                  /*02*/
    "Chongqing",                                /*03*/
    "Fujian",                                   /*04*/
    "Gansu",                                    /*05*/
    "Guangdong",                                /*06*/
    "Guangxi",                                  /*07*/
    "Guizhou",                                  /*08*/
    "Hainan",                                   /*09*/
    "Hebei",                                    /*10*/
    "Heilongjiang",                             /*11*/
    "Henan",                                    /*12*/
    "Hong Kong",                                /*13*/
    "Hubei",                                    /*14*/
    "Hunan",                                    /*15*/
    "Inner Mongolia",                           /*16*/
    "Jiangsu",                                  /*17*/
    "Jiangxi",                                  /*18*/
    "Jilin",                                    /*19*/
    "Liaoning",                                 /*20*/
    "Macau",                                    /*21*/
    "Ningxia",                                  /*22*/
    "Qinghai",                                  /*23*/
    "Shaanxi",                                  /*24*/
    "Shandong",                                 /*25*/
    "Shanghai",                                 /*26*/
    "Shanxi",                                   /*27*/
    "Sichuan",                                  /*28*/
    "Taiwan",                                   /*29*/
    "Tianjin",                                  /*30*/
    "Tibet",                                    /*31*/
    "Xinjiang",                                 /*32*/
    "Yunnan",                                   /*33*/
    "Zhejiang",                                 /*34*/
    NULL
};

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
    "Industrial and Commercial Bank of China",	/*24*/
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

char * pay_list[] = {
    NULL,
    "Credit Card",                              /*01*/
    "Gift Card",                                /*02*/
    "Bank Account",                             /*03*/
    "Cash on Delivery",                         /*04*/
    NULL
};

char         tmpstr [TMPSTR_MAX];
char         msg    [MSG_MAX];

void       * queue  [WEB_MAX];

struct iovec iov    [WRITEV_MAX];

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


/*warning: strcasestr is a GNU specific extension to the C standard.*/
char *strcasestr(const char *haystack, const char *needle);


void create_fifo(void) {
    if ((mkfifo(FIFO_IN, FILE_MODE) < 0) && (EEXIST != errno)) {
        //perror(FIFO_IN);
        syslog(LOG_ERR, "%m");
        exit(1);
    }

    if ((mkfifo(FIFO_OUT, FILE_MODE) < 0) && (EEXIST != errno)) {
        unlink(FIFO_IN);
        //perror(FIFO_OUT);
        syslog(LOG_ERR, "%m");
        exit(2);
    }
}


void delete_fifo(void) {
    if (0 > unlink(FIFO_IN)) {
        //perror(FIFO_IN);
        syslog(LOG_ERR, "%m");
        exit(3);
    }

    if (0 > unlink(FIFO_OUT)) {
        //perror(FIFO_OUT);
        syslog(LOG_ERR, "%m");
        exit(4);
    }
}


int app_error(char * msg) {
    //fprintf(stderr, "%s\n", msg);
    syslog(LOG_ERR, "%s", msg);
    exit(5);
}


char * clean_str(char * str) {
    char * head = NULL;
    char * tail = str;
    char * old  = str;

    if ('\0' == *str) {
        return(NULL);
    }

    for (; '\0' != *tail; ++tail) {
        if (('\t' != *tail) && (' ' != *tail)) {
            head = tail;
            break;
        }
    }

    if (NULL == head) {
        *str = '\0';
        return(NULL);
    }

    do {
        ++tail;
    } while ('\0' != *tail);

    for (--tail; head != tail; --tail) {
        if (('\t' != *tail) && (' ' != *tail)) {
            break;
        }
    }

    for (++tail; head != tail; ++head, ++str) {
        if ('\t' == *head) {
            *str = ' ';
        }
        else {
            *str = *head;
        }
    }
    *str = '\0';

    return (old);
}


char * timestr(void) {
    static int      cnt = 0;
    char          * str;
    struct timeval  t1;
    struct tm     * t2;

    gettimeofday(&t1, NULL);
    t2 = localtime(&t1.tv_sec);
    str = (char *) calloc(100, sizeof(char));

    sprintf(str, "%.5d_%4d-%.2d-%.2d %.2d:%.2d:%.2d.%.6ld", ++cnt,
                t2->tm_year + 1900, t2->tm_mon + 1, t2->tm_mday,
                t2->tm_hour, t2->tm_min, t2->tm_sec, (long) t1.tv_usec);

    return str;
}


/*output: ABC DEF MNO T*/
void single_echo(char type, long len, void * aim) {
    int    fd;

/*
    long   len;

    switch (type) {
        case 'A' :
        case 'D' :
            len = sizeof(struct web_t);
            break;
        case 'B' :
        case 'E' :
            len = sizeof(struct shop_t);
            break;
        case 'C' :
        case 'F' :
            len = sizeof(struct order_t);
            break;
    }
*/
    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &len;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = aim;
    iov[2].iov_len  = len;

    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);

    writev(fd, iov, 3);

    close(fd);
}

/*output: every*/
void error_data(char * msg) {
    int    fd;
    long   len;
    char   type = 'Z';
    
    len = strlen(msg);

    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &len;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = msg;
    iov[2].iov_len  = len;

    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);

    writev(fd, iov, 3);

    close(fd);
}


struct web_t * find_web(char * web_name, char * url, long web_num, void * not) {
    struct web_t * web;

    web = web_head;
    while (NULL != web) {
        if ( ( (0 == strcasecmp(web->web_name, web_name)) ||
               (0 == strcasecmp(web->url, url)) ||
               (web->web_num == web_num) ) && (web != not) ) {
            return (web);
        }
        web = web->next;
    }

    return (NULL);
}


struct shop_t * find_shop(struct web_t * web, char * shop_id,
                                              char * shop_name, void * not) {
    struct shop_t * shop;

    shop = web->sub;
    while (NULL != shop) {
        if ( ( (0 == strcasecmp(shop->shop_id, shop_id)) ||
               (0 == strcasecmp(shop->shop_name, shop_name)) )
             && (shop != not ) ) {
            return (shop);
        }
        shop = shop->next;
    }

    return (NULL);
}


struct order_t * find_order(struct shop_t * shop, long order_num, void * not) {
    struct order_t * order;

    order = shop->sub;
    while (NULL != order) {
        if ( ( order->order_num == order_num ) && (order != not ) ) {
            return (order);
        }
        order = order->next;
    }

    return (NULL);
}



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


void add_shop_item(struct web_t * web, struct shop_t * new) {
    new->next = web->sub;
    web->sub  = new;
    ++(web->cnt);
}


void add_order_item(struct shop_t * shop, struct order_t * new) {
    new->next = shop->sub;
    shop->sub = new;
}


/*A*/
void add_web(char * msg) {
    struct web_t * new;
    char         * head = msg;
    char         * tail;
    long           i    = 0;

    /*get web_name*/
    get_first_string("web_name");
    tmpstr[NAME_MAX - 1] = '\0';

    /*store web_name*/
    new = (struct web_t *) calloc(1, sizeof(struct web_t));
    strcpy(new->web_name, tmpstr);

    /*get url*/
    get_another_string("url");
    tmpstr[URL_MAX - 1] = '\0';

    /*store url*/
    strcpy(new->url, tmpstr);

    /*check web_name, url*/
    if (NULL != find_web(new->web_name, new->url, 0, NULL)) {
        error_data("this website record already exists!");
        free(new);
        return;
    }

    /*new web_num*/
    new->web_num = ++web_num_max;

    /*pointers*/
    new->next = NULL;
    new->sub  = NULL;
    new->cnt  = 0;

    /*add to the chain*/
    add_web_item(new);

    single_echo('A', sizeof(struct web_t), new);
}


/*B*/
void add_shop(char * msg) {
    struct web_t  * web;
    struct shop_t * new;
    char          * head = msg;
    char          * tail;
    long            i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*store web_num*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }
    new = (struct shop_t *) calloc(1, sizeof(struct shop_t));
    new->web_num = i;

    /*get shop_id*/
    get_another_string("shop_id");
    tmpstr[ID_MAX - 1] = '\0';

    /*store shop_id*/
    strcpy(new->shop_id, tmpstr);

    /*get shop_name*/
    get_another_string("shop_name");
    tmpstr[NAME_MAX - 1] = '\0';

    /*store shop_name*/
    strcpy(new->shop_name, tmpstr);

    /*check shop_id, shop_name*/
    if (NULL != find_shop(web, new->shop_id, new->shop_name, NULL)) {
        error_data("this shop record already exists!");
        free(new);
        return;
    }

    /*get owner*/
    get_another_string("owner");
    tmpstr[NAME_MAX - 1] = '\0';

    /*store owner*/
    strcpy(new->owner, tmpstr);

    /*get shop_area*/
    get_another_string("shop_area");

    /*store shop_area*/
    i = 0;
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong shop_area!");
        free(new);
        return;
    }
    new->shop_area = i;

    /*get bank*/
    get_another_string("bank");

    /*store bank*/
    i = 0;
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong bank!");
        free(new);
        return;
    }
    new->bank = i;

    /*pointers*/
    new->sub  = NULL;

    /*add to the chain*/
    add_shop_item(web, new);

    single_echo('B', sizeof(struct shop_t), new);
}


/*C*/
void add_order(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * new;
    char           * head  = msg;
    char           * tail;
    long             i     = 0;
    long double      money = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*store web_num*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }
    new = (struct order_t *) calloc(1, sizeof(struct order_t));
    new->web_num = i;

    /*get shop_id*/
    get_another_string("shop_id");
    tmpstr[ID_MAX - 1] = '\0';

    /*store shop_id*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        free(new);
        return;
    }
    strcpy(new->shop_id, tmpstr);

    /*get pay*/
    get_another_string("payment");

    /*store pay*/
    i = 0;
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong payment!");
        free(new);
        return;
    }
    new->pay = i;

    /*get money*/
    get_another_string("money");

    /*store money*/
    money = 0;
    if (0 == sscanf(tmpstr, "%Lf", &money)) {
        error_data("wrong money!");
        free(new);
        return;
    }
    new->money = money;

    /*get and store date*/
    get_another_string("day");
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong day!");
        free(new);
        return;
    }
    new->date.day = i;

    get_another_string("month");
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong month!");
        free(new);
        return;
    }
    new->date.month = i;

    get_another_string("year");
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong year!");
        free(new);
        return;
    }
    new->date.year = i;

    /*get order_area*/
    get_another_string("order_area");

    /*store shop_area*/
    i = 0;
    if (0 == sscanf(tmpstr, "%ld", &i)) {
        error_data("wrong order_area!");
        free(new);
        return;
    }
    new->order_area = i;

    /*new order_num*/
    new->order_num = ++order_num_max;

    /*set recent modified time*/
    strcpy(new->recent, timestr());

    /*add to the chain*/
    add_order_item(shop, new);

    single_echo('C', sizeof(struct order_t), new);
}


struct web_t * prev_web(struct web_t * aim) {
    struct web_t * tmp;

    if (aim == (tmp = web_head) ) {
        return (NULL);
    }

    while (NULL != tmp) {
        if (tmp->next == aim) {
            return (tmp);
        }
        tmp = tmp->next;
    }

    return (NULL);
}


struct shop_t * prev_shop(struct shop_t * aim) {
    struct shop_t * tmp;

    if (aim == (tmp = find_web("","",aim->web_num, NULL)->sub) ) {
        return (NULL);
    }

    while (NULL != tmp) {
        if (tmp->next == aim) {
            return (tmp);
        }
        tmp = tmp->next;
    }

    return (NULL);
}


struct order_t * prev_order(struct order_t * aim) {
    struct order_t * tmp;

    if (aim == (tmp = find_shop(find_web("", "", aim->web_num, NULL),
                                aim->shop_id, "", NULL)->sub) ) {
        return (NULL);
    }

    while (NULL != tmp) {
        if (tmp->next == aim) {
            return (tmp);
        }
        tmp = tmp->next;
    }

    return (NULL);
}


/*D*/
void replace_web(char * msg) {
    struct web_t * new;
    struct web_t * old;
    char         * head = msg;
    char         * tail;
    long           i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (old = find_web("", "", i, NULL))) {
        error_data("wrong old web_num!");
        return;
    }

    new = (struct web_t *) calloc(1, sizeof(struct web_t));

    /*update web_name*/
    update_string_item(web_name, NAME_MAX);

    /*update url*/
    update_string_item(url, URL_MAX);

    /*check web_name, url*/
    if (NULL != find_web(new->web_name, new->url, 0, old)) {
        error_data("this website record already exists!");
        free(new);
        return;
    }

    /*copy web_num*/
    new->web_num = old->web_num;

    /*copy cnt*/
    new->cnt  = old->cnt;

    /*pointers*/
    new->next = old->next;
    new->sub  = old->sub;

    if (NULL != prev_web(old)) {
        prev_web(old)->next = new;
    }

    if (old == web_head) {
        web_head = new;
    }
    if (old == web_tail) {
        web_tail = new;
    }

    /*free old*/
    free(old);

    single_echo('D', sizeof(struct web_t), new);
}


/*E*/
void replace_shop(char * msg) {
    struct web_t  * old_web;
    struct web_t  * web;
    struct shop_t * new;
    struct shop_t * old;
    char          * head = msg;
    char          * tail;
    long            i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong old web_num!");
        return;
    }
    old_web = web;

    /*get shop_id*/
    get_another_string("shop_id");

    /*find the shop*/
    if (NULL == (old = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong old shop_id!");
        return;
    }

    new = (struct shop_t *) calloc(1, sizeof(struct shop_t));

    /*update web_num*/
    update_long_item(web_num);

    /*check the new web*/
    if (NULL == (web = find_web("", "", new->web_num, NULL))) {
        error_data("wrong new web_num!");
        free(new);
        return;
    }

    /*update shop_id*/
    update_string_item(shop_id, ID_MAX);

    /*update shop_name*/
    update_string_item(shop_name, NAME_MAX);

    /*check shop_id, shop_name*/
    if (NULL != find_shop(web, new->shop_id, new->shop_name, old)) {
        error_data("this shop record already exists!");
        free(new);
        return;
    }

    /*update owner*/
    update_string_item(owner, NAME_MAX);

    /*update shop_area*/
    update_long_item(shop_area);

    /*update bank*/
    update_long_item(bank);

    /*pointers*/
    if (new->web_num == old->web_num) {
        new->sub  = old->sub;
        new->next = old->next;

        if (NULL != prev_shop(old)) {
            prev_shop(old)->next = new;
        }

        if (old == web->sub) {
            web->sub = new;
        }
    }
    else {
        new->sub  = old->sub;

        if (NULL != prev_shop(old)) {
            prev_shop(old)->next = old->next;
        }

        if (old_web->sub == old) {
            old_web->sub =  old->next;
        }

        add_shop_item(web, new);

        --(find_web("", "", old->web_num, NULL)->cnt);
    }

    struct order_t * order = new->sub;
    while (NULL != order) {
        order->web_num = new->web_num;
        strcpy(order->shop_id, new->shop_id);
        order = order->next;
    }
    

    /*free old*/
    free(old);

    single_echo('E', sizeof(struct shop_t), new);
}


/*F*/
void replace_order(char * msg) {
    struct web_t   * web;
    struct shop_t  * old_shop;
    struct shop_t  * shop;
    struct order_t * new;
    struct order_t * old;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong old web_num!");
        return;
    }

    /*get shop_id*/
    get_another_string("shop_id");

    /*find the shop*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong old shop_id!");
        return;
    }
    old_shop = shop;

    /*get order_id*/
    get_another_string("order_id");

    /*find the order*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (old = find_order(shop, i, NULL))) {
        error_data("wrong old order_num!");
        return;
    }

    new = (struct order_t *) calloc(1, sizeof(struct order_t));

    /*update web_num*/
    update_long_item(web_num);

    /*check the new web*/
    if (NULL == (web = find_web("", "", new->web_num, NULL))) {
        error_data("wrong new web_num!");
        free(new);
        return;
    }

    /*update shop_id*/
    update_string_item(shop_id, NAME_MAX);

    /*check shop_id*/
    if (NULL == (shop = find_shop(web, new->shop_id, "", NULL))) {
        error_data("wrong new shop_id!");
        free(new);
        return;
    }

    /*update pay*/
    update_long_item(pay);

    /*update money*/
    update_float_item(money);

    /*update date*/
    update_long_item(date.day);
    update_long_item(date.month);
    update_long_item(date.year);

    /*update order_area*/
    update_long_item(order_area);

    /*copy order_num*/
    new->order_num = old->order_num;

    /*update recent modified time*/
    strcpy(new->recent, timestr());

    /*pointers*/
    if ( (new->web_num == old->web_num) &&
         (0 == strcasecmp(new->shop_id, old->shop_id)) ) {
        new->next = old->next;

        if (NULL != prev_order(old)) {
            prev_order(old)->next = new;
        }

        if (old == shop->sub) {
            shop->sub = new;
        }
    }
    else {
        if (NULL != prev_order(old)) {
            prev_order(old)->next = old->next;
        }

        if (old_shop->sub == old) {
            old_shop->sub =  old->next;
        }

        add_order_item(shop, new);
    }

    /*free old*/
    free(old);

    single_echo('F', sizeof(struct order_t), new);
}


/*output: GHI Y*/
void delete_done(char type) {
    int    fd;

    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);

    write(fd, &type, sizeof(char));
    
    close(fd);
}


void free_order(struct shop_t * shop, struct order_t * order) {
    if (NULL != prev_order(order)) {
        prev_order(order)->next = order->next;
    }

    if (shop->sub == order) {
        shop->sub =  order->next;
    }

    free(order);
}


void free_shop(struct web_t * web, struct shop_t * shop) {
    struct order_t * order;
    struct order_t * next;

    order = shop->sub;
    while (NULL != order) {
        next  = order->next;
        free_order(shop, order);

        order = next;
    }

    --(web->cnt);

    if (NULL != prev_shop(shop)) {
        prev_shop(shop)->next = shop->next;
    }

    if (web->sub == shop) {
        web->sub =  shop->next;
    }

    free(shop);
}


void free_web(struct web_t * web) {
    struct shop_t * shop;
    struct shop_t * next;

    shop = web->sub;
    while (NULL != shop) {
        next = shop->next;
        free_shop(web, shop);
        shop = next;
    }

    if (web_head == web) {
        web_head =  web->next;
    }

    if (web_tail == web) {
        web_tail =  prev_web(web);
    }

    if (NULL != prev_web(web)) {
        prev_web(web)->next = web->next;
    }

    free(web);
}


/*G*/
void delete_web(char * msg) {
    struct web_t   * web;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*delete it and its children*/
    free_web(web);

    delete_done('G');
}


/*H*/
void delete_shop(char * msg) {
    struct web_t  * web;
    struct shop_t * shop;
    char          * head = msg;
    char          * tail;
    long            i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*get shop_id*/
    get_another_string("shop_id");

    /*find the shop*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    /*delete it and its children*/
    free_shop(web, shop);

    delete_done('H');
}


/*I*/
void delete_order(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*get shop_id*/
    get_another_string("shop_id");

    /*find the shop*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    /*get order_id*/
    get_another_string("order_id");

    /*find the order*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (order = find_order(shop, i, NULL))) {
        error_data("wrong order_num!");
        return;
    }

    /*delete this one*/
    free_order(shop, order);

    delete_done('I');
}


/*output: J*/
/*J*/
void list_web(void) {
    struct web_t * web;
    long           cnt  = 0;
    long           len  = 0;
    int            fd;
    char           buf[URL_MAX];
    char           type = 'J';

    tmpstr[0] = '\0';

    web = web_head;
    while (NULL != web) {
        cnt++;
        sprintf(buf, "%ld\n\0", web->web_num);
        strcat(tmpstr, buf);
        strcat(tmpstr, web->web_name);
        strcat(tmpstr, "\n");
        web = web->next;
    }

    len = strlen(tmpstr);

    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &cnt;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = &len;
    iov[2].iov_len  = sizeof(long);

    iov[3].iov_base = tmpstr;
    iov[3].iov_len  = len;
    

    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);

    writev(fd, iov, 4);

    close(fd);
}

/*output: K*/
/*K*/
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

    web = web_head;
    while (NULL != web) {
        cnt++;
        sprintf(buf, "%ld\n\0", web->web_num);
        strcat(tmpstr, buf);
        strcat(tmpstr, web->web_name);
        strcat(tmpstr, "\n");

        shop = web->sub;
        while (NULL != shop) {
            strcat(tmpstr, shop->shop_id);
            strcat(tmpstr, "\n");
            strcat(tmpstr, shop->shop_name);
            strcat(tmpstr, "\n");

            shop = shop->next;
        }

        web = web->next;
    }

    len = strlen(tmpstr);


    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &cnt;
    iov[1].iov_len  = sizeof(long);

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

    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);
    
    writev(fd, iov, i);

    close(fd);
}

/*output: L*/
/*L*/
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

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*get shop_id*/
    get_another_string("shop_id");

    /*find the shop*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    /*start writing*/
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

    i = 3;
    order = shop->sub;
    while (NULL != order) {
        iov[i].iov_base = order;
        iov[i].iov_len  = len;

        i++;
        order = order->next;
    }


    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);

    writev(fd, iov, i);

    close(fd);
}

#if 0
/*output: MNO*/
void view_output(char type, long len, void * aim) {
    int  fd;

    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &len;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = aim;
    iov[2].iov_len  = len;

    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);

    writev(fd, iov, 3);

    close(fd);
}
#endif

/*M*/
void view_web(char * msg) {
    struct web_t   * web;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    single_echo('M', sizeof(struct web_t), web);
}


/*N*/
void view_shop(char * msg) {
    struct web_t  * web;
    struct shop_t * shop;
    char          * head = msg;
    char          * tail;
    long            i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*get shop_id*/
    get_another_string("shop_id");

    /*find the shop*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    single_echo('N', sizeof(struct shop_t), shop);
}


/*O*/
void view_order(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    char           * head = msg;
    char           * tail;
    long             i    = 0;

    /*get web_num*/
    get_first_string("web_num");

    /*find the web*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (web = find_web("", "", i, NULL))) {
        error_data("wrong web_num!");
        return;
    }

    /*get shop_id*/
    get_another_string("shop_id");

    /*find the shop*/
    if (NULL == (shop = find_shop(web, tmpstr, "", NULL))) {
        error_data("wrong shop_id!");
        return;
    }

    /*get order_id*/
    get_another_string("order_id");

    /*find the order*/
    i = 0;
    sscanf(tmpstr, "%ld", &i);
    if (NULL == (order = find_order(shop, i, NULL))) {
        error_data("wrong order_num!");
        return;
    }

    single_echo('O', sizeof(struct order_t), order);
}


/*output: PQRST*/
void state_output(char type, long cnt, long size) {
    int     fd;
    long    i;

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

    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);

    writev(fd, iov, i);
  
    close(fd);
}


/*P*/
void search_web(char * msg) {
    struct web_t   * web;
    char           * head  = msg;
    char           * tail;
    long             i     = 0;
    long             cnt   = 0;

    for (tail = strstr(msg, "\n"); head != tail; ++head) {
        tmpstr[i++] = *head;
    }
    tmpstr[i] = '\0';

    clean_str(tmpstr);

    web = web_head;
    while (NULL != web) {
        if ( (NULL != strcasestr(web->web_name, tmpstr)) ||
             (NULL != strcasestr(web->url, tmpstr)) ) {
            queue[cnt] = web;
            cnt++;
        }
        web = web->next;
    }

    state_output('P', cnt, sizeof(struct web_t));
}


/*Q*/
void search_shop(char * msg) {
    struct web_t   * web;
    struct shop_t  * shop;
    char           * head  = msg;
    char           * tail;
    long             i     = 0;
    long             cnt   = 0;

    for (tail = strstr(msg, "\n"); head != tail; ++head) {
        tmpstr[i++] = *head;
    }
    tmpstr[i] = '\0';

    clean_str(tmpstr);

    web = web_head;
    while (NULL != web) {
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

    state_output('Q', cnt, sizeof(struct shop_t));
}


/*R*/
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

    for (tail = strstr(msg, "\n"); head != tail; ++head) {
        tmpstr[i++] = *head;
    }
    tmpstr[i] = '\0';

    clean_str(tmpstr);

    if (0 == sscanf(tmpstr, "%ld", &year)) {
        error_data("wrong year!");
        return;
    }

    web = web_head;
    while (NULL != web) {
        shop = web->sub;
        while (NULL != shop) {
            state = (struct state_t *) calloc(1, sizeof(struct state_t));

            state->cnt = 0;
            state->sum = 0;

            order = shop ->sub;
            while (NULL != order) {
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

    state_output('R', cnt, sizeof(struct state_t));
}

/*
static int cntcmp(const void *p1, const void *p2) {
    long a = ((struct top5_t *)p1)->cnt;
    long b = ((struct top5_t *)p2)->cnt;

    if ( a < b ) {
        return (1);
    }
    else {
        if (a > b ) {
            return (-1);
        }
        else {
            return (0);
        }
    }
}*/


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

/*S*/
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

    /*get year*/
    for (tail = strstr(msg, "\n"); head != tail; ++head) {
        tmpstr[i++] = *head;
    }
    tmpstr[i] = '\0';

    clean_str(tmpstr);

    if (0 == sscanf(tmpstr, "%ld", &year)) {
        error_data("wrong year!");
        return;
    }

    /*get month*/
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

    web = web_head;
    while (NULL != web) {

        top = (struct top5_t *) calloc(1, sizeof(struct top5_t));
        top->cnt = 0;

        shop = web->sub;
        while (NULL != shop) {
            order = shop ->sub;
            while (NULL != order) {
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

    quicksort(0, cnt - 1);

    state_output('S', (cnt > 5) ? 5 : cnt, sizeof(struct top5_t));
}


/*T*/
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


    get_first_string("year");
    if (0 == sscanf(tmpstr, "%ld", &year)) {
        error_data("wrong year!");
        return;
    }

    get_another_string("month");
    if (0 == sscanf(tmpstr, "%ld", &month)) {
        error_data("wrong month!");
        return;
    }

    get_another_string("area_a");
    if (0 == sscanf(tmpstr, "%ld", &area_a)) {
        error_data("wrong area_a!");
        return;
    }

    get_another_string("area_b");
    if (0 == sscanf(tmpstr, "%ld", &area_b)) {
        error_data("wrong area_b!");
        return;
    }

    web = web_head;
    while (NULL != web) {
        shop = web->sub;
        while (NULL != shop) {
            order = shop ->sub;
            while (NULL != order) {
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

    single_echo('T', sizeof(long double), &sum);
}


/*output: UWX*/
void save_and_load_echo(char type) {
    int     fd;

    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &cntw;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = &cnts;
    iov[2].iov_len  = sizeof(long);

    iov[3].iov_base = &cnto;
    iov[3].iov_len  = sizeof(long);


    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);

    writev(fd, iov, 4);

    close(fd);
}


/*U*/
void save_data(void) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    int              fd;
    FILE *           fw;
    FILE *           fs;
    FILE *           fo;

    cntw = 0;
    cnts = 0;
    cnto = 0;

    umask(S_IXUSR | S_IXGRP | S_IXOTH);

    fd = open(DATA_FW, O_RDONLY | O_CREAT, FILE_MODE);
    close(fd);

    fd = open(DATA_FS, O_RDONLY | O_CREAT, FILE_MODE);
    close(fd);

    fd = open(DATA_FO, O_RDONLY | O_CREAT, FILE_MODE);
    close(fd);

    fw = fopen(DATA_FW, "wb");
    fs = fopen(DATA_FS, "wb");
    fo = fopen(DATA_FO, "wb");

    web = web_head;
    while (NULL != web) {
        fwrite(web, sizeof(struct web_t), 1, fw);
        ++cntw;

        shop = web->sub;
        while (NULL != shop) {
            fwrite(shop, sizeof(struct shop_t), 1, fs);
            ++cnts;

            order = shop->sub;
            while (NULL != order) {
                fwrite(order, sizeof(struct order_t), 1, fo);
                ++cnto;

                order = order->next;
            }

            shop = shop->next;
        }

        web = web->next;
    }

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

    cntw = 0;
    cnts = 0;
    cnto = 0;

    /*open files*/
    if (NULL == (fw = fopen(DATA_FW, "rb")) ) {
        return(1);
    }

    if (NULL == (fs = fopen(DATA_FS, "rb")) ) {
        return(1);
    }

    if (NULL == (fo = fopen(DATA_FO, "rb")) ) {
        return(1);
    }

    /*load web*/
    web_head    = NULL;
    web_tail    = NULL;
    web_num_max = 0;

    web = (struct web_t *) calloc(1, sizeof(struct web_t));
    n   = fread(web, sizeof(struct web_t), 1, fw);

    if (0 == n) {
        free(web);
        return(2);
    }

    while (0 != n) {
        web->next = NULL;
        web->sub  = NULL;
        web->cnt  = 0;

        if (web->web_num > web_num_max) {
            web_num_max = web->web_num;
        }

        add_web_item(web);
        ++cntw;

        web = (struct web_t *) calloc(1, sizeof(struct web_t));
        n   = fread(web, sizeof(struct web_t), 1, fw);
    }
    free(web);

    /*load shop*/
    shop = (struct shop_t *) calloc(1, sizeof(struct shop_t));
    n    = fread(shop, sizeof(struct shop_t), 1, fs);

    while (0 != n) {
        shop->sub  = NULL;

        add_shop_item(find_web("", "", shop->web_num, NULL), shop);
        ++cnts;

        shop = (struct shop_t *) calloc(1, sizeof(struct shop_t));
        n    = fread(shop, sizeof(struct shop_t), 1, fs);
    }
    free(shop);

    /*load order*/
    order_num_max = 0;

    order = (struct order_t *) calloc(1, sizeof(struct order_t));
    n     = fread(order, sizeof(struct order_t), 1, fo);

    while (0 != n) {
        if (order->order_num > order_num_max) {
            order_num_max = order->order_num;
        }

        add_order_item(find_shop(find_web("", "", order->web_num, NULL),
                        order->shop_id, "", NULL), order);

        ++cnto;

        order = (struct order_t *) calloc(1, sizeof(struct order_t));
        n     = fread(order, sizeof(struct order_t), 1, fo);
    }
    free(order);

    fclose(fw);
    fclose(fs);
    fclose(fo);

    return (0);
}


/*output: V*/
/*V*/
void active_year(void) {
    struct web_t   * web;
    struct shop_t  * shop;
    struct order_t * order;
    long             year_min = +10000;
    long             year_max = -10000;
    int              fd;
    char             type = 'V';


    web = web_head;
    while (NULL != web) {
        shop = web->sub;
        while (NULL != shop) {
            order = shop ->sub;
            while (NULL != order) {
                if ( order->date.year > year_max ) {
                    year_max = order->date.year;
                }

                if ( order->date.year < year_min ) {
                    year_min = order->date.year;
                }

                order = order->next;
            }
            shop = shop->next;
        }
        web = web->next;
    }

    if (10000 == year_min) {
        error_data("no order records.");
        return;
    }

    iov[0].iov_base = &type;
    iov[0].iov_len  = sizeof(char);

    iov[1].iov_base = &year_min;
    iov[1].iov_len  = sizeof(long);

    iov[2].iov_base = &year_max;
    iov[2].iov_len  = sizeof(long);

    fd = open(FIFO_OUT, O_WRONLY | O_TRUNC);

    writev(fd, iov, 3);

    close(fd);
}


/*W*/
void load_again(char * msg) {
    char    * head = msg;
    char    * tail;
    long      i    = 0;

    /*get passwd*/
    get_first_string("passwd");

    if (0 == strcmp(tmpstr, passwd)) {
        load_data();
        save_and_load_echo('W');
    }
    else {
        error_data("wrong load passwd!");
    }
}


/*X*/
void save_shut_passwd(char * msg) {
    char    * head = msg;
    char    * tail;
    long      i    = 0;

    /*get passwd*/
    get_first_string("passwd");

    if (0 == strcmp(tmpstr, passwd)) {
        run  = 0;
        save = 1;
        save_data();
        save_and_load_echo('X');
    }
    else {
        error_data("wrong save & shutdown passwd!");
    }
}


/*Y*/
void shut_passwd(char * msg) {
    char    * head = msg;
    char    * tail;
    long      i    = 0;

    /*get passwd*/
    get_first_string("passwd");

    if (0 == strcmp(tmpstr, passwd)) {
        run  = 0;
        save = 0;
        delete_done('Y');
    }
    else {
        error_data("wrong shutdown passwd!");
    }
}


char * read_fifo(void) {
    int    fd;
    long   len;

    fd  = open(FIFO_IN, O_RDONLY);

    len = read(fd, msg, MSG_MAX);

    close(fd);

    return (msg);
}


static void time_limit(int signo) {
    delete_fifo();

    syslog(LOG_WARNING, "%s. time is up.", timestr());

    _exit(11);
}


int lockfile(int fd) {
    struct flock fl;

    fl.l_type   = F_WRLCK;
    fl.l_start  = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len    = 0;

    return(fcntl(fd, F_SETLK, &fl));
}


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



int main(int argc, char** argv) {
    struct sigaction    act, oact;

    if (0 != daemon(1, 1)) {
        perror(NULL);
        exit(10);
    }

    openlog("iShopMatrix", LOG_PERROR, 0);

    if (1 == already_running()) {
        syslog(LOG_ERR, "this daemon already running!");
        exit(7);
    }

    act.sa_handler = time_limit;
    act.sa_flags   = SA_INTERRUPT;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGALRM, &act, &oact) < 0) {
        //perror(NULL);
        syslog(LOG_ERR, "%m");
        exit(6);
    }

    create_fifo();

    load_data();

    do {
        read_fifo();

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
        
        alarm(WAIT_SEC);
    } while (run);

    if (1 == save) {
        syslog(LOG_NOTICE, "%s. shutdown and saved by passwd.", timestr());
    }
    else {
        syslog(LOG_NOTICE, "%s. shutdown by passwd.", timestr());
    }

    delete_fifo();

    return (EXIT_SUCCESS);
}


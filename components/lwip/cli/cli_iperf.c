/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_TCPIP)
#include <stdbool.h>
#include <aos/aos.h>
#include <aos/cli.h>

#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>


#define TAG "IPERF"

#ifndef BSD_STACK_SIZE
#define BSD_STACK_SIZE          1024*4
#endif


typedef unsigned int u32;
//typedef unsigned char bool;


static bool running;


struct iperf_param {
    u32 buf_size;
    bool server; /** server/client*/
    bool udp;
    char server_ip[16];
    u32 port;
    u32 second;
    u32 report_interval;
    u32 socket_timeout;
};


struct udp_header {
    u32 id;
    u32 tv_sec;
    u32 tv_usec;
};

typedef struct udp_report_hdr_v1_s {
    int32_t flags;
    int32_t total_len1;
    int32_t total_len2;
    int32_t stop_sec;
    int32_t stop_usec;
    int32_t error_cnt;
    int32_t outorder_cnt;
    int32_t datagrams;
    int32_t jitter1;
    int32_t jitter2;
} udp_report_hdr_v1;


#ifndef __NEWLIB__
static int opterr = 1;    /* getopt prints errors if this is one */
static int optind = 1;    /* token pointer */
static int optopt;        /* option character passed back to user */
static char *optarg = NULL;      /* flag argument (or value) */


static int getopt(int argc, char **argv, char *opts)
{
    static int sp = 1;   /* character index in current token */
    register char *cp;   /* pointer into current token */

    if (sp == 1) {

        /* check for more flag-like tokens */
        if (optind >= argc ||
            argv[optind][0] != '-' || argv[optind][1] == '\0') {
            return (EOF);
        } else if (strcmp(argv[optind], "--") == 0) {
            optind++;
            return (EOF);
        }
    }

    optopt = argv[optind][sp];

    if (optopt == ':' || (cp = strchr(opts, optopt)) == 0) {
        printf("%c: illegal option \n", optopt);

        /* if no chars left in this token, move to next token */
        if (argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }

        return ('?');
    }

    if (*++cp == ':') { /* if a value is expected, get it */
        if (argv[optind][sp + 1] != '\0')
            /* flag value is rest of current token */
        {
            optarg = &argv[optind++][sp + 1];
        } else if (++optind >= argc) {
            printf("%c: option requires an argument -- ", optopt);
            sp = 1;
            return ('?');
        } else
            /* flag value is next token */
        {
            optarg = argv[optind++];
        }

        sp = 1;
    } else {
        /* set up to look at next char in token, next time */
        if (argv[optind][++sp] == '\0') {
            /* no m ore in current token, set up next token */
            sp = 1;
            optind++;
        }

        optarg = 0;
    }

    return (optopt); /* return current flag character found */
}
#endif

static int tcpclient(struct iperf_param *param)
{
    int                 iCounter;
    struct sockaddr_in  sAddr;
    int                 iAddrSize;
    int                 iSockFD;
    int                 iStatus;
    long                lLoopCount = 0;
    char            *cBsdBuf = NULL;
    int time_ms = aos_now_ms();
    int time_ms_step = aos_now_ms();
    int send_bytes = 0;

    cBsdBuf = aos_malloc(param->buf_size);

    if (NULL == cBsdBuf) {
        LOGE(TAG, "TCP: Allocate client buffer failed.");
        return -1;
    }

    // filling the buffer
    for (iCounter = 0; iCounter < param->buf_size; iCounter++) {
        cBsdBuf[iCounter] = (char)(iCounter % 10);
    }

    //filling the TCP server socket address
    memset(&sAddr, 0, sizeof(struct sockaddr_in));
    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons(param->port);
    sAddr.sin_addr.s_addr = inet_addr(param->server_ip);

    iAddrSize = sizeof(struct sockaddr_in);

    // creating a TCP socket
    iSockFD = socket(AF_INET, SOCK_STREAM, 0);

    if (iSockFD < 0) {
        LOGE(TAG, "TCP ERROR: create tcp client socket fd error!");
        goto Exit1;
    }

    LOGD(TAG, "ServerIP=%s port=%d.", param->server_ip, param->port);
    LOGD(TAG, "Create socket %d.", iSockFD);
    // connecting to TCP server
    iStatus = connect(iSockFD, (struct sockaddr *)&sAddr, iAddrSize);

    if (iStatus < 0) {
        LOGE(TAG, "TCP ERROR: tcp client connect server error! ");
        goto Exit;
    }

    LOGD(TAG, "TCP: Connect server successfully.");
    // sending multiple packets to the TCP server

    if (param->socket_timeout) {
        struct timeval interval = {param->socket_timeout, 0};
        if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 100)) {
            interval.tv_sec = 1;
            interval.tv_usec = 10000;
        }
        if (setsockopt(iSockFD, SOL_SOCKET, SO_SNDTIMEO, (char *)&interval, sizeof(struct timeval))) {
            LOGE(TAG, "SetSocket SO_SNDTIMEO failed");
            close(iSockFD);
            aos_free(cBsdBuf);
            return -1;
        }
    }



    printf("[  ID] Interval       Transfer     Bandwidth\n");

    while (running) {
        // sending packet
        iStatus = send(iSockFD, cBsdBuf, param->buf_size, 0);

        if (iStatus <= 0) {
            printf("TCP ERROR: tcp client send data error!  iStatus:%d", iStatus);
            goto Exit;
        }

        lLoopCount++;

        /** aos_now_ms is not accurate when system is busy */
        if ((aos_now_ms() - time_ms_step) >=  1000 * param->report_interval) {
            time_ms_step = aos_now_ms();
            int time_now_sec = (aos_now_ms() - time_ms) / 1000 - 1;
            printf("[%d] %d - %d sec  %d KBytes %.2f Mbits/sec\n",
                   iSockFD, time_now_sec - param->report_interval, time_now_sec,
                   send_bytes / 1024,
                   (float)send_bytes * 8 / 1024 / 1024 / param->report_interval);
            send_bytes = 0;
        }


        if ((aos_now_ms() - time_ms) / 1000 >  param->second) {
            break;
        }

        send_bytes += iStatus;

        //printf("BsdTcpClient:: send data count:%ld iStatus:%d \n\r", lLoopCount, iStatus);
    }

    LOGD(TAG, "TCP: Sent packets successfully.");

Exit:
    //closing the socket after sending 1000 packets
    close(iSockFD);

Exit1:
    //free buffer
    aos_free(cBsdBuf);

    return 0;
}

static int tcpserver(struct iperf_param *param)
{
    struct sockaddr_in  sAddr;
    struct sockaddr_in  sLocalAddr;
    int                 iCounter;
    int                 iAddrSize;
    int                 iSockFD;
    int                 iStatus;
    int                 iNewSockFD;
    long                lLoopCount = 0;
    int                 recv_bytes = 0;
    //long                lNonBlocking = 1;
//    int                 iTestBufLen;
    int                 n;
    char            *cBsdBuf = NULL;
    int time_ms = aos_now_ms();
    int time_ms_step = aos_now_ms();

    cBsdBuf = aos_malloc(param->buf_size);

    if (NULL == cBsdBuf) {
        LOGE(TAG, "TCP: Allocate server buffer failed.");
        return -1;
    }

    // filling the buffer
    for (iCounter = 0; iCounter < param->buf_size; iCounter++) {
        cBsdBuf[iCounter] = (char)(iCounter % 10);
    }

    // creating a TCP socket
    iSockFD = socket(AF_INET, SOCK_STREAM, 0);

    if (iSockFD < 0) {
        goto Exit2;
    }

    LOGD(TAG, "TCP: Create server socket %d\n\r", iSockFD);
    n = 1;
    if (setsockopt(iSockFD, SOL_SOCKET, SO_REUSEADDR, (const char *) &n, sizeof(n)) < 0) {
        goto Exit1;
    }

    //filling the TCP server socket address
    memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));
    sLocalAddr.sin_family      = AF_INET;
    sLocalAddr.sin_len         = sizeof(sLocalAddr);
    sLocalAddr.sin_port        = htons(param->port);
    sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    iAddrSize = sizeof(sLocalAddr);

    // binding the TCP socket to the TCP server address
    iStatus = bind(iSockFD, (struct sockaddr *)&sLocalAddr, iAddrSize);

    if (iStatus < 0) {
        LOGE(TAG, "ERROR: bind tcp server socket fd error! ");
        goto Exit1;
    }

    LOGD(TAG, "Bind successfully.");

    // putting the socket for listening to the incoming TCP connection
    iStatus = listen(iSockFD, 20);

    if (iStatus != 0) {
        LOGE(TAG, "TCP ERROR: listen tcp server socket fd error! ");
        goto Exit1;
    }

    LOGD(TAG, "TCP: Listen port %d", param->port);

//Restart:
    iNewSockFD = -1;
    lLoopCount = 0;

    // waiting for an incoming TCP connection
    while (iNewSockFD < 0) {
        // accepts a connection form a TCP client, if there is any
        // otherwise returns SL_EAGAIN
        int addrlen = sizeof(sAddr);
        iNewSockFD = accept(iSockFD, (struct sockaddr *)&sAddr,
                            (socklen_t *)&addrlen);

        if (iNewSockFD < 0) {
            LOGE(TAG, "ERROR: Accept tcp client socket fd error! ");
            goto Exit1;
        }

        LOGD(TAG, " Accept socket %d successfully.", iNewSockFD);
    }

    // waits packets from the connected TCP client
    while (running) {
        iStatus = recv(iNewSockFD, cBsdBuf, param->buf_size, 0);  //MSG_DONTWAIT   MSG_WAITALL

        if (iStatus < 0) {
            printf("\n\rTCP ERROR: server recv data error iStatus:%d ", iStatus);
            goto Exit;
        } else if (iStatus == 0) {
            printf("\n\rTCP: Recieved %ld packets successfully.", lLoopCount);
            close(iNewSockFD);
            //goto Restart;
            goto Exit1;
        }

        if ((aos_now_ms() - time_ms_step) / 1000 >=  1) {
            time_ms_step = aos_now_ms();
            int time_now_sec = (aos_now_ms() - time_ms) / 1000 - 1;
            printf("[%d] %d - %d sec  %d KBytes %.2f Mbits/sec\n",
                   iSockFD, time_now_sec, time_now_sec + 1, recv_bytes / 1024, (float)recv_bytes * 8 / 1024 / 1024);
            recv_bytes = 0;
        }


        //if (iStatus != param->buf_size) {
        //   LOGE(TAG, "TCP E1");
        //}

        recv_bytes += iStatus;
        lLoopCount++;
    }

Exit:
    // close the connected socket after receiving from connected TCP client
    close(iNewSockFD);

Exit1:
    // close the listening socket
    close(iSockFD);

Exit2:
    //free buffer
    aos_free(cBsdBuf);

    return 0;
}

static struct timeval os_get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    //avoid long int too large to overflow
    return tv;
}


static int udpclient(struct iperf_param *param)
{
    int cli_sockfd;
    socklen_t addrlen;
    struct sockaddr_in cli_addr;
    int loop = 0;
    char *buffer;
//  int delay = 2;
    int time_ms = aos_now_ms();
    int time_ms_step = aos_now_ms();
    int udp_id = 0;

    buffer = (char *)aos_malloc(param->buf_size);

    if (NULL == buffer) {
        printf("\n\rudpclient: Allocate buffer failed.\n");
        return -1;
    }

    /*create socket*/
    memset(buffer, 0, param->buf_size);
    cli_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (cli_sockfd < 0) {
        printf("create socket failed\r\n\n");
        aos_free(buffer);
        return -1;
    }

    /* fill sockaddr_in*/
    addrlen = sizeof(struct sockaddr_in);
    memset(&cli_addr, 0, addrlen);

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = inet_addr(param->server_ip);
    cli_addr.sin_port = htons(param->port);


    int send_ret = 0;
    int send_bytes = 0;


    /** wait arp entry filled or arp will stuck */
    LOGD(TAG, "UDP Prepare for ARP ...");
    if (sendto(cli_sockfd, buffer, param->buf_size, 0, (struct sockaddr *)&cli_addr, addrlen) < 0) {
        close(cli_sockfd);
        aos_free(buffer);
        return -1;
    }
    aos_msleep(2000);
    LOGD(TAG, "UDP Prepare for ARP Done");

    if (param->socket_timeout) {
        struct timeval interval = {param->socket_timeout, 0};
        if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 100)) {
            interval.tv_sec = 1;
            interval.tv_usec = 10000;
        }
        if (setsockopt(cli_sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&interval, sizeof(struct timeval))) {
            LOGE(TAG, "SetSocket SO_SNDTIMEO failed");
            close(cli_sockfd);
            aos_free(buffer);
            return -1;
        }
    }


    /* send data to server*/
    while (running) {

        struct udp_header *udp = (struct udp_header *)buffer;
        struct timeval tv = os_get_time();
        /** for jitter & packet lost */
        udp->id = htonl(udp_id++);
        udp->tv_sec = htonl(tv.tv_sec);
        udp->tv_usec = htonl(tv.tv_usec);

        send_ret = sendto(cli_sockfd, buffer, param->buf_size, 0, (struct sockaddr *)&cli_addr, addrlen);

        if (send_ret <= 0) {
            //LOGE(TAG, "UDP Send failed");
            //printf("U %d\n", send_ret);
            aos_msleep(10);
            continue;
        }

        if ((aos_now_ms() - time_ms_step) / 1000 >=  1) {
            time_ms_step = aos_now_ms();
            int time_now_sec = (aos_now_ms() - time_ms) / 1000 - 1;
            printf("[%d] %d - %d sec  %d KBytes %.2f Mbits/sec\n",
                   cli_sockfd, time_now_sec, time_now_sec + 1, send_bytes / 1024, (float)send_bytes * 8 / 1024 / 1024);
            send_bytes = 0;

            //aos_msleep(10);
        }

        if ((aos_now_ms() - time_ms) / 1000 >  param->second) {
            udp->id = htonl(udp_id++ | 0x80000000);
            udp->tv_sec = htonl(tv.tv_sec);
            udp->tv_usec = htonl(tv.tv_usec);
            send_ret = sendto(cli_sockfd, buffer, param->buf_size, 0, (struct sockaddr *)&cli_addr, addrlen);
            LOGD(TAG, "Notify peer test end.");
            break;
        }


        send_bytes += send_ret;
        loop++;
    }

    close(cli_sockfd);
    aos_free(buffer);
    return 0;
}

static int udpserver(struct iperf_param *param)
{
    int ser_sockfd;
    socklen_t addrlen;
    struct sockaddr_in ser_addr, peer_addr;
    //uint32_t start_time;
    unsigned char *buffer;
    //int total_size = 0; report_interval = 1000;
    int time_ms_step = aos_now_ms();
    int time_ms = aos_now_ms();

    /*create socket*/
    ser_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (ser_sockfd < 0) {
        printf("\n\rudp server success");
        return 1;
    }

    /*fill the socket in*/
    addrlen = sizeof(ser_addr);
    memset(&ser_addr, 0, addrlen);
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(param->port);

    /*bind*/
    if (bind(ser_sockfd, (struct sockaddr *)&ser_addr, addrlen) < 0) {
        close(ser_sockfd);
        printf("bind failed\r\n");
        return 1;
    }

    buffer = aos_malloc(param->buf_size);

    if (NULL == buffer) {
        close(ser_sockfd);
        printf("\n\rudpclient: Allocate buffer failed.\n");
        return -1;
    }

    //uint32_t start_time = aos_now_ms();
    int recv_bytes = 0;

    while (running) {
        int read_size = 0;
        addrlen = sizeof(peer_addr);
        read_size = recvfrom(ser_sockfd, buffer, param->buf_size, 0, (struct sockaddr *) &peer_addr, &addrlen);

        if (read_size < 0) {
            printf("%s recv error\r\n", __FUNCTION__);
            goto Exit;
        }

        if ((aos_now_ms() - time_ms_step) / 1000 >=  1) {
            time_ms_step = aos_now_ms();
            int time_now_sec = (aos_now_ms() - time_ms) / 1000 - 1;
            printf("[%d] %d - %d sec  %d KBytes %.2f Mbits/sec\n",
                   ser_sockfd, time_now_sec, time_now_sec + 1, recv_bytes / 1024, (float)recv_bytes * 8 / 1024 / 1024);
            recv_bytes = 0;
        }

        recv_bytes += read_size;

        /*ack data to client*/
// Not send ack to prevent send fail due to limited skb, but it will have warning at iperf client
//      sendto(ser_sockfd,buffer,read_size,0,(struct sockaddr*)&peer_addr,sizeof(peer_addr));
    }

Exit:
    close(ser_sockfd);
    //free buffer
    aos_free(buffer);
    return 0;
}





static void network_thread(void *arg)
{
    struct iperf_param *param = (struct iperf_param *)arg;
    running = 1;

    LOGD(TAG, "iperf thread starts...");

    if (param->udp) {

        /** max = 14+20+8+1472 =1514, 1472 is max,
               1470 is iperf default value */
        if (param->buf_size > 1472) {
            param->buf_size = 1470; //Force below 1514 or driver will fail when mt_send_lynx_buff
            LOGD(TAG, "Buf size adjust to 1470 for UDP");
        }

        if (param->server) {
            udpserver(param);
        } else {
            udpclient(param);
        }
    } else {
        if (param->server) {
            tcpserver(param);
        } else {
            tcpclient(param);
        }

    }

    LOGD(TAG, "iperf thread end");

    aos_task_exit(0);
}

static void print_help()
{
    printf("-g:Set sockopt send timeout\n"
           "-u:use UDP rather than TCP\n"
           "-l:length of buffer in bytes to read or write (default:10KB)\n"
           "-c:run in client mode, connecting to <host>\n"
           "-x:stop iperf test\n"
           "-s:run in server mode\n"
           "-t:time in seconds to transmit for (default 10 secs)\n"
           "-p:server port to listen on/connect to\n"
           "-i:seconds between periodic bandwidth reports\n");

}

static void iperf(char *wbuf, int wbuf_len, int argc, char **argv)
{
    static struct iperf_param param;
    param.buf_size = 10 * 1024; //default 10K for tcp
    param.port = 5001;
    param.report_interval = 1;
    param.second = 10;
    param.server = false;
    param.socket_timeout = 0;
    memset(param.server_ip, 0, sizeof(param.server_ip));
    param.udp = 0;



    /**
        dump arguments

        argv[0] = iperf
        argv[1] = -u
    */
    int i = 0;

    for (i = 0; i < argc; i++) {
        LOGD(TAG, "argv[%d]=%s", i, argv[i]);
    }

    if (argc == 1) {
        print_help();
        return;
    }

    /** getopt init */
    opterr = 1;
    optind = 1;
//    optopt;
    optarg = NULL;


    int opt;

    while ((opt = getopt(argc, argv, "ui:c:st:p:xl:g:h")) != -1) {
        switch (opt) {
        case 'h':
            print_help();
            return;


        case 'g':
            param.socket_timeout = atoi(optarg);
            printf("Socket timeout=%ds flow slow device such as GPRS\n", param.socket_timeout);
            break;

        case 'u':
            param.udp = 1;
            printf("using udp rather than tcp\n");
            break;

        case 'l':
            param.buf_size = atoi(optarg);
            break;

        case 'c':
            param.server = 0;
            if (optarg) {
                strlcpy(param.server_ip, optarg, sizeof(param.server_ip));
                printf("Server ip=%s\n", optarg);
            }
            break;

        case 'x':
            running = 0;
            return;

        case 's':
            param.server = 1;
            break;

        case 't':
            printf("time=%ss\n", optarg);
            param.second = atoi(optarg);
            break;

        case 'p':
            printf("port=%s\n", optarg);
            param.port = atoi(optarg);
            break;

        case 'i':
            printf("interval: %s\n", optarg);
            param.report_interval = atoi(optarg);
            break;

        case ':':
            printf("option needs a value\n");
            break;

        case '?':
            printf("unknown option: %c\n", optopt);
            break;
        }
    }

    // optind is for the extra arguments
    // which are not parsed
    for (; optind < argc; optind++) {
        printf("unknown arguments: %s\n", argv[optind]);
    }

    printf("Buffer Size=%d\nServer IP=%s:%d, Type=%s %s \nRun for %d seconds \nReport Every %d seconds\n",
           param.buf_size, param.server_ip, param.port,
           param.udp ? "UDP" : "TCP",
           param.server ?
           "Server" : "Client",
           param.second,
           param.report_interval
          );


    aos_task_t task_handle;

    if (0 != aos_task_new_ext(&task_handle, "network_thread", network_thread, &param, BSD_STACK_SIZE, AOS_DEFAULT_APP_PRI + 3)) {
        LOGE(TAG, "Create network task failed.");
    }

}

void cli_reg_cmd_iperf(void)
{
    static const struct cli_command cmd_info = {
        "iperf",
        "network performance test.",
        iperf
    };

    aos_cli_register_command(&cmd_info);

    return;
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define __FAVOR_BSD
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define CHKADDRESS(_saddr_)                                                                                             \
    {                                                                                                                   \
        unsigned char *p = (unsigned char *)&(_saddr_);                                                                 \
        if ((p[0] == 127) || (p[0] == 10) || (p[0] == 172 && 16 <= p[1] && p[1] <= 31) || (p[0] == 192 && p[1] == 168)) \
            ;                                                                                                           \
        else                                                                                                            \
        {                                                                                                               \
            fprintf(stderr, "IP address error.\n");                                                                     \
            exit(EXIT_FAILURE);                                                                                         \
        }                                                                                                               \
    }

#define MAXDATA 1024

enum
{
    CMD_NAME,
    DST_IP,
    DST_PORT,
    SRC_IP,
    SRC_PORT,
    SEQ
};

struct packet_tcp
{
    struct ip ip;
    struct tcphdr tcp;
    unsigned char data[MAXDATA];
};

/* プロトタイプ宣言 */
void make_ip_header(struct ip *ip, int src_ip, int dst_ip, int len);
void make_tcp_header(struct packet_tcp *packet, int src_ip, int src_port,
                     int dst_ip, int dst_port, int seq, int ack, int datalen);
uint16_t checksum(u_int16_t *data, int len);

int main(int argc, char *argv[])
{
    struct packet_tcp send;
    struct sockaddr_in dest;
    u_int32_t src_ip;
    u_int32_t dst_ip;
    u_int16_t src_port;
    u_int16_t dst_port;
    int s;
    tcp_seq seq;
    tcp_seq ack;
    int datalen;
    int iplen;
    int on = 1;

    /* 引数チェック */
    if (argc != 6)
    {
        fprintf(stderr, "usage: %s dst_ip dst_port src_ip src_port seq\n",
                argv[CMD_NAME]);
        exit(EXIT_FAILURE);
    }

    /*
        * 書式
        #include <sys/types.h>
        #include <sys/socket.h>

        int
        socket(int domain, int type, int protocol);

        * 解説
        socket()は, 通信のエンドポイントを生成してそのエンドポイントを参照するファイルディスクリプタ
        を返す. エラーが発生した場合は-1を返す
     */
    if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0)
    {
        perror("socket(SOCK_RAW)");
        exit(EXIT_FAILURE);
    }

    /*
        * 書式
        #include <sys/types.h>
        #include <sys/socket.h>

        int
        setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);

        * 解説
        ソケットに関するオプションを設定する
        正常に終了した場合は0を, そうでない場合は-1を返す
     */
    if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &on, sizeof on) < 0)
    {
        perror("setsockopt(IPPROTO_IP, IP_HDRINCL)");
        exit(EXIT_FAILURE);
    }

    /*
        * 書式
        #include <string.h>

        void*
        memset(void *b, int c, size_t len);

        * 解説
        memset()関数は, 値 c(unsigned charに変換)のlenバイトを文字列bに書き込む
     */
    memset(&dest, 0, sizeof dest);
    dest.sin_family = AF_INET;
    dst_ip = dest.sin_addr.s_addr = inet_addr(argv[DST_IP]);
    src_ip = inet_addr(argv[SRC_IP]);

    /*
        #include <stdio.h>

        int
        sscanf(const char *str, const char *format, ...);

        * 解説
         sscanf()は, 入力をstrの指すキャラクタ文字列から読み取る
     */
    sscanf(argv[DST_PORT], "%hu", &dst_port);
    sscanf(argv[SRC_PORT], "%hu", &src_port);
    sscanf(argv[SEQ], "%ul", &seq);
    ack = 0;
    datalen = 0;
    iplen = datalen + (sizeof send.ip) + (sizeof send.tcp);

    memset(&send, 0, sizeof send);
    make_tcp_header(&send, src_ip, src_port, dst_ip, dst_port, seq, ack, datalen);
    make_ip_header(&(send.ip), src_ip, dst_ip, iplen);

    CHKADDRESS(dst_ip);

    printf("SYN send to %s.\n", argv[DST_IP]);

    /*
        * 書式
        #include <sys/types.h>
        #include <sys/socket.h>

        ssize_
        sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

        * 解説
        メッセージを別のソケットに送信するのに使用する
        成功した場合は送信された文字数を、エラーが起きた場合は-1を返す
     */
    if (sendto(s, (char *)&send, iplen, 0, (struct sockaddr *)&dest, sizeof dest) < 0)
    {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
    close(s);

    return 0;
}

void make_tcp_header(struct packet_tcp *packet, int src_ip, int src_port, int dst_ip, int dst_port, int seq, int ack, int datalen)
{
    /* htonl()関数は, unsigned integer hostlongをホストバイトオーダーからネットワークバイトオーダーに変換する */
    packet->tcp.th_seq = htonl(seq);
    packet->tcp.th_ack = htonl(ack);
    /* htons()関数は, unsigned short integer hostshortをホストバイトオーダーからネットワークバイトオーダーに変換する */
    packet->tcp.th_sport = htons(src_port);
    packet->tcp.th_dport = htons(dst_port);
    packet->tcp.th_off = 5;
    packet->tcp.th_flags = TH_SYN;
    packet->tcp.th_win = htons(8102);
    packet->tcp.th_urp = htons(0);

    packet->ip.ip_ttl = 0;
    packet->ip.ip_p = IPPROTO_TCP;
    packet->ip.ip_src.s_addr = src_ip;
    packet->ip.ip_dst.s_addr = dst_ip;
    packet->ip.ip_sum = htons((sizeof packet->tcp) + datalen);

#define PSEUDO_HEADER_LEN 12
    packet->tcp.th_sum = 0;
    packet->tcp.th_sum = checksum((u_int16_t *)&(packet->ip.ip_ttl),
                                  PSEUDO_HEADER_LEN + (sizeof packet->tcp) + datalen);
}

void make_ip_header(struct ip *ip, int src_ip, int dst_ip, int iplen)
{
    ip->ip_v = IPVERSION;
    ip->ip_hl = sizeof(struct ip) >> 2;
    ip->ip_id = 0;
#ifdef __linux
    ip->ip_len = htons(iplen);
    ip->ip_off = htons(0);
#else
    ip->ip_len = iplen;
    ip->ip_off = IP_DF;
#endif
    ip->ip_ttl = 2;
    ip->ip_p = IPPROTO_TCP;
    ip->ip_src.s_addr = src_ip;
    ip->ip_dst.s_addr = dst_ip;

    ip->ip_sum = 0;
    ip->ip_sum = checksum((u_int16_t *)ip, sizeof ip);
}

u_int16_t checksum(u_int16_t *data, int len)
{
    u_int32_t sum = 0;

    for (; len > 1; len -= 2)
    {
        sum += *data++;
        if (sum & 0x80000000)
            sum = (sum & 0xffff) + (sum >> 16);
    }

    if (len == 1)
    {
        u_int16_t i = 0;
        *(u_char *)(&i) = *(u_char *)data;
        sum += i;
    }

    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return (sum == 0xffff) ? sum : ~sum;
}

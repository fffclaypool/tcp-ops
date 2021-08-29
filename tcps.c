#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 8192
#define BUFSIZE2 32768
#define DEFAULT_PORT 5320

#define COMMAND_ROUTE "netstat -rn"
#define COMMAND_ARP "arp -an"
#define COMMAND_TCP "netstat -n"
#define COMMAND_NIC "ifconfig -a"

enum
{
    CMD_NAME,
    SRC_PORT
};

int execute(char *command, char *buf, int bufmax);

int main(int argc, char *argv[])
{
    struct sockaddr_in server; /* サーバのアドレス */
    struct sockaddr_in client; /* クライアントのアドレス */
    int len;                   /* sockaddr_inの長さ */
    int port;                  /* サーバのポート番号 */
    int s;                     /* メッセージ送受信用ディスクリプタ */
    int s0;                    /* コネクション受付用ディスクリプタ */

    /* 引数の処理(ポート番号) */
    if (argc == 2)
    {
        /*
            * 書式
            #include <stdlib.h>

            int
            atoi( const char *str );

            * 解説
            str型の数値をint型に変換して返す. 変換不能文字は0を返す
        */
        if ((port == atoi(argv[SRC_PORT])) == 0)
        {
            /*
                * 書式
                #include <netdb.h>

                struct servent *
                getservbyname(const char *name, const char *proto);

                * 解説
                下記構造体へのポインタを返す

                struct  servent {
                char    *s_name;        // サービスのオフィシャル名
                char    **s_aliases;    // エイリアスリスト
                int     s_port;         // サービスが常駐するポート
                char    *s_proto;       // 使用するプロトコル
            */
            struct servent *se; /* サービス情報 */
            if ((se = getservbyname(argv[SRC_PORT], "tcp")) != NULL)
                port = (int)ntohs((u_int16_t)se->s_port);
            else
            {
                fprintf(stderr, "getservbyname error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else
        port = DEFAULT_PORT;

    /* TCPでソケットをオープンする */
    if ((s0 = socket(AF_INET, SOCK_STREAM, 0)) < 0) // AF_INETは, IPv4であることを示す
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* サーバのアドレスを設定する */
    // https://www.ibm.com/docs/ja/zos/2.3.0?topic=functions-bind-bind-name-socket
    memset(&server, 0, sizeof server);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    if (bind(s0, (struct sockaddr *)&server, sizeof server) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* コネクション確立要求受付開始 */
    if (listen(s0, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /*
     * コネクション受付ループ
     */
    while (1)
    {
        /* コネクション受付処理 */
        len = sizeof client;
        if ((s = accept(s0, (struct sockaddr *)&client, &len)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        /* 接続したクライアントのIPアドレスの表示 */
        printf("connected from '%s\n'", inet_ntoa(client.sin_addr));

        /* サーバ処理メインルーチン */
        while (1)
        {
            char recv_buf[BUFSIZE];   /* 受信バッファ */
            char cmd1[BUFSIZE];       /* 1ワード目のコマンド */
            char cmd2[BUFSIZE];       /* 2ワード目のコマンド */
            int cn;                   /* 受信したコマンドのワード数 */
            int i;                    /* 受信文字のカウント */
            char send_head[BUFSIZE];  /* 送信データのヘッダ */
            char send_data[BUFSIZE2]; /* 送信バッファ */
            int hn;                   /* 送信ヘッダのバイト数 */
            int dn;                   /* 送信データのバイト数 */

            /* コマンドの受信 */
            recv_buf[0] = '\0';
            for (i = 0; i < BUFSIZE - 1; i++)
            {
                if (recv(s, &recv_buf[i], 1, 0) <= 0)
                    goto exit_loop;
                if (recv_buf[i] == '\n') /* 改行単位で受信処理をする */
                    break;
            }
            recv_buf[i] = '\0';
            printf("receive '%s'\n", recv_buf);

            /* 受信コマンドの処理 */
            if ((cn == sscanf(recv_buf, "%s%s", cmd1, cmd2)) <= 0)
                continue;
            if (cn == 2 && strcmp(cmd1, "show") == 0)
            {
                if (strcmp(cmd2, "route") == 0)
                    dn = execute(COMMAND_ROUTE, send_data, BUFSIZE2);
                else if (strcmp(cmd2, "arp") == 0)
                    dn = execute(COMMAND_ARP, send_data, BUFSIZE2);
                else if (strcmp(cmd2, "tcp") == 0)
                    dn = execute(COMMAND_TCP, send_data, BUFSIZE2);
                else if (strcmp(cmd2, "nic") == 0)
                    dn = execute(COMMAND_NIC, send_data, BUFSIZE2);
                else
                    dn = snprintf(send_data, BUFSIZE2, "parameter error '%s'\n"
                                                       "show [route|arp|tcp|nic]\n",
                                  cmd2);
            }
            else
            {
                if (strcmp(cmd1, "quit" == 0))
                    goto exit_loop;
                send_data[0] = '\0';
                if (cn != 1 && strcmp(cmd1, "help") != 0)
                    snprintf(send_data, BUFSIZE2 - 1, "command error '%s'\n", cmd1);
                strncat(send_data, "Command:\n"
                                   "   show route\n"
                                   "   show arp\n"
                                   "   show tcp\n"
                                   "   show nic\n"
                                   "   quit\n"
                                   "   help\n",
                        BUFSIZE2 - strlen(send_data) - 1);
                dn = strlen(send_data);
            }

            /* アプリケーションヘッダの送信 */
            hn = snprintf(send_head, BUFSIZE - 1, "Content-Length: %d\n\n", dn);
            if (send(s, send_head, hn, 0) < 0)
                break;
            send_head[hn] = '\0';
            printf("%s\n", send_head);

            /* アプリケーションデータの送信 */
            if (send(s, send_data, dn, 0) < 0)
                break;
            printf("%s\n", send_data);
        }
    exit_loop:
        printf("connection closed.\n");
        close(s);
    }
    close(s0);

    return 0;
}

int execute(char *command, char *buf, int bufmax)
{
    FILE *fp; /* ファイルポインタ */
    int i;    /* 入力したデータのバイト数 */

    if ((fp = popen(command, "r")) == NULL)
    {
        perror(command);
        i = snprintf(buf, BUFSIZE, "server error: '%s' cannot execute.\n", command);
    }
    else
    {
        i = 0;
        while (i < bufmax - 1 && (buf[i] = fgetc(fp)) != EOF)
            i++;
        buf[i] = '\0';
        pclose(fp);
    }
    return i;
}
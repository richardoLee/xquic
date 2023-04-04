//
// Created by lzq on 2023/4/4.
//

#include <malloc.h>
#include <netinet/in.h>
#include <memory.h>
#include <xquic/xquic_typedef.h>
#include <getopt.h>
#include <stdlib.h>
#include "common.h"

typedef enum xqc_demo_cli_task_mode_s {
    /* send multi requests in single connection with multi streams */
    MODE_SCMR,

    /* serially send multi requests in multi connections, with one request each connection */
    MODE_SCSR_SERIAL,

    /* concurrently send multi requests in multi connections, with one request each connection */
    MODE_SCSR_CONCURRENT,
} xqc_demo_cli_task_mode_t;

/* network arguments */
typedef struct xqc_demo_cli_net_config_s {

    /* server addr info */
    struct sockaddr_in6 addr;
    int addr_len;
    char server_addr[64];
    short server_port;
    char host[256];

    /* ipv4 or ipv6 */
    int ipv6;

    /* congestion control algorithm */
    CC_TYPE cc;     /* congestion control algorithm */
    int pacing; /* is pacing on */

    /* idle persist timeout */
    int conn_timeout;

    xqc_demo_cli_task_mode_t mode;

} xqc_demo_cli_net_config_t;


//xqc_demo_cli_quic_config_t

typedef enum xqc_demo_cli_alpn_type_s {
    ALPN_HQ,
    ALPN_H3,
} xqc_demo_cli_alpn_type_t;

#define MAX_SESSION_TICKET_LEN      2048    /* session ticket len */
#define MAX_TRANSPORT_PARAMS_LEN    2048    /* transport parameter len */
#define XQC_MAX_TOKEN_LEN           256     /* token len */

typedef struct xqc_demo_cli_quic_config_s {
    /* alpn protocol of client */
    xqc_demo_cli_alpn_type_t alpn_type;
    char alpn[16];

    /* 0-rtt config */
    int st_len;                        /* session ticket len */
    char st[MAX_SESSION_TICKET_LEN];    /* session ticket buf */
    int tp_len;                        /* transport params len */
    char tp[MAX_TRANSPORT_PARAMS_LEN];  /* transport params buf */
    int token_len;                     /* token len */
    char token[XQC_MAX_TOKEN_LEN];      /* token buf */

    char *cipher_suites;                /* cipher suites */

    uint8_t use_0rtt;                   /* 0-rtt switch, default turned off */
    uint64_t keyupdate_pkt_threshold;   /* packet limit of a single 1-rtt key, 0 for unlimited */

} xqc_demo_cli_quic_config_t;

// xqc_demo_cli_env_config_t
typedef struct xqc_demo_cli_env_config_s {

    /* log path */
    char log_path[256];
    int log_level;

    /* out file */
    char out_file_dir[256];

    /* key export */
    int key_output_flag;
    char key_out_path[256];

    /* life cycle */
    int life;
} xqc_demo_cli_env_config_t;

// xqc_demo_cli_client_args_t

#define MAX_REQUEST_CNT 2048    /* client might deal MAX_REQUEST_CNT requests once */
#define MAX_REQUEST_LEN 256     /* the max length of a request */

typedef struct xqc_demo_cli_request_s {
    char path[RESOURCE_LEN];         /* request path */
    char scheme[8];                  /* request scheme, http/https */
    REQUEST_METHOD method;
    char auth[AUTHORITY_LEN];
    char url[URL_LEN];               /* original url */
    // char            headers[MAX_HEADER][256];   /* field line of h3 */
} xqc_demo_cli_request_t;

typedef struct xqc_demo_cli_requests_s {
    /* requests */
    char urls[MAX_REQUEST_CNT * MAX_REQUEST_LEN];
    int request_cnt;    /* requests cnt in urls */
    xqc_demo_cli_request_t reqs[MAX_REQUEST_CNT];
} xqc_demo_cli_requests_t;

typedef struct xqc_demo_cli_client_args_s {
    /* network args */
    xqc_demo_cli_net_config_t net_cfg;

    /* quic args */
    xqc_demo_cli_quic_config_t quic_cfg;

    /* environment args */
    xqc_demo_cli_env_config_t env_cfg;

    /* request args */
    xqc_demo_cli_requests_t req_cfg;
} xqc_demo_cli_client_args_t;

// xqc_demo_cli_init_args(args);
#define LOG_PATH "clog.log"
#define KEY_PATH "ckeys.log"
#define OUT_DIR  "."

void
xqc_demo_cli_init_args(xqc_demo_cli_client_args_t *args) {
    memset(args, 0, sizeof(xqc_demo_cli_client_args_t));

    /* net cfg */
    args->net_cfg.conn_timeout = 30;
    strncpy(args->net_cfg.server_addr, "127.0.0.1", sizeof(args->net_cfg.server_addr));
    args->net_cfg.server_port = 8443;

    /* env cfg */
    args->env_cfg.log_level = XQC_LOG_DEBUG;
    strncpy(args->env_cfg.log_path, LOG_PATH, sizeof(args->env_cfg.log_path));
    strncpy(args->env_cfg.out_file_dir, OUT_DIR, sizeof(args->env_cfg.out_file_dir));

    /* quic cfg */
    args->quic_cfg.alpn_type = ALPN_HQ;
    strncpy(args->quic_cfg.alpn, "hq-interop", sizeof(args->quic_cfg.alpn));
    args->quic_cfg.keyupdate_pkt_threshold = UINT64_MAX;
}

// xqc_demo_cli_parse_args
void
xqc_demo_cli_parse_args(int argc, char *argv[], xqc_demo_cli_client_args_t *args)
{
    int ch = 0;
    while ((ch = getopt(argc, argv, "a:p:c:Ct:S:0m:A:D:l:L:k:K:U:u:")) != -1) {
        switch (ch) {
            /* server ip */
            case 'a':
                printf("option addr :%s\n", optarg);
                snprintf(args->net_cfg.server_addr, sizeof(args->net_cfg.server_addr), optarg);
                break;

                /* server port */
            case 'p':
                printf("option port :%s\n", optarg);
                args->net_cfg.server_port = atoi(optarg);
                break;

                /* congestion control */
            case 'c':
                printf("option cong_ctl :%s\n", optarg);
                /* r:reno b:bbr c:cubic */
                switch (*optarg) {
                    case 'b':
                        args->net_cfg.cc = CC_TYPE_BBR;
                        break;
                    case 'c':
                        args->net_cfg.cc = CC_TYPE_CUBIC;
                        break;
                    case 'r':
                        args->net_cfg.cc = CC_TYPE_RENO;
                        break;
                    default:
                        break;
                }
                break;

                /* pacing */
            case 'C':
                printf("option pacing :%s\n", "on");
                args->net_cfg.pacing = 1;
                break;

                /* idle persist timeout */
            case 't':
                printf("option connection timeout :%s\n", optarg);
                args->net_cfg.conn_timeout = atoi(optarg);
                break;

                /* ssl cipher suites */
            case 'S':
                printf("option cipher suites: %s\n", optarg);
                args->quic_cfg.cipher_suites = optarg;
                break;

                /* 0rtt option */
            case '0':
                printf("option 0rtt\n");
                args->quic_cfg.use_0rtt = 1;
                break;

                /* multi connections */
            case 'm':
                printf("option multi connection: on\n");
                switch (atoi(optarg)) {
                    case 0:
                        args->net_cfg.mode = MODE_SCMR;
                        break;
                    case 1:
                        args->net_cfg.mode = MODE_SCSR_SERIAL;
                        break;
                    case 2:
                        args->net_cfg.mode = MODE_SCSR_CONCURRENT;
                    default:
                        break;
                }
                break;

                /* alpn */
            case 'A':
                printf("option set ALPN[%s]\n", optarg);
                if (strcmp(optarg, "h3") == 0) {
                    args->quic_cfg.alpn_type = ALPN_H3;
                    strncpy(args->quic_cfg.alpn, "h3", 3);

                } else if (strcmp(optarg, "hq") == 0) {
                    args->quic_cfg.alpn_type = ALPN_HQ;
                    strncpy(args->quic_cfg.alpn, "hq-interop", 11);
                }

                break;

                /* out file directory */
            case 'D':
                printf("option save body dir: %s\n", optarg);
                strncpy(args->env_cfg.out_file_dir, optarg, sizeof(args->env_cfg.out_file_dir) - 1);
                break;

                /* log level */
            case 'l':
                printf("option log level :%s\n", optarg);
                /* e:error d:debug */
                args->env_cfg.log_level = optarg[0];
                break;

                /* log directory */
            case 'L':
                printf("option log directory :%s\n", optarg);
                strncpy(args->env_cfg.log_path, optarg, sizeof(args->env_cfg.log_path) - 1);
                break;

                /* key out path */
            case 'k':
                printf("key output file: %s\n", optarg);
                args->env_cfg.key_output_flag = 1;
                strncpy(args->env_cfg.key_out_path, optarg, sizeof(args->env_cfg.key_out_path) - 1);
                break;

                /* client life time circle */
            case 'K':
                printf("client life circle time: %s\n", optarg);
                args->env_cfg.life = atoi(optarg);
                break;

                /* request urls */
            case 'U': // request URL, address is parsed from the request
                printf("option url only:%s\n", optarg);
                xqc_demo_cli_parse_urls(optarg, args);
                break;

                /* key update packet threshold */
            case 'u':
                printf("key update packet threshold: %s\n", optarg);
                args->quic_cfg.keyupdate_pkt_threshold = atoi(optarg);
                break;

            default:
                printf("other option :%c\n", ch);
                xqc_demo_cli_usage(argc, argv);
                exit(0);
        }
    }
}

int
main(int argc, char *argv[]) {
    /* get input client args */

    xqc_demo_cli_client_args_t *args = calloc(1, sizeof(xqc_demo_cli_client_args_t));
    xqc_demo_cli_init_args(args);
    xqc_demo_cli_parse_args(argc, argv, args);
//
//    /* init client ctx */
//    xqc_demo_cli_ctx_t *ctx = calloc(1, sizeof(xqc_demo_cli_ctx_t));
//    xqc_demo_cli_init_ctx(ctx, args);
//
//    /* engine event */
//    ctx->eb = event_base_new();
//    ctx->ev_engine = event_new(ctx->eb, -1, 0, xqc_demo_cli_engine_callback, ctx);
//    xqc_demo_cli_init_xquic_engine(ctx, args);
//
//    /* start task scheduler */
//    xqc_demo_cli_start_task_manager(ctx);
//
//    event_base_dispatch(ctx->eb);
//
//    xqc_engine_destroy(ctx->engine);
//    xqc_demo_cli_free_ctx(ctx);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>

#define SERVER_PORT               7812

#define THREAD_REQUEST_EXIT       1
#define CLIENT_CON_CHECK_TIME     3
#define CLIENT_NOT_VERIFIED_TO_S  10

#define FRAME_MARKER  0x55

typedef enum {
    CLIENT_CONNECTED,
    CLIENT_VERIFIED,
}client_cond_state_t;

typedef struct {
    int sockfd;
    uint8_t exit_req;
    int newsockfd;
    int port;
    struct sockaddr_in addr;

} server_info_t;

typedef struct {
    int socket_fd;
    pthread_t wd_thread;
    pthread_t thread;
    uint32_t spawn_id;
    char target_machine_user[30];
    char target_machine_password[30];
    char target_ip[15];
    bool client_exit;
    uint8_t not_verified_timeout;
    server_info_t *server_info;
    client_cond_state_t state;
} client_info_t;

static void load_client_defaults(client_info_t *client_info,
    server_info_t *server_info,
    int socket_fd)
{
    client_info->server_info = server_info;
    client_info->socket_fd = socket_fd;
    client_info->client_exit = false;
    client_info->state = CLIENT_CONNECTED;
    client_info->not_verified_timeout = 0;
}
/** thr_client_wd
 *
 * Description:
 *          Watch dog thread for client thread
 *
 */
void *thr_client_wd(void *context)
{
    int con_check_time = 0;
    client_info_t *client_info = context;

    while (client_info->client_exit == false) {
        sleep(1);
        switch (client_info->state) {
            case CLIENT_CONNECTED:
                if (client_info->not_verified_timeout < CLIENT_NOT_VERIFIED_TO_S) {
                    client_info->not_verified_timeout ++;
                } else {
                    printf("CLIENT_CONNECTED timed out\n");
                    client_info->client_exit = true;
                }
                break;
            case CLIENT_VERIFIED:
                con_check_time ++;
                if (con_check_time < CLIENT_CON_CHECK_TIME) {
                    printf("Check client connectivity\n");
                    con_check_time = 0;
                }
                break;
        }
    }
    return NULL;
}

/* clients will spawn this thread*/
void *thr_client(void *context)
{
    uint8_t buffer[255];
    int n;
    client_info_t *client_info = context;

    if (!client_info) {
        printf("[%s: %d] no input parametters\n", __func__, __LINE__);
        return NULL;
    }

    pthread_create(&client_info->wd_thread,
        NULL, &thr_client_wd, (void *)client_info);

    while (client_info->client_exit == false) {
       n = read(client_info->socket_fd, buffer, 1);
       if (n == 1) {
         if (buffer[0] == FRAME_MARKER) {

         }
       }
    }

    printf("[%s: %d] client thread created\n", __func__, __LINE__);
    printf("[%s: %d] Now try to exit\n", __func__, __LINE__);

    pthread_join(client_info->wd_thread, NULL);
    client_info->server_info->exit_req = 1;
    return NULL;
}

/* server spawn this thread, only one instance */
void *my_server(void *args)
{
    int rc;
    struct sockaddr_in client_addr;
    server_info_t *server_info = args;
    client_info_t *client_info;
    int socketfd;
    socklen_t clilen;

    if (!args) {
        printf("[%s: %d]No input parametters", __func__, __LINE__);
    }

    printf("[%s: %d] server started\n", __func__, __LINE__);
    server_info->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (!server_info->sockfd) {
        printf("[%s: %d] cannot create sockets\n", __func__, __LINE__);
        return NULL;
    }

    bzero((char *)&server_info->addr, sizeof(struct sockaddr_in));
    server_info->addr.sin_family = AF_INET;
    server_info->addr.sin_addr.s_addr = INADDR_ANY;
    server_info->addr.sin_port = htons(server_info->port);

    rc = bind(server_info->sockfd,
        (struct sockaddr *)&server_info->addr,
        sizeof(struct sockaddr_in));
    if(rc < 0) {
        printf("[%s: %d] Bind socket failed\n", __func__, __LINE__);
        return NULL;
    }

    clilen = sizeof(struct sockaddr_in);
    printf("[%s: %d] Starting main loop\n", __func__, __LINE__);
    while (!server_info->exit_req) {
        printf("[%s: %d] Wait for client\n", __func__, __LINE__);
        listen(server_info->sockfd, 5);

        socketfd = accept(server_info->sockfd,
            (struct sockaddr*) &client_addr,
            &clilen);

        if(socketfd) {
            printf("[%s: %d] Client Connected.Try to create client thread\n", __func__, __LINE__);
            client_info = NULL;
            /*TODO: Add client_info in lists */
            client_info = (client_info_t*)malloc(sizeof(client_info_t));
            if (client_info) {
                load_client_defaults(client_info, server_info, socketfd);
                pthread_create(&client_info->thread,
                    NULL, &thr_client, (void *)client_info);
            }
        }
    }

    printf("[%s: %d] Exiting\n", __func__, __LINE__);
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t thr_server;
    server_info_t *server_info;
    server_info = (server_info_t *)malloc(sizeof(server_info_t));
    server_info->port = SERVER_PORT;
    server_info->exit_req = 0;
    printf("Starting server...");
    pthread_create(&thr_server, NULL, &my_server, (void *)server_info);
    printf("done\n");
    pthread_join(thr_server, NULL);

    return 0;
}

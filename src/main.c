#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>

#define SERVER_PORT 7812

#define THREAD_REQUEST_EXIT 1

typedef struct {
    int sockfd;
    uint8_t exit_req;
    int newsockfd;
    int port;
    struct sockaddr_in addr;
} server_info_t;

typedef struct {
    pthread_t thread;
    uint32_t spawn_id;
    char target_machine_user[30];
    char target_machine_password[30];
    char target_ip[15];
    server_info_t *server_info;
} client_info_t;

/* clients will spawn this thread*/
void *thr_client(void *context)
{
    client_info_t *client_info = context;

    if (!client_info) {
        printf("[%s: %d] no input parametters\n", __func__, __LINE__);
        return NULL;
    }

    printf("[%s: %d] client thread created\n", __func__, __LINE__);
    printf("[%s: %d] Now try to exit\n", __func__, __LINE__);

    client_info->server_info->exit_req = 1;
    return NULL;
}

/* server spawn this thread, only
one instance */
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
            client_info = (client_info_t*)malloc(sizeof(client_info_t));
            if (client_info) {
                client_info->server_info = server_info;
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

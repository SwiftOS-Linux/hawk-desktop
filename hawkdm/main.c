#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "scall.h"

bool scall_running = false;

void termination_handler(int signum) {
    scall_running = false;

    // Clean up: close the sockets and remove the socket file.
    close(conn_fd);
    close(listen_fd);
    unlink(SOCKET_PATH);
}

int scall_init() {
    int listen_fd, conn_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];

    // Bug Unable To Create File [Fixed]: Clean up old socket file if it exists.
    unlink(PIPELINE_PATH);

    // Create a Unix domain socket.
    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        pthread_exit((void*)2);
    }

    // Bind the socket to a file path.
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, PIPELINE_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        pthread_exit((void*)5);
    }

    while(scall_running) {
        // Listen for incoming connections.
        if (listen(listen_fd, BACKLOG) == -1) {
            pthread_exit((void*)1);
        }
        printf("Server listening on socket %s...\n", SOCKET_PATH);

        // Accept a client connection.
        client_len = sizeof(client_addr);
        conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if (conn_fd == -1) {
            pthread_exit((void*)11);
        }
        printf("Client connected.\n");

        // Read data from the client.
        ssize_t bytes_read = recv(conn_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read == -1) {
            perror("recv");
        } else {
            //buffer[bytes_read] = '\0'; // Null-terminate the received data
            printf("Received message: %s\n", buffer);
        }
    }
}

int main() {
    pthread_t thread;
    scall_running = true;
    int result = pthread_create(&thread, NULL, scall_init, NULL);
    if(result == 2) {
        fprintf(stderr, "")
    }

    pthread_join(thread, NULL);
}
#include "scall.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

int broadcastMessage(const char *message) {
    int client_fd;
    struct sockaddr_un server_addr;

    // 1. Create a Unix domain socket.
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        return 7;
    }

    // 2. Specify the server's address (file path).
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, PIPELINE_PATH, sizeof(server_addr.sun_path) - 1);

    // 3. Connect to the server socket.
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(client_fd);
        return 2;
    }

    // 4. Send data to the server.
    if (send(client_fd, message, strlen(message), 0) == -1) {
        return 5;
    }

    close(client_fd);
    return 0;
}
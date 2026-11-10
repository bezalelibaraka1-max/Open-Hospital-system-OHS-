
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "ftp.h"


#define PORT 8080
#define BUFFER_SIZE 4096

int send_file(char *path)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    FILE *fp;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for connection...\n");
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("Client connected!\n");

    // Open file
    fp = fopen(path, "rb");
    if (!fp) {
        perror("File not found");
        close(new_socket);
        return 1;
    }

    // Send file in chunks
    int n;
    while ((n = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        send(new_socket, buffer, n, 0);
    }

    printf("File sent successfully!\n");

    fclose(fp);
    close(new_socket);
    close(server_fd);
}
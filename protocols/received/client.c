/*
 * client.c
 *
 * Bidirectional threaded file-transfer client.
 *
 * Compile:
 *     gcc client.c -o client -Wall -Wextra -pthread
 *
 * Run:
 *     ./client <server-ip> <port>
 *
 * Example:
 *     ./client 192.168.1.10 9000
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define BUFFER_SIZE 8192
#define MAX_FILENAME 4096

static uint64_t htonll(uint64_t value)
{
    static const int num = 1;

    if (*(const char *)&num == 1) {
        return ((uint64_t)htonl(value & 0xffffffffULL) << 32) |
               htonl(value >> 32);
    }

    return value;
}

static uint64_t ntohll(uint64_t value)
{
    static const int num = 1;

    if (*(const char *)&num == 1) {
        return ((uint64_t)ntohl(value & 0xffffffffULL) << 32) |
               ntohl(value >> 32);
    }

    return value;
}

static int send_all(int sock, const void *buffer, size_t length)
{
    const char *ptr = buffer;

    while (length > 0) {

        ssize_t sent = send(sock, ptr, length, 0);

        if (sent <= 0)
            return -1;

        ptr += sent;
        length -= sent;
    }

    return 0;
}

static int recv_all(int sock, void *buffer, size_t length)
{
    char *ptr = buffer;

    while (length > 0) {

        ssize_t received = recv(sock, ptr, length, 0);

        if (received <= 0)
            return -1;

        ptr += received;
        length -= received;
    }

    return 0;
}

static const char *basename_only(const char *path)
{
    const char *slash = strrchr(path, '/');

    if (slash)
        return slash + 1;

    return path;
}

static int send_file(int sock, const char *filename)
{
    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("fopen");
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(file);
        return -1;
    }

    long size = ftell(file);

    if (size < 0) {
        perror("ftell");
        fclose(file);
        return -1;
    }

    rewind(file);

    const char *name = basename_only(filename);

    uint32_t name_length = strlen(name);
    uint64_t file_size = (uint64_t)size;

    if (name_length == 0 || name_length > MAX_FILENAME) {
        fprintf(stderr, "Invalid filename.\n");
        fclose(file);
        return -1;
    }

    uint32_t net_name_length = htonl(name_length);

    if (send_all(sock,
                 &net_name_length,
                 sizeof(net_name_length)) < 0) {
        perror("send");
        fclose(file);
        return -1;
    }

    uint64_t net_file_size = htonll(file_size);

    if (send_all(sock,
                 &net_file_size,
                 sizeof(net_file_size)) < 0) {
        perror("send");
        fclose(file);
        return -1;
    }

    if (send_all(sock, name, name_length) < 0) {
        perror("send");
        fclose(file);
        return -1;
    }

    char buffer[BUFFER_SIZE];

    uint64_t remaining = file_size;

    while (remaining > 0) {

        size_t amount =
            remaining > BUFFER_SIZE
            ? BUFFER_SIZE
            : (size_t)remaining;

        size_t bytes_read =
            fread(buffer, 1, amount, file);

        if (bytes_read == 0) {
            fclose(file);
            return -1;
        }

        if (send_all(sock, buffer, bytes_read) < 0) {
            perror("send file");
            fclose(file);
            return -1;
        }

        remaining -= bytes_read;
    }

    fclose(file);

    printf("[SEND] %s (%llu bytes)\n",
           name,
           (unsigned long long)file_size);

    return 0;
}

static int receive_file(int sock)
{
    uint32_t net_name_length;
    uint64_t net_file_size;

    if (recv_all(sock,
                 &net_name_length,
                 sizeof(net_name_length)) < 0)
        return -1;

    uint32_t name_length = ntohl(net_name_length);

    if (name_length == 0 || name_length > MAX_FILENAME)
        return -1;

    if (recv_all(sock,
                 &net_file_size,
                 sizeof(net_file_size)) < 0)
        return -1;

    uint64_t file_size = ntohll(net_file_size);

    char filename[MAX_FILENAME + 1];

    if (recv_all(sock,
                 filename,
                 name_length) < 0)
        return -1;

    filename[name_length] = '\0';

    const char *safe_name = basename_only(filename);

    mkdir("received", 0755);

    char output_path[MAX_FILENAME + 32];

    snprintf(output_path,
             sizeof(output_path),
             "received/%s",
             safe_name);

    FILE *file = fopen(output_path, "wb");

    if (!file) {
        perror("fopen");
        return -1;
    }

    char buffer[BUFFER_SIZE];

    uint64_t remaining = file_size;

    while (remaining > 0) {

        size_t amount =
            remaining > BUFFER_SIZE
            ? BUFFER_SIZE
            : (size_t)remaining;

        ssize_t received =
            recv(sock, buffer, amount, 0);

        if (received <= 0) {
            fclose(file);
            return -1;
        }

        if (fwrite(buffer,
                   1,
                   received,
                   file) != (size_t)received) {

            fclose(file);
            return -1;
        }

        remaining -= received;
    }

    fclose(file);

    printf("\n[RECEIVED] %s (%llu bytes)\n",
           output_path,
           (unsigned long long)file_size);

    return 0;
}

static void *receive_thread(void *argument)
{
    int sock = *(int *)argument;

    while (1) {

        if (receive_file(sock) < 0) {
            printf("\nConnection closed.\n");
            break;
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <server-ip> <port>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET,
                  server_ip,
                  &server_addr.sin_addr) <= 0) {

        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {

        perror("connect");
        close(sock);
        return 1;
    }

    printf("Connected to %s:%d\n",
           server_ip,
           port);

    pthread_t receiver;

    pthread_create(&receiver,
                   NULL,
                   receive_thread,
                   &sock);

    char filename[MAX_FILENAME];

    while (1) {

        printf("\nEnter filename to send (or 'quit'): ");

        if (!fgets(filename,
                   sizeof(filename),
                   stdin))
            break;

        filename[strcspn(filename, "\n")] = '\0';

        if (strcmp(filename, "quit") == 0)
            break;

        if (strlen(filename) == 0)
            continue;

        send_file(sock, filename);
    }

    close(sock);

    pthread_cancel(receiver);
    pthread_join(receiver, NULL);

    return 0;
}
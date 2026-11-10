/*
 * server.c
 *
 * Bidirectional threaded file-transfer server.
 *
 * The server can:
 *   - receive files
 *   - send files
 *   - do both independently
 *
 * Compile:
 *     gcc server.c -o server -Wall -Wextra -pthread
 *
 * Run:
 *     ./server 9000
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "communi.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define BUFFER_SIZE 8192
#define MAX_FILENAME 4096
#define BACKLOG 10



/* ---------------------------------------------------------
 * Convert uint64_t between host and network byte order.
 *
 * There is no standard htonll() on every Linux system,
 * so we implement it ourselves.
 * --------------------------------------------------------- */

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

/* ---------------------------------------------------------
 * send_all()
 *
 * send() may send fewer bytes than requested.
 * Keep calling it until everything is transmitted.
 * --------------------------------------------------------- */

static int send_all(int sock, const void *buffer, size_t length)
{
    const char *ptr = buffer;

    while (length > 0) {

        ssize_t sent = send(sock, ptr, length, 0);

        if (sent <= 0) {
            return -1;
        }

        ptr += sent;
        length -= sent;
    }

    return 0;
}

/* ---------------------------------------------------------
 * recv_all()
 *
 * recv() may return fewer bytes than requested.
 * Keep receiving until the requested amount arrives.
 * --------------------------------------------------------- */

static int recv_all(int sock, void *buffer, size_t length)
{
    char *ptr = buffer;

    while (length > 0) {

        ssize_t received = recv(sock, ptr, length, 0);

        if (received <= 0) {
            return -1;
        }

        ptr += received;
        length -= received;
    }

    return 0;
}

/* ---------------------------------------------------------
 * Get only the filename portion.
 *
 * This prevents a sender from sending:
 *
 *     ../../something
 *
 * and writing outside our receive directory.
 * --------------------------------------------------------- */

static const char *basename_only(const char *path)
{
    const char *slash = strrchr(path, '/');

    if (slash)
        return slash + 1;

    return path;
}

/* ---------------------------------------------------------
 * SEND FILE
 * --------------------------------------------------------- */

static int send_file(int sock, const char *filename)
{
    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("fopen");
        return -1;
    }

    /* Find file size */

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
    //we cast size from long to uint64_t because long type varies based on system.

    if (name_length == 0 || name_length > MAX_FILENAME) {
        fprintf(stderr, "Invalid filename.\n");
        fclose(file);
        return -1;
    }

    /* Send filename length */

    uint32_t net_name_length = htonl(name_length);

    if (send_all(sock,
        &net_name_length,
        sizeof(net_name_length)) < 0) {
        perror("send filename length");
    fclose(file);
    return -1;
        }

        /* Send file size */

        uint64_t net_file_size = htonll(file_size);

        if (send_all(sock,
            &net_file_size,
            sizeof(net_file_size)) < 0) {
            perror("send file size");
        fclose(file);
        return -1;
            }

            /* Send filename */

            if (send_all(sock, name, name_length) < 0) {
                perror("send filename");
                fclose(file);
                return -1;
            }

            /* Send file contents */

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
                    if (ferror(file))
                        perror("fread");

                    fclose(file);
                    return -1;
                }

                if (send_all(sock, buffer, bytes_read) < 0) {
                    perror("send file data");
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

/* ---------------------------------------------------------
 * RECEIVE FILE
 * --------------------------------------------------------- */

static int receive_file(int sock)
{
    uint32_t net_name_length;
    uint64_t net_file_size;

    /* Receive filename length */

    if (recv_all(sock,
        &net_name_length,
        sizeof(net_name_length)) < 0) {
        return -1;
        }

        uint32_t name_length = ntohl(net_name_length);

    if (name_length == 0 || name_length > MAX_FILENAME) {
        fprintf(stderr, "Invalid filename length.\n");
        return -1;
    }

    /* Receive file size */

    if (recv_all(sock,
        &net_file_size,
        sizeof(net_file_size)) < 0) {
        return -1;
        }

        uint64_t file_size = ntohll(net_file_size);

    /* Receive filename */

    char filename[MAX_FILENAME + 1];

    if (recv_all(sock,
        filename,
        name_length) < 0) {
        return -1;
        }

        filename[name_length] = '\0';

    /*
     * Remove any directory components.
     */

    const char *safe_name = basename_only(filename);

    /* Create received directory */

    mkdir("received", 0755);

    char output_path[MAX_FILENAME + 32];

    snprintf(output_path,
             sizeof(output_path),
             "received/%s",
             safe_name);

    FILE *file = fopen(output_path, "wb");

    if (!file) {
        perror("fopen received file");
        return -1;
    }

    /* Receive file data */

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
            perror("recv file data");
            fclose(file);
            return -1;
        }

        size_t written =
        fwrite(buffer, 1, received, file);

        if (written != (size_t)received) {
            perror("fwrite");
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

/* ---------------------------------------------------------
 * RECEIVING THREAD
 *
 * This thread continuously waits for files from the peer.
 * --------------------------------------------------------- */

static void *receive_thread(void *argument)
{
    int sock = *(int *)argument;

    while (1) {

        if (receive_file(sock) < 0) {
            printf("\nConnection closed or receive error.\n");
            break;
        }
    }

    return NULL;
}

int communicate(int port,char*name)
{


    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0) {
        perror("socket");
        return 1;
    }

    /* Allow quick reuse of port */

    int option = 1;

    setsockopt(server_sock,
               SOL_SOCKET,
               SO_REUSEADDR,
               &option,
               sizeof(option));

    /* Server address */

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    /* Bind */

    if (bind(server_sock,
        (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {

        perror("bind");
    close(server_sock);
    return 1;
             }

             /* Listen */

             if (listen(server_sock, BACKLOG) < 0) {
                 perror("listen");
                 close(server_sock);
                 return 1;
             }

             printf("Server listening on port %d...\n", port);

             /* Accept */

             struct sockaddr_in client_addr;
             socklen_t client_length = sizeof(client_addr);

             int client_sock =
             accept(server_sock,
                    (struct sockaddr *)&client_addr,
                    &client_length);

             if (client_sock < 0) {
                 perror("accept");
                 close(server_sock);
                 return 1;
             }

             printf("Connected to %s:%d\n",
                    inet_ntoa(client_addr.sin_addr),
                    ntohs(client_addr.sin_port));

             /*
              * and next upgrade is to make the status to the doctor to notify that receiver is online or offline
              * Start receiving thread.
              */

             pthread_t receiver;

             pthread_create(&receiver,
                            NULL,
                            receive_thread,
                            &client_sock);

             /*
              * Main thread handles sending.
              */

            char filename[MAX_FILENAME];

            strcpy(filename,name);

            filename[strcspn(filename, "\n")] = '\0';

            if (strcmp(filename, "") == 0||strlen(filename) == 0)
            {printf("empty filename is not allowed\n");}

             send_file(client_sock, filename);

             close(client_sock);
             close(server_sock);

             pthread_cancel(receiver);
             pthread_join(receiver, NULL);

             return 0;
}

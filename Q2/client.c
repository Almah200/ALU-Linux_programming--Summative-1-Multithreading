#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/inotify.h>

#define PORT_NUMBER 8080
#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))
#define MAX_FILES 100
#define MAX_FILE_NAME_SIZE 100

char activeFiles[MAX_FILES][MAX_FILE_NAME_SIZE];
int activeFilesCount = 0;

void *sendFileActivity(void *arg);
void *trackFileActivity(void *arg);
void sendActiveFiles(int socket);

int main() {
    int socket_fd = 0;
    struct sockaddr_in server_address;
    pthread_t track_thread, send_thread;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation error");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);

    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&track_thread, NULL, trackFileActivity, (void *)&socket_fd) != 0) {
        perror("pthread_create for trackFileActivity failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&send_thread, NULL, sendFileActivity, (void *)&socket_fd) != 0) {
        perror("pthread_create for sendFileActivity failed");
        exit(EXIT_FAILURE);
    }

    pthread_join(track_thread, NULL);
    pthread_join(send_thread, NULL);

    close(socket_fd);

    return 0;
}

void sendActiveFiles(int socket) {
    char buffer[1024] = {0};
    for (int i = 0; i < activeFilesCount; ++i) {
        strcat(buffer, activeFiles[i]);
        strcat(buffer, "\n");
    }
    send(socket, buffer, strlen(buffer), 0);
    memset(buffer, 0, sizeof(buffer));
    activeFilesCount = 0;
}

void *sendFileActivity(void *arg) {
    int socket = *((int *)arg);

    while (1) {
        sendActiveFiles(socket);
        sleep(3);
    }

    return NULL;
}

void *trackFileActivity(void *arg) {
    int socket = *((int *)arg);

    int inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        perror("inotify_init");
    }

    int watch_descriptor = inotify_add_watch(inotify_fd, "/home/username", IN_OPEN | IN_MOVED_TO | IN_CREATE);

    char buffer[BUF_LEN] = {0};
    int length, i = 0;

    while (1) {
        i = 0;
        length = read(inotify_fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
        }

        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->len) {
                int exists = 0;
                for (int j = 0; j < activeFilesCount; j++) {
                    if (strcmp(activeFiles[j], event->name) == 0) {
                        exists = 1;
                        break;
                    }
                }
                if (exists == 0 && activeFilesCount < MAX_FILES) {
                    strcpy(activeFiles[activeFilesCount], event->name);
                    activeFilesCount++;
                }
                i += EVENT_SIZE + event->len;
            }
        }
    }

    inotify_rm_watch(inotify_fd, watch_descriptor);
    close(inotify_fd);
    return NULL;
}

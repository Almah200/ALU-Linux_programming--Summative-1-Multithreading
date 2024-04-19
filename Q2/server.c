#include <stdio.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PORT_NUMBER 8080

void printReceivedURLs(const char *data) {
    char *dataCopy = strdup(data);
    if (dataCopy == NULL) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    char *delimiter = "\n";
    char *token = strtok(dataCopy, delimiter);
    while (token != NULL) {
        printf("Received URL: %s\n", token);
        token = strtok(NULL, delimiter);
    }

    free(dataCopy);
}

void *receiveData(void *arg) {
    int clientSocket = *(int *)arg;
    char receivedData[1024] = {0};

    while (1) {
        memset(receivedData, 0, sizeof(receivedData));
        if (recv(clientSocket, receivedData, sizeof(receivedData), 0) <= 0) {
            perror("recv failed");
            exit(EXIT_FAILURE);
        }
        printReceivedURLs(receivedData);
    }

    return NULL;
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress;
    int option = 1;
    int addressLength = sizeof(serverAddress);
    char receivedData[1024] = {0};

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT_NUMBER);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((clientSocket = accept(serverSocket, (struct sockaddr *)&serverAddress, (socklen_t*)&addressLength))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    pthread_t receiveThread;
    if (pthread_create(&receiveThread, NULL, receiveData, (void *)&clientSocket) != 0) {
        perror("pthread_create for receiveData failed");
        exit(EXIT_FAILURE);
    }

    pthread_join(receiveThread, NULL);

    return 0;
}

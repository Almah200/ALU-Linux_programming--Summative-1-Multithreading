#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 9999
#define BUFFER_SIZE 1024
#define MAX_RETRIES 3
#define RETRY_DELAY_SEC 1

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    int retries = 0;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Main loop
    while (1) {
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

        // Send message
        if (send(client_socket, buffer, strlen(buffer), 0) <= 0) {
            perror("send");
            break;
        }

        // Wait for acknowledgment
        char ack[3];
        while (1) {
            bytes_received = recv(client_socket, ack, sizeof(ack), 0);
            if (bytes_received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    if (retries < MAX_RETRIES) {
                        retries++;
                        sleep(RETRY_DELAY_SEC);
                        continue; // Retry receiving acknowledgment
                    } else {
                        perror("recv");
                        break; // Maximum retries exceeded
                    }
                } else {
                    perror("recv");
                    break; // Other receive error
                }
            } else if (bytes_received == 0) {
                printf("Server disconnected\n");
                break; // Server disconnected
            } else {
                break; // Acknowledgment received
            }
        }
        
        if (bytes_received <= 0) {
            break; // Exit loop if there was an error receiving acknowledgment
        }
        
        if (strcmp(ack, "ACK") != 0) {
            printf("Error: Did not receive acknowledgment\n");
            break;
        } else {
            printf("Acknowledgment received\n");
        }

        // Clear buffer
        memset(buffer, 0, sizeof(buffer));
    }

    close(client_socket);
    return 0;
}

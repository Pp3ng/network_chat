#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 9527
#define BUFFER_SIZE 1024

void check_error(int n, const char *error_message)
{
    if (n < 0)
    {
        perror(error_message);
        exit(EXIT_FAILURE);
    }
}

void *receive_messages(void *arg)
{
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int n;

    while ((n = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        buffer[n] = '\0';
        printf("%s\n", buffer);
    }

    if (n == 0)
    {
        printf("Server has closed the connection\n");
        close(sock);
        exit(0);
    }

    return NULL;
}
int main()
{
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    pthread_t thread_id;
    char username[50];

    check_error((sock = socket(AF_INET, SOCK_STREAM, 0)), "Socket creation failed");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    check_error(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr), "Invalid address");

    check_error(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)), "Connection failed");

    // Input and send username
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    send(sock, username, strlen(username), 0);

    pthread_create(&thread_id, NULL, receive_messages, (void *)&sock);
    pthread_detach(thread_id);

    while (1)
    {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "/quit") == 0)
        {
            printf("Exiting...\n");
            close(sock);
            exit(0);
        }

        // Handle private message command
        if (strncmp(buffer, "/w ", 3) == 0)
        {
            // Directly send the buffer as it contains the '/w ' command
            send(sock, buffer, strlen(buffer), 0);
        }
        else
        {
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    close(sock);
    return 0;
}
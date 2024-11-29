#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

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
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        printf("\r[%02d:%02d:%02d] %s\n> ", t->tm_hour, t->tm_min, t->tm_sec, buffer);
        fflush(stdout);
    }

    if (n == 0)
    {
        printf("\rServer has closed the connection\n");
        fflush(stdout);
        close(sock);
        exit(0);
    }

    return NULL;
}

void print_help()
{
    printf("Available commands:\n");
    printf("/quit - Exit the chat\n");
    printf("/w <username> <message> - Send a private message\n");
    printf("/list - List all connected users\n");
    printf("/clear - Clear the screen\n");
    printf("/help - Show this help message\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    pthread_t thread_id;
    char username[50];
    char channel[10];

    check_error((sock = socket(AF_INET, SOCK_STREAM, 0)), "Socket creation failed");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    check_error(inet_pton(AF_INET, server_ip, &server_addr.sin_addr), "Invalid address");

    check_error(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)), "Connection failed");

    // Input and send username
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    send(sock, username, strlen(username), 0);

    // Input and send channel number
    printf("Enter channel number: ");
    fgets(channel, sizeof(channel), stdin);
    channel[strcspn(channel, "\n")] = 0;
    send(sock, channel, strlen(channel), 0);

    pthread_create(&thread_id, NULL, receive_messages, (void *)&sock);
    pthread_detach(thread_id);

    print_help();

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
        else if (strcmp(buffer, "/help") == 0)
        {
            print_help();
        }
        else if (strcmp(buffer, "/clear") == 0)
        {
            printf("\033[H\033[J"); // ANSI escape code to clear the screen
        }
        else
        {
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    close(sock);
    return 0;
}
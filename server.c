#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 9527
#define BUFFER_SIZE 1024
#define MAX_CHAT_HISTORY 1000
#define MAX_CLIENTS 100
#define MAX_USERNAME_LENGTH 50
#define LOG_FILE "server_log.txt"
#define CHAT_HISTORY_FILE "chat_history.txt"

typedef struct
{
    int socket;
    char username[MAX_USERNAME_LENGTH];
    int logged_in;
    int channel;
} Client;

Client *clients = NULL;
int client_count = 0;
int logged_in_clients = 0;
char chat_history[MAX_CHAT_HISTORY][BUFFER_SIZE + MAX_USERNAME_LENGTH];
int chat_count = 0;
pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t chat_history_mutex = PTHREAD_MUTEX_INITIALIZER;

// Logging function
void log_message(const char *message)
{
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL)
    {
        perror("Unable to open log file");
        return;
    }
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline character
    fprintf(log_file, "[%s] %s\n", timestamp, message);
    fclose(log_file);
}

void check_error(int n, const char *error_message)
{
    if (n < 0)
    {
        perror(error_message);
        log_message(error_message);
        exit(EXIT_FAILURE);
    }
}

void add_client(int client_socket, const char *username, int channel)
{
    pthread_mutex_lock(&client_list_mutex);
    clients = realloc(clients, sizeof(Client) * (client_count + 1));
    clients[client_count].socket = client_socket;
    strncpy(clients[client_count].username, username, sizeof(clients[client_count].username) - 1); // copy username to client
    clients[client_count].username[sizeof(clients[client_count].username) - 1] = '\0';
    clients[client_count].logged_in = 1;
    clients[client_count].channel = channel;
    client_count++;
    logged_in_clients++;
    char log_msg[BUFFER_SIZE];
    snprintf(log_msg, sizeof(log_msg), "User %s logged in on channel %d. Currently logged in clients: %d", username, channel, logged_in_clients);
    log_message(log_msg);
    printf("%s\n", log_msg);
    pthread_mutex_unlock(&client_list_mutex);
}

void remove_client(int client_socket)
{
    pthread_mutex_lock(&client_list_mutex);
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].socket == client_socket)
        {
            char username[MAX_USERNAME_LENGTH];
            strncpy(username, clients[i].username, sizeof(username) - 1);
            username[sizeof(username) - 1] = '\0';

            clients[i] = clients[client_count - 1];
            client_count--;
            logged_in_clients--;
            clients = realloc(clients, sizeof(Client) * client_count);
            char log_msg[BUFFER_SIZE];
            snprintf(log_msg, sizeof(log_msg), "User %s disconnected. Currently logged in clients: %d", username, logged_in_clients);
            log_message(log_msg);
            printf("%s\n", log_msg);
            break;
        }
    }
    pthread_mutex_unlock(&client_list_mutex);
}

void broadcast_message(const char *message, int exclude_socket, int channel)
{
    pthread_mutex_lock(&client_list_mutex);
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].socket != exclude_socket && clients[i].channel == channel)
        {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&client_list_mutex);
}

// Handles private messages
void private_message(const char *message, const char *target_username, int exclude_socket)
{
    pthread_mutex_lock(&client_list_mutex);
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].socket != exclude_socket && strcmp(clients[i].username, target_username) == 0)
        {
            send(clients[i].socket, message, strlen(message), 0);
            break;
        }
    }
    pthread_mutex_unlock(&client_list_mutex);
}

void list_users(int client_socket, int channel)
{
    pthread_mutex_lock(&client_list_mutex);
    char user_list[BUFFER_SIZE] = "Connected users in your channel:\n";
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].channel == channel)
        {
            strcat(user_list, clients[i].username);
            strcat(user_list, "\n");
        }
    }
    pthread_mutex_unlock(&client_list_mutex);
    send(client_socket, user_list, strlen(user_list), 0);
}

void save_chat_history()
{
    FILE *chat_history_file = fopen(CHAT_HISTORY_FILE, "w");
    if (chat_history_file == NULL)
    {
        perror("Unable to open chat history file");
        pthread_mutex_unlock(&chat_history_mutex);
        return;
    }
    for (int i = 0; i < chat_count; i++)
    {
        fprintf(chat_history_file, "%s\n", chat_history[i]);
    }
    fclose(chat_history_file);
}

void *handle_client(void *arg)
{
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int n;
    char username[MAX_USERNAME_LENGTH];
    char channel_str[10];
    int channel;

    // Receive username
    n = recv(client_socket, username, sizeof(username), 0);
    if (n <= 0)
    {
        close(client_socket);
        return NULL;
    }
    username[n] = '\0';

    // Receive channel number
    n = recv(client_socket, channel_str, sizeof(channel_str), 0);
    if (n <= 0)
    {
        close(client_socket);
        return NULL;
    }
    channel_str[n] = '\0';
    channel = atoi(channel_str);

    add_client(client_socket, username, channel);

    // Message handling loop
    while ((n = recv(client_socket, buffer, sizeof(buffer), 0)) > 0)
    {
        buffer[n] = '\0';

        // Get current time
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

        // Handle private messages
        if (strncmp(buffer, "/w ", 3) == 0)
        {
            char target_username[MAX_USERNAME_LENGTH];
            sscanf(buffer + 3, "%s", target_username);
            char *private_msg = strchr(buffer + 3, ' ') + 1;
            char message_with_username[BUFFER_SIZE + MAX_USERNAME_LENGTH + 40];
            snprintf(message_with_username, sizeof(message_with_username), "(Private) [%s] %s: %s", timestamp, username, private_msg);
            private_message(message_with_username, target_username, client_socket);
        }
        else if (strcmp(buffer, "/list") == 0)
        {
            list_users(client_socket, channel);
        }
        else
        {
            // Add username and timestamp to the message
            char message_with_username[BUFFER_SIZE + MAX_USERNAME_LENGTH + 40];
            snprintf(message_with_username, sizeof(message_with_username), "[%s] %s: %s", timestamp, username, buffer);

            // Save chat history
            pthread_mutex_lock(&chat_history_mutex);
            if (chat_count < MAX_CHAT_HISTORY)
            {
                strncpy(chat_history[chat_count], message_with_username, sizeof(chat_history[chat_count]) - 1);
                chat_history[chat_count][sizeof(chat_history[chat_count]) - 1] = '\0';
                chat_count++;
            }
            save_chat_history();
            pthread_mutex_unlock(&chat_history_mutex);

            // Print chat history
            printf("%s\n", message_with_username);

            // Broadcast message to all clients in the same channel
            broadcast_message(message_with_username, client_socket, channel);
        }
    }

    remove_client(client_socket);
    close(client_socket);
    return NULL;
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    check_error((server_fd = socket(AF_INET, SOCK_STREAM, 0)), "Socket creation failed");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    check_error(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)), "Bind failed");

    check_error(listen(server_fd, 3), "Listen failed");

    printf("Server listening on port %d\n", PORT);
    char log_msg[BUFFER_SIZE];
    snprintf(log_msg, sizeof(log_msg), "Server started listening on port %d", PORT);
    log_message(log_msg);

    while (1)
    {
        check_error((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)), "Accept failed");

        pthread_mutex_lock(&client_list_mutex);
        if (client_count < MAX_CLIENTS)
        {
            pthread_create(&thread_id, NULL, handle_client, (void *)&new_socket);
            pthread_detach(thread_id);
        }
        else
        {
            fprintf(stderr, "Maximum clients reached. Rejecting new connection.\n");
            close(new_socket);
        }
        pthread_mutex_unlock(&client_list_mutex);
    }

    close(server_fd);
    free(clients);
    return 0;
}
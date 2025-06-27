# Simple Chat Server and Client in C

This is a simple chat server and client written in C. The server is multithreaded and can handle multiple clients at once. The server is able to broadcast messages to all connected clients. The client is able to send messages to the other clients and receive messages from them or private message to a specified client.

## Features

- **Multithreaded server**: The server is able to handle multiple clients at once.
- **Broadcast messages**: The server is able to broadcast messages to all connected clients.
- **Send and receive messages**: The client is able to send messages to the server and receive messages from the server.
- **Server logging**: The server logs all messages received from clients.
- **Server save chat history**: The server saves all messages received from clients in a file.
- **Private messages**: The client is able to send private messages to other clients.
- **Channel-based communication**: Clients can join specific channels and communicate within those channels.
- **List users in channel**: Clients can list all users in their current channel.

## Dependencies

- POSIX Threads (pthread)
- POSIX Sockets (arpa/inet.h)
- Standard C libraries (stdio.h, stdlib.h, string.h, unistd.h, time.h)

## Compilation

To compile the server and client, run the following command:

```bash
make client && make server
```

## Usage

- 1. Start the server by running the following command:

```bash
./server
```

- 2. Start the client by running the following command:

```bash
./client <server_ip>
```

- 3. Enter your username and channel number when prompted.
- 4. Start chatting!

## Example

```bash
$ ./client 127.0.0.1
Enter your username: user1
Enter channel number: 1
> Hello everyone!
> /list
Connected users in your channel:
user1
user2
> /w user2 Hi there!
> /quit
Exiting...
```

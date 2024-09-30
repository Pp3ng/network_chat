# Chat Server and Client

This is a simple chat server and client written in C. The server is multithreaded and can handle
multiple clients at once. The server is able to broadcast messages to all connected clients.The client
is able to send messages to the other clients and receive messages from them or private
message to a specified client.
## Features

- Multithreaded server: The server is able to handle multiple clients at once.
- Broadcast messages: The server is able to broadcast messages to all connected clients.
- Send and receive messages: The client is able to send messages to the server and receive messages from the server.
- Server logging: The server logs all messages received from clients.
- Private messages: The client is able to send private messages to other clients.

## Dependencies

- POSIX Threads (pthread)
- POSIX Sockets (arpa/inet.h)
- Standard C libraries (stdio.h, stdlib.h, string.h, unistd.h, time.h)

## Compilation

To compile the server and client, run the following command:

```bash
make client && make server
```

## Commands

- Public message: Simply type your message and press Enter to send it to all connected clients.
- Private message: Use the command /w <username> <message> to send a private message to a specific user.
- Quit: Use the command /quit to exit the client.

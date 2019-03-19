#include <stdio.h>
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <string.h>
#include <stdlib.h>

void *listener(void *arguments);

struct socket_args {
  int recv_socket, send_socket;
};

int main(int arg_count, char *args[]) {

  // Create server and client sockets
  int server_socket_desc, client_socket_desc, server_conn_socket_desc;
  server_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  client_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_desc == -1 || client_socket_desc == -1) {
    printf("[!]...Failed to create socket(s), exiting...\n");
    return 1;
  }
  printf("[-]...Created sockets\n");

  // Connect to localhost:8001
  // TODO: variable for ip/port
  struct sockaddr_in client;
  client.sin_family = AF_INET;
  client.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // htonl() otherwise it's backwards.
  client.sin_port = htons(8001);
  if(connect(client_socket_desc, (struct sockaddr *)&client, sizeof(client)) < 0) {
    printf("[!]...Could not connect to remote destination, exiting...\n");
    return 1;
  }
  printf("[-]...Connected to localhost:8001\n");

  // Bind server port to localhost:8000
  // TODO: variable for port (and IP?)
  struct sockaddr_in server, server_conn;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // htonl() otherwise it's backwards.
  server.sin_port = htons(8000);
  if (bind(server_socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
    printf("[!]...Failed to bind server socket, exiting...\n");
    return 1;
  }
  printf("[-]...Bound socket to localhost:8000\n");

  // Wait for connection
  listen(server_socket_desc, 3);
  printf("[-]...Awaiting incoming connections...\n");
  int c = sizeof(struct sockaddr_in);
  server_conn_socket_desc = accept(server_socket_desc, (struct sockaddr *)&server_conn, (socklen_t*)&c);
  if (server_conn_socket_desc < 0) {
    printf("[!]...Socket accept failed, exiting...\n");
  }
  printf("[-]...Accepted connection from <TODO>\n");

  // Assign each thread to listen on one socket and send on the other
  struct socket_args server_thread_args, client_thread_args;
  server_thread_args.recv_socket = server_conn_socket_desc;
  server_thread_args.send_socket = client_socket_desc;
  client_thread_args.recv_socket = client_socket_desc;
  client_thread_args.send_socket = server_conn_socket_desc;

  // Make some threads
  pthread_t server_thread, client_thread;
  if (pthread_create(&server_thread, NULL, listener, &server_thread_args) < 0) {
    printf("[!]...Failed to create server thread, exiting...\n");
    return 1;
  }
  if (pthread_create(&client_thread, NULL, listener, &client_thread_args) < 0) {
    printf("[!]...Failed to create client thread, exiting...\n");
    return 1;
  }
  printf("[-]...Handlers assigned\n");

  pthread_join(server_thread, NULL);
  pthread_join(client_thread, NULL);

  return 0;
}

void *listener(void *arguments) {
  struct socket_args *sockets = arguments;
  char socket_message[4096];
  int read_size;

  while((read_size = recv(sockets->recv_socket, socket_message, 4096, 0)) > 0) {
    write(sockets->send_socket, socket_message, strlen(socket_message));
  }

  if (read_size == 0) {
    printf("[-]...Client disconnected\n");
    fflush(stdout);
  }
  else if (read_size == -1) {
    perror("[!]...Recv failed\n");
  }

  free(&(sockets->send_socket));

  return 0;
}
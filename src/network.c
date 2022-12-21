#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "../include/error.h"

void epoll_add_fd(int epoll_fd, int fd) {
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

// ! TCP
void create_tcp_socket(int *fd, struct sockaddr_in *server_address, int epoll_fd) {
  *fd = socket(PF_INET, SOCK_STREAM, 0);
  if (*fd == -1) {
    error_handling("tcp socket create()");
  }

  // TODO: SO_REUSEADDR
  // ! need to solve
  int option = 1;  // true
  socklen_t option_size = sizeof(option);
  setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, option_size);

  if (bind(*fd, (struct sockaddr *)server_address, sizeof(*server_address)) == -1) {
    error_handling("tcp socket bind()");
  }

  if (listen(*fd, 15) == -1) {
    error_handling("tcp socket listen()");
  }

  epoll_add_fd(epoll_fd, *fd);

  printf("TCP is running!\n");
}

// ! UDP
void create_udp_socket(int *fd, struct sockaddr_in *server_address, int epoll_fd) {
  *fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (*fd == -1) {
    error_handling("udp socket create()");
  }

  // TODO: SO_REUSEADDR
  // ! need to solve
  int option = 1;  // true
  socklen_t option_size = sizeof(option);
  setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, option_size);

  if (bind(*fd, (struct sockaddr *)server_address, sizeof(*server_address)) == -1) {
    error_handling("udp socket bind()");
  }

  epoll_add_fd(epoll_fd, *fd);

  printf("UDP is running!\n");
}

void set_server_address(struct sockaddr_in *server_address, unsigned int port) {
  server_address->sin_family = AF_INET;
  server_address->sin_addr.s_addr = inet_addr("127.0.0.1");
  server_address->sin_port = htons(port);
}

void accept_tcp_connection(int fd, int epoll_fd) {
  struct sockaddr_in client_address;
  socklen_t client_address_size = sizeof(client_address);

  int tcp_socket_for_accept = accept(fd, (struct sockaddr *)&client_address, &client_address_size);
  if (tcp_socket_for_accept == -1) {
    error_handling("tcp socket accept()");
  }

  epoll_add_fd(epoll_fd, tcp_socket_for_accept);

  printf("Connected client: %d\n\n", tcp_socket_for_accept);
}
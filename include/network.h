void epoll_add_fd(int epoll_fd, int fd);
void create_tcp_socket(int *fd, struct sockaddr_in *server_address, int epoll_fd);
void create_udp_socket(int *fd, struct sockaddr_in *server_address, int epoll_fd);
void set_server_address(struct sockaddr_in *server_address, unsigned int port);
void accept_tcp_connection(int fd, int epoll_fd);

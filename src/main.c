#define PORT 9999
#define EPOLL_SIZE 50
#define BUFFER_SIZE 256
#define USERNAME_SIZE 256
#define EMAIL_SIZE 256
#define PASSWORD_SIZE 256

#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/error.h"
#include "../include/mysql.h"
#include "../include/network.h"

int main(int argc, char *argv[]) {
  // *
  // *
  // * MySQL variables
  MYSQL *connection = mysql_init(NULL);
  char *aws_rds_ip = "localhost";
  unsigned int aws_rds_port = 3306;
  char *aws_rds_user = "root";
  char *aws_rds_password = "Alchemy@0)3)8)1";
  char *aws_rds_database = "db";
  // char *aws_rds_ip = "database-1.ctdgsyao1atl.us-east-1.rds.amazonaws.com";
  // char *aws_rds_user = "admin";
  // char *aws_rds_password = "12345678";
  // char *aws_rds_database = "db";

  // *
  // *
  // * socket variables
  int tcp_socket_for_listen;
  int tcp_socket_for_accept;
  int udp_socket;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  // socklen_t server_address_size = sizeof(server_address);
  // socklen_t client_address_size = sizeof(server_address);

  // *
  // *
  // * EPOLL variables (socket)
  int epoll_fd;
  struct epoll_event *epoll_events;  // store "read change" fd this time

  // *
  // *
  // * MySQL connect to AWS RDS
  mysql_connect_remote_database(connection, aws_rds_ip, aws_rds_port,
                                aws_rds_user, aws_rds_password,
                                aws_rds_database);

  // *
  // *
  // * set server address
  set_server_address(&server_address, PORT);

  // *
  // *
  // * set epoll
  epoll_fd = epoll_create(EPOLL_SIZE);
  epoll_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

  // *
  // *
  // * create socket including bind and listen(only tcp) and add to epoll
  create_tcp_socket(&tcp_socket_for_listen, &server_address, epoll_fd);
  create_udp_socket(&udp_socket, &server_address, epoll_fd);

  printf("\n");

  while (1) {
    int event_count = epoll_wait(epoll_fd, epoll_events, EPOLL_SIZE,
                                 -1);  // -1 = timeout disable
    if (event_count == -1) {
      puts("epoll_wait() error");
      break;
    }

    for (int i = 0; i < event_count; i++) {
      if (epoll_events[i].data.fd == udp_socket) {
        // *
        // *
        // * read data from udp socket (=> buffer)
        socklen_t client_address_size = sizeof(client_address);
        char buffer[BUFFER_SIZE] = {0};
        recvfrom(udp_socket, buffer, BUFFER_SIZE, 0,
                 (struct sockaddr *)&client_address, &client_address_size);
        printf("%s\n", buffer);

        // *
        // *
        // * register (register_sample)
        if (strncmp(buffer, "register", strlen("register")) == 0) {
          // * parse data
          char username[USERNAME_SIZE] = {0};
          char email[EMAIL_SIZE] = {0};
          char password[PASSWORD_SIZE] = {0};

          sscanf(buffer, "register %s %s %s", username, email, password);

          // TODO: encapsulation
          printf("Checking same name and same email... ");

          memset(buffer, 0, BUFFER_SIZE);
          sprintf(
              buffer,
              "SELECT * FROM users WHERE username = '%s' OR useremail='%s';",
              username, email);

          if (mysql_query(connection, buffer)) {
            fprintf(stderr, "%s\n", mysql_error(connection));
            mysql_close(connection);
            exit(1);
          }

          MYSQL_RES *result;
          result = mysql_store_result(connection);
          int num_rows = mysql_num_rows(result);
          mysql_free_result(result);

          if (num_rows > 0) {
            printf("\nSame name or email! num_rows = %d\n\n", num_rows);
            sendto(udp_socket, "Username or Email is already used\n",
                   strlen("Username or Email is already used\n"), 0,
                   (struct sockaddr *)&client_address, client_address_size);
            continue;
          }
          printf("done\n");

          // * insert user data
          memset(buffer, 0, BUFFER_SIZE);
          sprintf(buffer,
                  "INSERT INTO users (username, useremail, userpassword) "
                  "VALUES ('%s', '%s', '%s');",
                  username, email, password);
          execute_mysql_query(connection, buffer);

          // * register successfully
          sendto(udp_socket, "Register Successfully\n",
                 strlen("Register Successfully\n"), 0,
                 (struct sockaddr *)&client_address, client_address_size);
          printf("\n");
        }

        //
        //
        // TODO: list users (list_rooms_and_users_sample)
        if (strncmp(buffer, "list users", strlen("list users")) == 0) {
          execute_mysql_query_and_print(
              connection, "SELECT * FROM users ORDER BY username;");
          continue;
        }

        //
        //
        // TODO: list rooms (list_rooms_and_users_sample)
        if (strncmp(buffer, "list rooms", strlen("list rooms")) == 0) {
          execute_mysql_query_and_print(connection,
                                             "SELECT * FROM rooms;");
          continue;
        }

      } else if (epoll_events[i].data.fd == tcp_socket_for_listen) {
        // *
        // *
        // * accept new user
        accept_tcp_connection(tcp_socket_for_listen, epoll_fd);

      } else {
        // TODO: TCP data transfer
        char buffer[BUFFER_SIZE] = {0};
        int data_len = read(epoll_events[i].data.fd, buffer,
                            BUFFER_SIZE);  // read data from tcp socket

        if (data_len == 0) {
          // TODO: close request
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epoll_events[i].data.fd,
                    NULL);                 // delete from epoll
          close(epoll_events[i].data.fd);  // socket close
          printf("closed client: %d\n", epoll_events[i].data.fd);
        } else {
          //
          //
          // TODO: login <username> <password> (login_sample)
          if (strncmp(buffer, "login", strlen("login")) == 0) {
            // TODO: check username exist
            // TODO: check already login at this socket
            // TODO: check someone already login as this username
            // TODO: check password is correct
          }

          //
          //
          // TODO: logout (logout_sample)
          if (strncmp(buffer, "logout", strlen("logout")) == 0) {
            ;
          }
        }
      }
    }
    // TODO
  }

  // // MySQL query
  // execute_mysql_query_and_print(connection, "show tables");

  // * socket close
  close(tcp_socket_for_listen);
  close(udp_socket);
  close(epoll_fd);

  // * MySQL close
  mysql_close(connection);

  return 0;
}
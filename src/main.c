#define PORT 9999
#define EPOLL_SIZE 50
#define BUFFER_SIZE 1024
#define USERNAME_SIZE 256
#define EMAIL_SIZE 256
#define PASSWORD_SIZE 256
#define QUERY_SIZE 256
#define ROW_SIZE 64

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




int is_string_match(const char *buffer, const char *keyword) {
  return (strncmp(buffer, keyword, strlen(keyword)) == 0);
}



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

        // register (register_sample)
        if (strncmp(buffer, "register", strlen("register")) == 0) {
          // parse data
          char username[USERNAME_SIZE] = {0};
          char email[EMAIL_SIZE] = {0};
          char passwd[PASSWORD_SIZE] = {0};

          sscanf(buffer, "register %s %s %s", username, email, passwd);

          // check unique username and email
          printf("Checking unique username and same email... ");
          if (!username_and_email_are_unique(username, email)) {
            printf("\nSame name or email! num_rows = %d\n\n", num_rows);
            sendto(udp_socket, "Username or Email is already used\n",
                   strlen("Username or Email is already used\n"), 0,
                   (struct sockaddr *)&client_address, client_address_size);
            continue;
          }
          printf("done\n");

          // register a new user and return successful message
          register_user(username, email, passwd);
          sendto(udp_socket, "Register Successfully\n",
                 strlen("Register Successfully\n"), 0,
                 (struct sockaddr *)&client_address, client_address_size);
          printf("\n");
        }

        // list users (list_rooms_and_users_sample)
        if (strncmp(buffer, "list users", strlen("list users")) == 0) {

          memset(buffer, 0, BUFFER_SIZE);
          strcat(buffer, "List Users\n");

          // no users
          if (get_mysql_query_result_row_count(connection, "SELECT * FROM users;") == 0) {
            strcat(buffer, "No Users\n");
            sendto(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, client_address_size);
            printf("\n");
            continue;
          }

          // query
          execute_mysql_query(connection, "SELECT username, email, onlinefd FROM users ORDER BY username;");

          MYSQL_RES *result = mysql_use_result(connection);
          MYSQL_ROW row;

          // fetch result
          int count = 0;
          while ((row = mysql_fetch_row(result)) != NULL) {
            count++;
            char temp[ROW_SIZE];
            // ! 確認一下 onlinefd 沒有值的時候 row[2] 會是甚麼?
            sprintf(temp, "%d. %s<%s> %s\n", count, row[0], row[1], row[2] != 0 ? "Online" : "Offline");
            strcat(buffer, temp);
          }
          mysql_free_result(result);

          // return message
          sendto(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, client_address_size);
          printf("\n");

        }

        // list rooms (list_rooms_and_users_sample)
        if (strncmp(buffer, "list rooms", strlen("list rooms")) == 0) {

          memset(buffer, 0, BUFFER_SIZE);
          strcat(buffer, "List Game Rooms\n");

          // no users
          if (get_mysql_query_result_row_count(connection, "SELECT * FROM users;") == 0) {
            strcat(buffer, "No Rooms\n");
            sendto(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, client_address_size);
            printf("\n");
            continue;
          }

          // query
          execute_mysql_query(connection, "SELECT class, id, round FROM rooms ORDER BY id;");

          MYSQL_RES *result = mysql_use_result(connection);
          MYSQL_ROW row;

          // fetch result
          int count = 0;
          while ((row = mysql_fetch_row(result)) != NULL) {
            count++;
            char temp[ROW_SIZE];
            // ! careful about the strcmp
            sprintf(temp, "%d. (%s) Game Room %s %s\n", count, (strcmp(row[0], "1") == 0) ? "Public" : "Private", row[1], (strcmp(row[2], "0") == 0) ? "is open for players" : "has started playing");
            strcat(buffer, temp);
          }
          mysql_free_result(result);

          // return message
          sendto(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, client_address_size);
          printf("\n");


        }

      } else if (epoll_events[i].data.fd == tcp_socket_for_listen) {
        // accept new user
        accept_tcp_connection(tcp_socket_for_listen, epoll_fd);

      } else {
        char buffer[BUFFER_SIZE] = {0};
        int data_len = read(epoll_events[i].data.fd, buffer,
                            BUFFER_SIZE);  // read data from tcp socket

        if (data_len == 0) {
          // close request
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epoll_events[i].data.fd,
                    NULL);                 // delete from epoll
          close(epoll_events[i].data.fd);  // socket close
          printf("closed client: %d\n", epoll_events[i].data.fd);


        } else {
          // login <username> <password> (login_sample)
          if (strncmp(buffer, "login", strlen("login")) == 0) {
            // parser username and password
            char username[USERNAME_SIZE] = { 0 };
            char passwd[PASSWORD_SIZE] = { 0 };
            sscanf(buffer, "login %s %s", username, passwd);

            char query[QUERY_SIZE] = { 0 };

            // 從 database 找出 match 的 username
            // 如果找不到 Fail(1) Username does not exist
            // 如果找到但是有 online fd Fail(3) Someone already logged in as <username>
            // 如果找到且沒有 online fd 但是 password 不 match Fail(4) Wrong password
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT username, onlinefd, passwd FROM users WHERE username='%s'", username);
            execute_mysql_query(connection, query);

            MYSQL_RES *result = mysql_use_result(connection);
            MYSQL_ROW row = mysql_fetch_row(result)
            if (row == NULL) { // Fail(1)
              write(epoll[i].data.fd, "Username does not exist", strlen("Username does not exist"));
              continue;
            }
            if (row[1] != none???) { // Fail(3) // TODO
              write();
              continue;
            }
            if (strcmp(row[2], passwd) != 0) { // Fail(4)
              write(epoll[i].data.fd, "Wrong password", strlen("Wrong password"));
              continue;
            }
            mysql_free_result(result);




            // 從 database 找出 match 的 onlinefd
            // 如果找到 Fail(2) You already logged in as <username>
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT username FROM users WHERE onlinefd=%d", epoll_events[i].data.fd);
            execute_mysql_query(connection, query);
            
            MYSQL_RES *result = mysql_use_result(connection);
            MYSQL_ROW row = mysql_fetch_row(result)
            if (row != NULL) { // Fail(2) // TODO
              write();
              continue;
            }
            mysql_free_result(result);



            // login successfully
            // TODO: 更改 database 的 onlinefd 數值

            // return successful message
            char success_message[ROW_SIZE] = { 0 };
            sprintf(success_message, "Welcome, %s", username);
            write(epoll_events[i].data.fd, success_message, ROW_SIZE);

          }





          // logout (logout_sample)
          if (strncmp(buffer, "logout", strlen("logout")) == 0) {

            // 在 database 的 users table 找出和當前 fd match 的 roomid
            // check login Fail(1)
            // check in room Fail(2)
            char query[QUERY_SIZE] = { 0 };
            sprintf(query, "SELECT username, roomid FROM users WHERE onlinefd=%d", epoll_events[i].data.fd);
            execute_mysql_query(connection, query);
            MYSQL_RES *result = mysql_use_result(connection);
            MYSQL_ROW row = mysql_fetch_row(result)
            if (row == NULL) {
              write(epoll_events[i].data.fd, "You are not logged in", strlen(You are not logged in));
              continue;
            }
            if (row[1] != ?????) {
              char temp[ROW_SIZE] = { 0 };
              sprintf(temp, "You are already in game room %s, please leave game room", row[1]);
              write(epoll_events[i].data.fd, temp, ROW_SIZE);
              continue;
            }

            // logout successfully
            // TODO: 修改 database 的 users table 裡面和當前 fd match 的 onlinefd 為 none

            // return successful message
            // Goodbye, <username>
            char success_message[ROW_SIZE] = { 0 };
            sprintf(success_message, "Goodbye, %s", row[0]);
            write(epoll_events[i].data.fd, success_message, ROW_SIZE);

            mysql_free_result(result); // 用完 row[0] 才 free !

          }






          // create public room
          if (strncmp(buffer, "create public room", strlen("create public room")) == 0) {
            
            
            // parser game room id
            unsigned int roomid = 0;
            sscanf(buffer, "create public room %u", roomid);

            char query[QUERY_SIZE] = { 0 };
            // 從 database 的 users table 找和當前 fd match 的 online fd 的 roomid
            // check login Fail(1)
            // check already in room Fail(2)
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT roomid, id FROM users WHERE onlinefd=%d", epoll_events[i].data.fd);
            execute_mysql_query(connection, query);

            MYSQL_RES *result = mysql_use_result(connection);
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row == NULL) { // Fail(1)
              write(epoll_events[i].data.fd, "You are not logged in", strlen("You are not logged in"));
              continue;
            }
            if (row[0] != ???) { // Fail(2)
              char fail_message[ROW_SIZE] = { 0 };
              sprintf(fail_message, "You are already in game room %s, please leave game room", row[0]);
              write(epoll_events[i].data.fd, fail_message, ROW_SIZE);
              continue;
            }
            mysql_free_result(result);


            // 看有沒有重複 roomid
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT * FROM rooms WHERE id=%u", roomid);
            if (get_mysql_query_result_row_count(connection, query) != 0) {
              write(epoll_events[i].data.fd, "Game room ID is used, choose another one", strlen("Game room ID is used, choose another one"));
              continue;
            }



            // create public room successfully
            // TODO: 在 rooms table 新增 id 為 roomid 的房間並把 host 設為 row[1] 以及 class 設為 1 (public)
            // TODO: 在 users table 和當前 fd match 的玩家的 roomid 設為 roomid

            // return successful message
            char success_message[ROW_SIZE] = { 0 };
            sprintf(success_message, "You create public game room %u", roomid);
            write(epoll_events[i].data.fd, success_message, ROW_SIZE);


          }







          if (strncmp(buffer, "create private room", strlen("create private room")) == 0) {
            
            
            // parser room_id and room_code
            unsigned int room_id, room_code;
            sscanf(buffer, "create private room %u %u", room_id, room_code);


            char query[QUERY_SIZE] = { 0 };
            // 從 database 的 users table 找出和當前 fd match 的 roomid
            // not login Fail(1)
            // in room Fail(2)
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT roomid FROM users WHERE onlinefd=%d", epoll_events[i].data.fd);
            execute_mysql_query(query);

            MYSQL_RES *result = mysql_use_result(connection);
            MYSQL_ROW row = mysql_fetch_row(result);

            if (row == NULL) { // Fail(1)
              write(epoll_events[i].data.fd, "You are not logged in", strlen("You are not logged in"));
              continue;
            }
            if (row[0] != ?????) { // Fail(2)
              char fail_message[ROW_SIZE] = { 0 };
              sprintf(fail_message, "You are already in game room %s, please leave game room", row[0]);
              write(epoll_events[i].data.fd, fail_message, ROW_SIZE);
              continue;
            }

            // 看有沒有重複 roomid
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT * FROM rooms WHERE id=%u", roomid);
            if (get_mysql_query_result_row_count(connection, query) != 0) {
              write(epoll_events[i].data.fd, "Game room ID is used, choose another one", strlen("Game room ID is used, choose another one"));
              continue;
            }



            // create public room successfully
            // TODO: 在 rooms table 新增 id 為 room_id 的房間並把 host 設為 row[1] 以及 class 設為 0 (private) code 設為 room_code
            // TODO: 在 users table 和當前 fd match 的玩家的 roomid 設為 room_id

            // return successful message
            char success_message[ROW_SIZE] = { 0 };
            sprintf(success_message, "You create private game room %u", room_id);
            write(epoll_events[i].data.fd, success_message, ROW_SIZE);




          }



          if (strncmp(buffer, "join room", strlen("join room")) == 0) {
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
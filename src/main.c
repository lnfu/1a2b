#define TEST
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
int is_string_match(const char *buffer, const char *keyword) { return (strncmp(buffer, keyword, strlen(keyword)) == 0); }
int main(int argc, char *argv[]) {
  // MySQL variables
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





  // socket variables
  int tcp_socket_for_listen;
  int tcp_socket_for_accept;
  int udp_socket;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  // socklen_t server_address_size = sizeof(server_address);
  // socklen_t client_address_size = sizeof(server_address);





  // EPOLL variables (socket)
  int epoll_fd;
  struct epoll_event *epoll_events;  // store "read change" fd this time





  // MySQL connect to AWS RDS
  mysql_connect_remote_database(connection, aws_rds_ip, aws_rds_port, aws_rds_user, aws_rds_password, aws_rds_database);
#ifdef TEST
  printf("clear data... ");
  char query[QUERY_SIZE] = {0};

  memset(query, 0, QUERY_SIZE);
  sprintf(query, "TRUNCATE TABLE users;");
  execute_mysql_query(connection, query);

  memset(query, 0, QUERY_SIZE);
  sprintf(query, "TRUNCATE TABLE rooms;");
  execute_mysql_query(connection, query);
  printf("done\n");
#endif





  // set server address
  set_server_address(&server_address, PORT);





  // set epoll

  epoll_fd = epoll_create(EPOLL_SIZE);
  epoll_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);





  // create socket including bind and listen(only tcp) and add to epoll
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
      int current_fd = epoll_events[i].data.fd;
      if (current_fd == udp_socket) {
        // read data from udp socket (=> buffer)
        socklen_t client_address_size = sizeof(client_address);
        char buffer[BUFFER_SIZE] = {0};
        recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, &client_address_size);
        printf("\033[0;32m%s\n\033[0m", buffer);





        // register (register_sample)
        if (is_string_match(buffer, "register")) {
          // parse data
          char username[USERNAME_SIZE] = {0};
          char email[EMAIL_SIZE] = {0};
          char passwd[PASSWORD_SIZE] = {0};
          sscanf(buffer, "register %s %s %s", username, email, passwd);


          // check unique username and email
          printf("Checking unique username and same email... ");
          if (!username_and_email_are_unique(connection, username, email)) {
            printf("\nSame name or email!\n\n");
            sendto(udp_socket, "Username or Email is already used\n", strlen("Username or Email is already used\n"), 0,
                   (struct sockaddr *)&client_address, client_address_size);
            continue;
          }
          printf("done\n");


          // register a new user and return successful message
          register_user(connection, username, email, passwd);
          sendto(udp_socket, "Register Successfully\n", strlen("Register Successfully\n"), 0, (struct sockaddr *)&client_address,
                 client_address_size);

          printf("\n");

          // char query[QUERY_SIZE] = {0};
          // sprintf(query, "SELECT * FROM users;");
          // execute_mysql_query_and_print(connection, query, 6);
        }





        // list users (list_rooms_and_users_sample)
        if (is_string_match(buffer, "list users")) {
          memset(buffer, 0, BUFFER_SIZE);
          strcat(buffer, "List Users\n");





          // no users
          if (get_mysql_query_result_row_count(connection, "SELECT * FROM users;") == 0) {
            strcat(buffer, "No Users\n");
            sendto(udp_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&client_address, client_address_size);

            printf("\n");
            continue;
          }




          MYSQL_RES *result;
          MYSQL_ROW row;
          // query
          execute_mysql_query(connection, "SELECT username, email, online_fd FROM users ORDER BY username;");
          result = mysql_use_result(connection);
          // fetch result
          int count = 0;
          while ((row = mysql_fetch_row(result)) != NULL) {
            count++;
            char temp[ROW_SIZE];
            sprintf(temp, "%d. %s<%s> %s\n", count, row[0], row[1], row[2] != NULL ? "Online" : "Offline");
            strcat(buffer, temp);
          }
          mysql_free_result(result);





          // return message
          sendto(udp_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&client_address, client_address_size);
          printf("\n");
        }





        // list rooms (list_rooms_and_users_sample)
        if (is_string_match(buffer, "list rooms")) {
          memset(buffer, 0, BUFFER_SIZE);
          strcat(buffer, "List Game Rooms\n");





          // no users
          if (get_mysql_query_result_row_count(connection, "SELECT * FROM users;") == 0) {
            strcat(buffer, "No Rooms\n");
            sendto(udp_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&client_address, client_address_size);

            printf("\n");
            continue;
          }




          MYSQL_RES *result;
          MYSQL_ROW row;
          // query
          execute_mysql_query(connection, "SELECT class, id, round FROM rooms ORDER BY id;");
          result = mysql_use_result(connection);
          // fetch result
          int count = 0;
          while ((row = mysql_fetch_row(result)) != NULL) {
            count++;
            char temp[ROW_SIZE];
            sprintf(temp, "%d. (%s) Game Room %s %s\n", count, (strcmp(row[0], "1") == 0) ? "Public" : "Private", row[1],
                    (strcmp(row[2], "0") == 0) ? "is open for players" : "has started playing");
            strcat(buffer, temp);
          }
          mysql_free_result(result);





          // return message
          sendto(udp_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&client_address, client_address_size);
          printf("\n");
        }





      } else if (current_fd == tcp_socket_for_listen) {
        // accept new user
        accept_tcp_connection(tcp_socket_for_listen, epoll_fd);
      } else {
        char buffer[BUFFER_SIZE] = {0};
        int data_len = read(current_fd, buffer, BUFFER_SIZE);  // read data from tcp socket
        printf("\033[0;32m%s\033[0m", buffer);


        if (data_len == 0) {
          // close request
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);  // delete from epoll
          close(current_fd);                                     // socket close
          printf("closed client: %d\n\n", current_fd);





        } else {
          // login <username> <password> (login_sample)
          if (is_string_match(buffer, "login")) {
            MYSQL_RES *result;
            MYSQL_ROW row;
            char query[QUERY_SIZE] = {0};
            char input_username[USERNAME_SIZE] = {0};
            char input_passwd[PASSWORD_SIZE] = {0};
            char current_login_username[USERNAME_SIZE] = {0};





            // parser username and passwd
            sscanf(buffer, "login %s %s", input_username, input_passwd);


            // Fail(1) Username does not exist
            if (!check_username_exist(connection, input_username)) {
              write(current_fd, "Username does not exist\n", strlen("Username does not exist\n"));

              printf("Fail(1)\n\n");
              continue;
            }


            // Fail(2) You already logged in as <username>
            if (get_user_id_by_current_fd(connection, current_fd, current_login_username) != 0) {
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "You already logged in as %s\n", current_login_username);
              write(current_fd, fail_message, strlen(fail_message));

              printf("Fail(2)\n\n");
              continue;
            };


            // Fail(3) Someone already logged in as<username>
            if (!check_user_is_not_logged_in(connection, input_username)) {
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "Someone already logged in as %s\n", input_username);
              write(current_fd, fail_message, strlen(fail_message));

              printf("Fail(3)\n\n");
              continue;
            }


            // Fail(4) Wrong password
            if (!check_passwd_is_correct(connection, input_username, input_passwd)) {
              write(current_fd, "Wrong password\n", strlen("Wrong password\n"));

              printf("Fail(4)\n\n");
              continue;
            }


            // Success Welcome, <username>
            // update database users table
            printf("Login as %s... ", input_username);
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET online_fd=%d WHERE username='%s';", current_fd, input_username);
            execute_mysql_query(connection, query);
            printf("done\n");

            // return successful message
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "Welcome, %s\n", input_username);
            write(current_fd, success_message, strlen(success_message));

            printf("\n");
          }





          // // logout (logout_sample)
          // if (strncmp(buffer, "logout", strlen("logout")) == 0) {
          //   // 在 database 的 users table 找出和當前 fd match 的 roomid
          //   // check login Fail(1)
          //   // check in room Fail(2)
          //   char query[QUERY_SIZE] = {0};
          //   sprintf(query,
          //           "SELECT username, roomid FROM users WHERE onlinefd=%d",
          //           current_fd);
          //   execute_mysql_query(connection, query);
          //   MYSQL_RES *result = mysql_use_result(connection);
          //   MYSQL_ROW row = mysql_fetch_row(result) if (row == NULL) {
          //     write(current_fd, "You are not logged in",
          //           strlen(You are not logged in));
          //     continue;
          //   }
          //   if (row[1] != ?????) {
          //     char temp[ROW_SIZE] = {0};
          //     sprintf(temp,
          //             "You are already in game room %s, please leave game
          //             room", row[1]);
          //     write(current_fd, temp, ROW_SIZE);
          //     continue;
          //   }
          //   // logout successfully
          //   // TODO: 修改 database 的 users table 裡面和當前 fd match 的
          //   // onlinefd 為 none
          //   // return successful message
          //   // Goodbye, <username>
          //   char success_message[ROW_SIZE] = {0};
          //   sprintf(success_message, "Goodbye, %s", row[0]);
          //   write(current_fd, success_message, ROW_SIZE);
          //   mysql_free_result(result);  // 用完 row[0] 才 free !
          // }





          // create public room
          if (is_string_match(buffer, "create public room")) {
            unsigned int input_room_id = 0;
            unsigned int current_room_id = 0;
            int user_id = 0;
            char current_login_username[USERNAME_SIZE] = {0};





            // parser game room id
            sscanf(buffer, "create public room %u", &input_room_id);


            // Fail(1) You are not logged in
            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id == 0) {
              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              printf("Fail(1)\n\n");
              continue;
            }


            // Fail(2) You are already in game room <game room id>, please leave game room
            if (!check_current_fd_is_not_in_room(connection, current_fd, &current_room_id)) {
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "You are already in game room %u, please leave game room\n", current_room_id);
              write(current_fd, fail_message, strlen(fail_message));

              printf("Fail(2)\n\n");
              continue;
            }


            // Fail(3) Game room ID is used, choose another one
            if (!room_id_is_unique(connection, input_room_id)) {
              write(current_fd, "Game room ID is used, choose another one\n", strlen("Game room ID is used, choose another one\n"));

              printf("Fail(3)\n\n");
              continue;
            }


            // Success You create public game room <game room id>
            // 在 rooms table 新增 id 為 room_id 的房間並把 host 設為 row[1] 以及 class 設為 1 (public)
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "INSERT INTO rooms (id, class, host, round) VALUES (%u, 1, %d, 0);", input_room_id, user_id);
            execute_mysql_query(connection, query);

            // 在 users table 和當前 fd match 的玩家的 roomid 設為 roomid
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=%d WHERE online_fd=%d;", input_room_id, current_fd);
            execute_mysql_query(connection, query);

            // return successful message
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "You create public game room %u\n", input_room_id);
            write(current_fd, success_message, strlen(success_message));

            printf("\n");
          }




          // create private room
          if (is_string_match(buffer, "create private room")) {
            unsigned int input_room_id = 0;
            unsigned int input_room_code = 0;
            unsigned int current_room_id = 0;
            int user_id = 0;
            char current_login_username[USERNAME_SIZE] = {0};





            // parser room_id and room_code
            sscanf(buffer, "create private room %u %u", &input_room_id, &input_room_code);


            // Fail(1) You are not logged in
            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id == 0) {
              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              printf("Fail(1)\n\n");
              continue;
            }


            // Fail(2) You are already in game room <game room id>, please leave game room
            if (!check_current_fd_is_not_in_room(connection, current_fd, &current_room_id)) {
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "You are already in game room %u, please leave game room\n", current_room_id);
              write(current_fd, fail_message, strlen(fail_message));

              printf("Fail(2)\n\n");
              continue;
            }


            // Fail(3) Game room ID is used, choose another one
            if (!room_id_is_unique(connection, input_room_id)) {
              write(current_fd, "Game room ID is used, choose another one\n", strlen("Game room ID is used, choose another one\n"));

              printf("Fail(3)\n\n");
              continue;
            }


            // create public room successfully
            // 在 rooms table 新增 id 為 room_id 的房間並把 host 設為 row[1] 以及 class 設為 1 (public)
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "INSERT INTO rooms (id, class, host, round, code) VALUES (%u, 0, %d, 0, %u);", input_room_id, user_id,
                    input_room_code);
            execute_mysql_query(connection, query);

            // 在 users table 和當前 fd match 的玩家的 roomid 設為 roomid
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=%d WHERE online_fd=%d;", input_room_id, current_fd);
            execute_mysql_query(connection, query);

            // return successful message
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "You create private game room %u\n", input_room_id);
            write(current_fd, success_message, strlen(success_message));

            printf("\n");
          }



          // join room
          if (is_string_match(buffer, "join room")) {
            // parser room id
            unsigned int room_id;
            sscanf(buffer, "join room %u", &room_id);





            int user_id = get_user_id_if_allowed_to_enter_room(connection, current_fd);
            if (user_id == 0) {
              continue;
            }





            MYSQL_RES *result;
            MYSQL_ROW row;
            char query[QUERY_SIZE] = {0};
            char username[USERNAME_SIZE] = {0};  // current username





            if (room_id_is_unique(connection, room_id)) {
              printf("\n");
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "Game room %u is not exist\n", room_id);
              write(current_fd, fail_message, strlen(fail_message));

              continue;
            }


            if (get_room_class(connection, room_id) == 0) {
              write(current_fd, "Game room is private, please join game by invitation code\n",
                    strlen("Game room is private, please join game by invitation code\n"));
              continue;
            }

            if (is_game_in_room_in_progress(connection, room_id)) {
              write(current_fd, "Game has started, you can't join now\n", strlen("Game has started, you can't join now\n"));
              continue;
            }





            // success
            // You join game room <game room id>
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "You join game room %u\n", room_id);
            write(current_fd, success_message, strlen(success_message));


            // send to others in the room

            // get current username
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT username FROM users WHERE online_fd=%d", current_fd);
            execute_mysql_query(connection, query);

            result = mysql_use_result(connection);
            row = mysql_fetch_row(result);
            sprintf(username, "%s", row[0]);
            mysql_free_result(result);


            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT online_fd FROM users WHERE room_id=%u", room_id);
            execute_mysql_query(connection, query);

            result = mysql_use_result(connection);
            while ((row = mysql_fetch_row(result)) != NULL) {
              int other_user_in_room;
              sscanf(row[0], "%d", &other_user_in_room);
              char temp[ROW_SIZE] = {0};
              sprintf(temp, "Welcome, %s to game!\n", username);
              write(other_user_in_room, temp, strlen(temp));
            }
            mysql_free_result(result);





            // 更改 users table 中現在 user 的 room_id
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=%d WHERE online_fd=%d;", room_id, current_fd);
            execute_mysql_query(connection, query);


            printf("\n\n");
          }




          // // leave room
          // if (is_string_match(buffer, "leave room")) {
          //   if (!is_user_logged_in_and_in_room(connection, current_fd)) {
          //     continue;
          //   }

          //   unsigned int room_id = is_user_host(connection, current_fd);
          //   if (room_id != 0) {
          //     printf("You leave game room %u\n", room_id);
          //     char success_message[ROW_SIZE] = {0};
          //     sprintf(success_message, "You leave game room %u", room_id);
          //     write(current_fd, success_message, strlen(success_message));
          //   }
          // }





          //

          // if (strncmp(buffer, "invite", strlen("invite")) == 0) {
          //   ;
          // }
          // if (strncmp(buffer, "list invitations", strlen("list invitations"))
          // ==
          //     0) {
          //   ;
          // }
          // if (strncmp(buffer, "accept", strlen("accept")) == 0) {
          //   ;
          // }
          // if (strncmp(buffer, "leave room", strlen("leave room")) == 0) {
          //   ;
          // }
          // if (strncmp(buffer, "start game", strlen("start game")) == 0) {
          //   ;
          // }
          // if (strncmp(buffer, "guess", strlen("guess")) == 0) {
          //   ;
          // }
          // if (strncmp(buffer, "exit", strlen("exit")) == 0) {
          //   ;
          // }
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
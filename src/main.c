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





        // // list users (list_rooms_and_users_sample)
        // if (strncmp(buffer, "list users", strlen("list users")) == 0) {
        //   memset(buffer, 0, BUFFER_SIZE);
        //   strcat(buffer, "List Users\n");
        //   // no users
        //   if (get_mysql_query_result_row_count(connection, "SELECT * FROM
        //   users;") == 0) {
        //     strcat(buffer, "No Users\n");
        //     sendto(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr
        //     *)&client_address, client_address_size); printf("\n"); continue;
        //   }
        //   // query
        //   execute_mysql_query(connection, "SELECT username, email, onlinefd
        //   FROM users ORDER BY username;");
        //   MYSQL_RES *result = mysql_use_result(connection);
        //   MYSQL_ROW row;
        //   // fetch result
        //   int count = 0;
        //   while ((row = mysql_fetch_row(result)) != NULL) {
        //     count++;
        //     char temp[ROW_SIZE];
        //     // ! 確認一下 onlinefd 沒有值的時候 row[2] 會是甚麼?
        //     sprintf(temp, "%d. %s<%s> %s\n", count, row[0], row[1], row[2] !=
        //     0 ? "Online" : "Offline"); strcat(buffer, temp);
        //   }
        //   mysql_free_result(result);
        //   // return message
        //   sendto(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr
        //   *)&client_address, client_address_size); printf("\n");
        // }
        // // list rooms (list_rooms_and_users_sample)
        // if (strncmp(buffer, "list rooms", strlen("list rooms")) == 0) {
        //   memset(buffer, 0, BUFFER_SIZE);
        //   strcat(buffer, "List Game Rooms\n");
        //   // no users
        //   if (get_mysql_query_result_row_count(connection, "SELECT * FROM
        //   users;") == 0) {
        //     strcat(buffer, "No Rooms\n");
        //     sendto(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr
        //     *)&client_address, client_address_size); printf("\n"); continue;
        //   }
        //   // query
        //   execute_mysql_query(connection, "SELECT class, id, round FROM rooms
        //   ORDER BY id;");
        //   MYSQL_RES *result = mysql_use_result(connection);
        //   MYSQL_ROW row;
        //   // fetch result
        //   int count = 0;
        //   while ((row = mysql_fetch_row(result)) != NULL) {
        //     count++;
        //     char temp[ROW_SIZE];
        //     // ! careful about the strcmp
        //     sprintf(temp, "%d. (%s) Game Room %s %s\n", count,
        //     (strcmp(row[0], "1") == 0) ? "Public" : "Private", row[1],
        //     (strcmp(row[2], "0") == 0) ? "is open for players" : "has started
        //     playing"); strcat(buffer, temp);
        //   }
        //   mysql_free_result(result);
        //   // return message
        //   sendto(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr
        //   *)&client_address, client_address_size); printf("\n");
        // }
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
            // parser username and passwd
            char username[USERNAME_SIZE] = {0};
            char passwd[PASSWORD_SIZE] = {0};
            sscanf(buffer, "login %s %s", username, passwd);

            MYSQL_RES *result;
            MYSQL_ROW row;
            char query[QUERY_SIZE] = {0};
            // TODO: check username exist --> encapsulation
            // 從 database 找出 match 的 username
            // 如果找不到 Fail(1) Username does not exist
            // 如果找到但是有 online fd Fail(3) Someone already logged in as
            // <username> 如果找到且沒有 online fd 但是 password 不 match
            // Fail(4) Wrong password
            printf("Checking username exist, not logged in yet, and correct password... ");

            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT username, online_fd, passwd FROM users WHERE username='%s';", username);

            execute_mysql_query(connection, query);

            result = mysql_use_result(connection);
            row = mysql_fetch_row(result);
            if (row == NULL) {  // Fail(1) Username does not exist
              printf("\nUsername does not exist\n\n");

              write(current_fd, "Username does not exist\n", strlen("Username does not exist\n"));

              mysql_free_result(result);
              continue;
            }


            if (row[1] != NULL) {  // Fail(3) Someone already logged in as (if not null)
              printf("\nSomeone already logged in as %s\n\n", username);

              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "Someone already logged in as %s\n", username);
              write(current_fd, fail_message, strlen(fail_message));

              mysql_free_result(result);
              continue;
            }


            if (strcmp(row[2], passwd) != 0) {  // Fail(4)
              printf("\nWrong password\n\n");

              write(current_fd, "Wrong password\n", strlen("Wrong password\n"));

              mysql_free_result(result);
              continue;
            }
            mysql_free_result(result);
            printf("done\n");




            // 從 database 找出 match 的 onlinefd
            // 如果找到 Fail(2) You already logged in as <username>
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT username FROM users WHERE online_fd=%d", current_fd);
            execute_mysql_query(connection, query);

            result = mysql_use_result(connection);
            row = mysql_fetch_row(result);
            if (row != NULL) {  // Fail(2) You already logged in as <username>
              printf("You already logged in as %s\n\n", username);

              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "You already logged in as %s\n", row[0]);
              write(current_fd, fail_message, strlen(fail_message));

              mysql_free_result(result);
              continue;
            }
            mysql_free_result(result);





            // login successfully
            // update database users table

            printf("Login as %s... ", username);
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET online_fd=%d WHERE username='%s';", current_fd, username);
            execute_mysql_query(connection, query);
            printf("done\n");

            // return successful message
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "Welcome, %s\n", username);
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
            // parser game room id
            unsigned int room_id = 0;
            sscanf(buffer, "create public room %u", &room_id);



            MYSQL_RES *result;
            MYSQL_ROW row;
            char query[QUERY_SIZE] = {0};
            // 從 database 的 users table 找和當前 fd match 的 online fd 的
            // roomid check login Fail(1) check already in room Fail(2)
            // TODO: not login and already in room check encapsulation
            printf("Check logged in and not in room... ");

            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT room_id, id FROM users WHERE online_fd=%d", current_fd);
            execute_mysql_query(connection, query);

            result = mysql_use_result(connection);
            row = mysql_fetch_row(result);
            if (row == NULL) {  // Fail(1)
              printf("\nYou are not logged in\n\n");

              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              mysql_free_result(result);
              continue;
            }

            if (row[0] != NULL) {  // Fail(2)
              printf("\nYou are already in game room %s, please leave game room\n\n", row[0]);

              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "You are already in game room %s, please leave game room\n", row[0]);
              write(current_fd, fail_message, strlen(fail_message));

              mysql_free_result(result);
              continue;
            }
            int user_id;
            sscanf(row[1], "%d", &user_id);
            mysql_free_result(result);
            printf("done\n");





            printf("Check unique room_id... ");
            // 看有沒有重複 room_id
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT * FROM rooms WHERE id=%u", room_id);
            if (get_mysql_query_result_row_count(connection, query) != 0) {
              printf("Game room ID is used, choose another one\n\n");

              write(current_fd, "Game room ID is used, choose another one\n", strlen("Game room ID is used, choose another one\n"));

              continue;
            }
            printf("done\n");





            // create public room successfully
            // 在 rooms table 新增 id 為 room_id 的房間並把 host 設為 row[1] 以及 class 設為 1 (public)
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "INSERT INTO rooms (id, class, host, round) VALUES (%u, 1, %d, 0);", room_id, user_id);
            execute_mysql_query(connection, query);

            // 在 users table 和當前 fd match 的玩家的 roomid 設為 roomid
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=%d WHERE online_fd=%d;", room_id, current_fd);
            execute_mysql_query(connection, query);


            // return successful message
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "You create public game room %u\n", room_id);
            write(current_fd, success_message, strlen(success_message));

            printf("\n");
          }





          if (is_string_match(buffer, "create private room")) {
            // parser room_id and room_code
            unsigned int room_id, room_code;
            sscanf(buffer, "create private room %u %u", &room_id, &room_code);



            MYSQL_RES *result;
            MYSQL_ROW row;
            char query[QUERY_SIZE] = {0};
            // 從 database 的 users table 找出和當前 fd match 的 roomid
            // not login Fail(1)
            // in room Fail(2)
            // TODO: using check logged in and in room encapsulation
            printf("Check logged in and in room... ");

            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT room_id, id FROM users WHERE online_fd=%d", current_fd);
            execute_mysql_query(connection, query);

            result = mysql_use_result(connection);
            row = mysql_fetch_row(result);
            if (row == NULL) {  // Fail(1)
              printf("\nYou are not logged in\n\n");

              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              mysql_free_result(result);
              continue;
            }

            if (row[0] != NULL) {  // Fail(2)
              printf("\nYou are already in game room %s, please leave game room\n\n", row[0]);

              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "You are already in game room %s, please leave game room\n", row[0]);
              write(current_fd, fail_message, strlen(fail_message));

              mysql_free_result(result);
              continue;
            }
            int user_id;
            sscanf(row[1], "%d", &user_id);
            mysql_free_result(result);
            printf("done\n");





            // 看有沒有重複 room_id
            // TODO: encapsulation (public also)
            printf("Check unique room_id... ");
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT * FROM rooms WHERE id=%u", room_id);
            if (get_mysql_query_result_row_count(connection, query) != 0) {
              printf("\nGame room ID is used, choose another one\n\n");

              write(current_fd, "Game room ID is used, choose another one\n", strlen("Game room ID is used, choose another one\n"));

              continue;
            }

            printf("done\n");



            // create public room successfully

            // 在 rooms table 新增 id 為 room_id 的房間並把 host 設為 row[1] 以及 class 設為 1 (public)
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "INSERT INTO rooms (id, class, host, round, code) VALUES (%u, 0, %d, 0, %u);", room_id, user_id, room_code);
            execute_mysql_query(connection, query);

            // 在 users table 和當前 fd match 的玩家的 roomid 設為 roomid
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=%d WHERE online_fd=%d;", room_id, current_fd);
            execute_mysql_query(connection, query);



            // return successful message
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "You create private game room %u\n", room_id);
            write(current_fd, success_message, strlen(success_message));

            printf("\n");
          }





          // if (strncmp(buffer, "join room", strlen("join room")) == 0) {
          //   // parser room id
          //   unsigned int room_id;
          //   sscanf(buffer, "join room %u", room_id);
          //   char username[USERNAME_SIZE] = {0};  // current username
          //   char query[QUERY_SIZE] = {0};
          //   // 從 database 的 users table 找出和當前 fd match 的 roomid
          //   // not login Fail(1)
          //   // in room Fail(2)
          //   memset(query, 0, QUERY_SIZE);
          //   sprintf(query,
          //           "SELECT roomid, username FROM users WHERE onlinefd=%d",
          //           current_fd);
          //   execute_mysql_query(query);
          //   MYSQL_RES *result = mysql_use_result(connection);
          //   MYSQL_ROW row = mysql_fetch_row(result);
          //   if (row == NULL) {  // Fail(1)
          //     write(current_fd, "You are not logged in",
          //           strlen("You are not logged in"));
          //     continue;
          //   }
          //   if (row[0] != ?????) { // Fail(2)
          //     char fail_message[ROW_SIZE] = {0};
          //     sprintf(fail_message,
          //             "You are already in game room %s, please leave game
          //             room", row[0]);
          //     write(current_fd, fail_message, ROW_SIZE);
          //     continue;
          //   }
          //   sprintf(username, "%s", row[1]);
          //   mysql_free_result(result);
          //   // 從 database 的 rooms table 找和 room_id match 的 id & class &
          //   // round
          //   memset(query, 0, QUERY_SIZE);
          //   sprintf(query, "SELECT id, class, round FROM rooms WHERE id=%u",
          //           room_id);
          //   execute_mysql_query(query);
          //   MYSQL_RES *result = mysql_use_result(connection);
          //   MYSQL_ROW row = mysql_fetch_row(result);
          //   if (row == NULL) {
          //     char fail_message[ROW_SIZE] = {0};
          //     sprintf(fail_message, "Game room %u is not exist", room_id);
          //     write(current_fd, fail_message, ROW_SIZE);
          //     continue;
          //   }
          //   if (strcmp(row[1], "0") ==
          //       0) {  // 如果 class = 0 代表是 private room
          //     write(current_fd,
          //           "Game room is private, please join game by invitation
          //           code", strlen("Game room is private, please join game by
          //           "
          //                  "invitation code"));
          //     continue;
          //   }
          //   if (strcat(row[2], "0") !=
          //       0) {  // 如果 round 不是 0, 代表遊戲進行中
          //     write(current_fd,
          //           "Game has started, you can't join now",
          //           strlen("Game has started, you can't join now"));
          //     continue;
          //   }
          //   mysql_free_result(result);
          //   // success
          //   // You join game room <game room id>
          //   char success_message[ROW_SIZE] = {0};
          //   sprintf(success_message, "You join game room %u", room_id);
          //   write(current_fd, success_message, ROW_SIZE);
          //   // send to others in the room
          //   memset(query, 0, QUERY_SIZE);
          //   sprintf(query, "SELECT onlinefd FROM users WHERE roomid=%u",
          //           room_id);
          //   execute_mysql_query(connection, query);
          //   result = mysql_use_result(result);
          //   while ((row = mysql_fetch_row(result)) != NULL) {
          //     int other_user_in_room;
          //     sscanf(row[0], "%d", other_user_in_room);
          //     char temp[ROW_SIZE] = {0};
          //     sscanf(temp, "Welcome, %s to game!", username);
          //     write(other_user_in_room, temp, ROW_SIZE);
          //   }
          //   // TODO: 更改 users table 中現在 user 的 room_id
          // }
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
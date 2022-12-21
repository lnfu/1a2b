#define TEST
#define LOCAL_DATABASE

#ifndef SERVER
#define SERVER 1
#endif

#define PORT 8888
#define EPOLL_SIZE 50
#define BUFFER_SIZE 1024
#define USERNAME_SIZE 256
#define EMAIL_SIZE 256
#define PASSWORD_SIZE 256
#define QUERY_SIZE 512
#define ROW_SIZE 64


#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/error.h"
#include "../include/game.h"
#include "../include/mysql.h"
#include "../include/network.h"


// does buffer match command keyword?
int is_string_match(const char *buffer, const char *keyword) { return (strncmp(buffer, keyword, strlen(keyword)) == 0); }





int main(int argc, char *argv[]) {
  // MySQL variables
  MYSQL *connection = mysql_init(NULL);
  char query[QUERY_SIZE] = {0};

  unsigned int aws_rds_port = 3306;
#ifndef LOCAL_DATABASE  // remote
  char *aws_rds_ip = "database-1.ctdgsyao1atl.us-east-1.rds.amazonaws.com";
  char *aws_rds_user = "admin";
  char *aws_rds_password = "12345678";
  char *aws_rds_database = "db";
#endif
#ifdef LOCAL_DATABASE  // local
  char *aws_rds_ip = "localhost";
  char *aws_rds_user = "root";
  char *aws_rds_password = "Alchemy@0)3)8)1";
  char *aws_rds_database = "db";
#endif





  // socket variables
  int tcp_socket_for_listen;
  int udp_socket;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;





  // EPOLL variables (socket)
  int epoll_fd;                      // file descriptor of epoll
  struct epoll_event *epoll_events;  // store "read change" fd this time





  // MySQL connect to AWS RDS
  mysql_connect_remote_database(connection, aws_rds_ip, aws_rds_port, aws_rds_user, aws_rds_password, aws_rds_database);
#ifdef TEST  // clear data every time
  printf("clear data... ");
  memset(query, 0, QUERY_SIZE);
  sprintf(query, "TRUNCATE TABLE users;");
  execute_mysql_query(connection, query);

  memset(query, 0, QUERY_SIZE);
  sprintf(query, "TRUNCATE TABLE rooms;");
  execute_mysql_query(connection, query);

  memset(query, 0, QUERY_SIZE);
  sprintf(query, "TRUNCATE TABLE invitations;");
  execute_mysql_query(connection, query);

  memset(query, 0, QUERY_SIZE);
  sprintf(query, "TRUNCATE TABLE servers;");
  execute_mysql_query(connection, query);

  printf("done\n");
#endif





  // set server address
  set_server_address(&server_address, PORT);





  // init epoll
  epoll_fd = epoll_create(EPOLL_SIZE);
  epoll_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);





  // create socket including bind and listen(only tcp) and add to epoll
  create_tcp_socket(&tcp_socket_for_listen, &server_address, epoll_fd);
  create_udp_socket(&udp_socket, &server_address, epoll_fd);
  printf("\n");





  // init server table
  memset(query, 0, QUERY_SIZE);
  sprintf(query, "INSERT INTO servers (id, number_of_user) VALUES (%d, 0);", SERVER);
  execute_mysql_query(connection, query);





  while (1) {
    int event_count = epoll_wait(epoll_fd, epoll_events, EPOLL_SIZE, -1);  // -1 = timeout disable





    if (event_count == -1) {
      puts("epoll_wait() error");
      break;
    }





    for (int i = 0; i < event_count; i++) {
      int current_fd = epoll_events[i].data.fd;





      if (current_fd == udp_socket) {
        // transfer by udp socket
        socklen_t client_address_size = sizeof(client_address);
        char buffer[BUFFER_SIZE] = {0};
        recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, &client_address_size);
        printf("\033[0;32m%s\n\033[0m", buffer);





        // * register <username> <email> <user password>
        if (is_string_match(buffer, "register")) {
          char username[USERNAME_SIZE] = {0};
          char email[EMAIL_SIZE] = {0};
          char passwd[PASSWORD_SIZE] = {0};





          // parse data
          sscanf(buffer, "register %s %s %s", username, email, passwd);





          // check unique username and email
          printf("Checking unique username and same email... ");
          if (!username_and_email_are_unique(connection, username, email)) {
            printf("same name or email!\n");
            sendto(udp_socket, "Username or Email is already used\n", strlen("Username or Email is already used\n"), 0,
                   (struct sockaddr *)&client_address, client_address_size);

            printf("Fail(1)\n");
            continue;
          }
          printf("done\n");




          // Success

          // register a new user and return successful message
          register_user(connection, username, email, passwd);

          // return success message
          sendto(udp_socket, "Register Successfully\n", strlen("Register Successfully\n"), 0, (struct sockaddr *)&client_address,
                 client_address_size);

          printf("\n");
        }





        // list users
        if (is_string_match(buffer, "list users")) {
          int count = 0;
          MYSQL_RES *result;
          MYSQL_ROW row;





          // clear buffer
          memset(buffer, 0, BUFFER_SIZE);
          strcat(buffer, "List Users\n");





          // no users
          if (get_mysql_query_result_row_count(connection, "SELECT * FROM users;") == 0) {
            strcat(buffer, "No Users\n");
            sendto(udp_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&client_address, client_address_size);

            printf("\n");
            continue;
          }




          // query
          execute_mysql_query(connection, "SELECT username, email, online_fd FROM users ORDER BY username;");
          result = mysql_use_result(connection);
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





        // list rooms
        if (is_string_match(buffer, "list rooms")) {
          int count = 0;
          MYSQL_RES *result;
          MYSQL_ROW row;





          // clear buffer
          memset(buffer, 0, BUFFER_SIZE);
          strcat(buffer, "List Game Rooms\n");





          // no users
          if (get_mysql_query_result_row_count(connection, "SELECT * FROM rooms;") == 0) {
            strcat(buffer, "No Rooms\n");
            sendto(udp_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&client_address, client_address_size);

            printf("\n");
            continue;
          }





          // query
          execute_mysql_query(connection, "SELECT class, id, round FROM rooms ORDER BY id;");
          result = mysql_use_result(connection);
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





        if (data_len == 0) {  // close request
          int user_id = 0;    // user_id in users table where online_fd = current_fd
          unsigned int current_room_id = 0;
          char current_login_username[USERNAME_SIZE] = {0};



          printf("\033[0;32mSPECIAL EXIT\033[0m\n");




          // get user id
          user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
          // ! user_id exists --> current_fd has logged in --> logout first
          if (user_id != 0) {
            // ! the user is in room --> leave room first
            if (check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
              // ! the user is host --> leave room and delete room first
              if (is_user_host(connection, user_id)) {
                // update current user's room to null
                memset(query, 0, QUERY_SIZE);
                sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE online_fd=%d;", current_fd);
                execute_mysql_query(connection, query);

                // update other user's room to null
                memset(query, 0, QUERY_SIZE);
                sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE room_id=%u;", current_room_id);
                execute_mysql_query(connection, query);

                // TODO: change the serial number of user after him

                // delete room
                // DELETE FROM table_name WHERE condition;
                memset(query, 0, QUERY_SIZE);
                sprintf(query, "DELETE FROM rooms WHERE id=%u;", current_room_id);
                execute_mysql_query(connection, query);





              } else if (is_room_start(connection, current_room_id)) {  // the user is NOT host and the room is playing game
                printf("USER IS NOT HOST BUT ROOM IS PLAYING\n");
                fflush(stdout);

                // !needed?
                // char success_message[ROW_SIZE] = {0};
                // sprintf(success_message, "You leave game room %u, game ends\n", current_room_id);
                // write(current_fd, success_message, strlen(success_message));

                // update current user's room to null
                memset(query, 0, QUERY_SIZE);
                sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE online_fd=%d;", current_fd);
                execute_mysql_query(connection, query);

                // ! needed?
                // broadcast to others
                // char broadcast_message[ROW_SIZE] = {0};
                // sprintf(broadcast_message, "%s leave game room %u, game ends\n", current_login_username, current_room_id);
                // send_message_to_others_in_room(connection, current_room_id, broadcast_message);


                // TODO: change the serial number of user after him

                // TODO: make room round = 0





              } else {  // the user is NOT host and the room is NOT playing game

                // ! needed?
                // char success_message[ROW_SIZE] = {0};
                // sprintf(success_message, "You leave game room %u\n", current_room_id);
                // write(current_fd, success_message, strlen(success_message));

                // update current user's room to null
                memset(query, 0, QUERY_SIZE);
                sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE online_fd=%d;", current_fd);
                execute_mysql_query(connection, query);

                // ! needed?
                // broadcast to others
                // char broadcast_message[ROW_SIZE] = {0};
                // sprintf(broadcast_message, "%s leave game room %u\n", current_login_username, current_room_id);
                // send_message_to_others_in_room(connection, current_room_id, broadcast_message);

                // TODO: change the serial number of user after him
              }
            }





            // TODO: logout
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET online_fd=NULL WHERE id=%d", user_id);
            execute_mysql_query(connection, query);


            // update server table
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE servers SET number_of_user=number_of_user-1 WHERE id=%d;", SERVER);
            execute_mysql_query(connection, query);
          }




          // close request
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);  // delete from epoll
          close(current_fd);                                     // socket close
          printf("closed client: %d\n\n", current_fd);





        } else {
          if (is_string_match(buffer, "status")) {
            for (int i = 0; i < 3; i++) {
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "SELECT number_of_user FROM servers WHERE id=%d;", i + 1);
              execute_mysql_query(connection, query);

              MYSQL_RES *result = mysql_use_result(connection);
              MYSQL_ROW row = mysql_fetch_row(result);

              char report_message[ROW_SIZE] = {0};
              sprintf(report_message, "Server%d: %s\n", i + 1, row[0]);
              write(current_fd, report_message, strlen(report_message));

              mysql_free_result(result);
            }
          }
          // login <username> <password> (login_sample)
          if (is_string_match(buffer, "login")) {
            MYSQL_RES *result;
            MYSQL_ROW row;
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
            if (!check_user_is_not_logged_in_by_username(connection, input_username)) {
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


            // update server table
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE servers SET number_of_user=number_of_user+1 WHERE id=%d;", SERVER);
            execute_mysql_query(connection, query);


            printf("\n");
          }





          // logout (logout_sample)
          if (is_string_match(buffer, "logout")) {
            unsigned int current_room_id = 0;
            int user_id = 0;
            char current_login_username[USERNAME_SIZE] = {0};



            // Fail(1) You are not logged in
            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id == 0) {
              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              printf("Fail(1)\n\n");
              continue;
            }


            // Fail(2) You are already in game room <game room id>, please leave game room
            if (check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "You are already in game room %u, please leave game room\n", current_room_id);
              write(current_fd, fail_message, strlen(fail_message));

              printf("Fail(2)\n\n");
              continue;
            }


            // Success Goodbye, <username>
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "Goodbye, %s\n", current_login_username);
            write(current_fd, success_message, strlen(success_message));


            // update users table
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET online_fd=NULL WHERE username='%s'", current_login_username);
            execute_mysql_query(connection, query);

            // update server table
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE servers SET number_of_user=number_of_user-1 WHERE id=%d;", SERVER);
            execute_mysql_query(connection, query);


            printf("\n");
          }





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
            if (check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
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
            sprintf(query, "INSERT INTO rooms (id, class, host, round, current_serial_number) VALUES (%u, 1, %d, 0, 0);", input_room_id,
                    user_id);
            execute_mysql_query(connection, query);

            // 在 users table 和當前 fd match 的玩家的 roomid 設為 roomid
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=%d, serial_number_in_room=0 WHERE online_fd=%d;", input_room_id, current_fd);
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
            if (check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
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
            sprintf(query, "INSERT INTO rooms (id, class, host, round, code, current_serial_number) VALUES (%u, 0, %d, 0, %u, 0);",
                    input_room_id, user_id, input_room_code);
            execute_mysql_query(connection, query);

            // 在 users table 和當前 fd match 的玩家的 roomid 設為 roomid
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=%d, serial_number_in_room=0 WHERE online_fd=%d;", input_room_id, current_fd);
            execute_mysql_query(connection, query);

            // return successful message
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "You create private game room %u\n", input_room_id);
            write(current_fd, success_message, strlen(success_message));

            printf("\n");
          }



          // join room
          if (is_string_match(buffer, "join room")) {
            unsigned int input_room_id = 0;
            unsigned int current_room_id = 0;
            int user_id = 0;
            char current_login_username[USERNAME_SIZE] = {0};





            // parser room id
            sscanf(buffer, "join room %u", &input_room_id);


            // Fail(1) You are not logged in
            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id == 0) {
              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              printf("Fail(1)\n\n");
              continue;
            }



            // Fail(2) You are already in game room <game room id>, please leave game room
            if (check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "You are already in game room %u, please leave game room\n", current_room_id);
              write(current_fd, fail_message, strlen(fail_message));

              printf("Fail(2)\n\n");
              continue;
            }


            // Fail(3) Game room <game room id> is not exist
            if (room_id_is_unique(connection, input_room_id)) {
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "Game room %u is not exist\n", input_room_id);
              write(current_fd, fail_message, strlen(fail_message));

              printf("Fail(3)\n\n");
              continue;
            }




            // Fail(4) Game room is private, please join game by invitation code
            if (!is_room_public(connection, input_room_id)) {
              write(current_fd, "Game room is private, please join game by invitation code\n",
                    strlen("Game room is private, please join game by invitation code\n"));

              printf("Fail(4)\n\n");
              continue;
            }



            // Fail(5) Game has started, you can't join now
            if (is_room_start(connection, input_room_id)) {
              write(current_fd, "Game has started, you can't join now\n", strlen("Game has started, you can't join now\n"));

              printf("Fail(5)\n\n");
              continue;
            }



            // Success
            // Response to you : You join game room<game room id>
            // Response to others that joined game room : Welcome, <user name> to game !


            // You join game room <game room id>
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "You join game room %u\n", input_room_id);
            write(current_fd, success_message, strlen(success_message));


            // send to others in the room
            char broadcast_message[ROW_SIZE] = {0};
            sprintf(broadcast_message, "Welcome, %s to game!\n", current_login_username);
            send_message_to_others_in_room(connection, input_room_id, broadcast_message);

            // 更改 users table 中現在 user 的 room_id
            int serial_number = get_room_user_count(connection, input_room_id);
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=%d, serial_number_in_room=%d WHERE online_fd=%d;", input_room_id, serial_number,
                    current_fd);
            execute_mysql_query(connection, query);

            printf("\n");
          }




          // leave room
          if (is_string_match(buffer, "leave room")) {
            unsigned int current_room_id = 0;
            int user_id = 0;
            char current_login_username[USERNAME_SIZE] = {0};




            // Fail(1) You are not logged in
            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id == 0) {
              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              printf("Fail(1)\n\n");
              continue;
            }


            // Fail(2) You did not join any game room
            if (!check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
              write(current_fd, "You did not join any game room\n", strlen("You did not join any game room\n"));

              printf("Fail(2)\n\n");
              continue;
            }


            // Success(1)
            // Response to you : You leave game room <game room id>
            // Response to others : Game room manager leave game room <game room id>, you are forced to leave too
            if (is_user_host(connection, user_id)) {
              char success_message[ROW_SIZE] = {0};
              sprintf(success_message, "You leave game room %u\n", current_room_id);
              write(current_fd, success_message, strlen(success_message));

              // update current user's room to null
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE online_fd=%d;", current_fd);
              execute_mysql_query(connection, query);

              // broadcast to others
              char broadcast_message[ROW_SIZE] = {0};
              sprintf(broadcast_message, "Game room manager leave game room %u, you are forced to leave too\n", current_room_id);
              send_message_to_others_in_room(connection, current_room_id, broadcast_message);

              // update other user's room to null
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE room_id=%u;", current_room_id);
              execute_mysql_query(connection, query);


              // delete room
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "DELETE FROM rooms WHERE id=%u;", current_room_id);
              execute_mysql_query(connection, query);

              // delete invitation
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "DELETE FROM invitations WHERE room_id=%u;", current_room_id);
              execute_mysql_query(connection, query);



              printf("\n");
              continue;
            }





            // Success(2)
            // Response to you : You leave game room <game room id>, game ends
            // Response to others: <user name> leave game room <game room id>, game ends
            if (is_room_start(connection, current_room_id)) {
              char success_message[ROW_SIZE] = {0};
              sprintf(success_message, "You leave game room %u, game ends\n", current_room_id);
              write(current_fd, success_message, strlen(success_message));

              // update current user's room to null
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE online_fd=%d;", current_fd);
              execute_mysql_query(connection, query);

              // broadcast to others
              char broadcast_message[ROW_SIZE] = {0};
              sprintf(broadcast_message, "%s leave game room %u, game ends\n", current_login_username, current_room_id);
              send_message_to_others_in_room(connection, current_room_id, broadcast_message);


              // change the serial number of user after him
              memset(query, 0, QUERY_SIZE);
              int current_serial_number = get_current_serial_number_in_room(connection, current_room_id);
              sprintf(query,
                      "UPDATE users SET serial_number_in_room=serial_number_in_room-1 WHERE room_id=%d AND serial_number_in_room>%d;",
                      current_room_id, current_serial_number);
              execute_mysql_query(connection, query);


              // make room round = 0, current_serial_number=0, answer=NULL
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "UPDATE rooms SET round=0, current_serial_number=0, answer=NULL WHERE id=%d;", current_room_id);
              execute_mysql_query(connection, query);


              printf("\n");
              continue;
            }





            // Success(3)
            // Response to you : You leave game room <game room id>
            // Response to others : <user name> leave game room <game room id>
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "You leave game room %u\n", current_room_id);
            write(current_fd, success_message, strlen(success_message));

            // update current user's room to null
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE online_fd=%d;", current_fd);
            execute_mysql_query(connection, query);

            // broadcast to others
            char broadcast_message[ROW_SIZE] = {0};
            sprintf(broadcast_message, "%s leave game room %u\n", current_login_username, current_room_id);
            send_message_to_others_in_room(connection, current_room_id, broadcast_message);


            // change the serial number of user after him
            memset(query, 0, QUERY_SIZE);
            int current_serial_number = get_current_serial_number_in_room(connection, current_room_id);
            sprintf(query, "UPDATE users SET serial_number_in_room=serial_number_in_room-1 WHERE room_id=%d AND serial_number_in_room>%d;",
                    current_room_id, current_serial_number);
            execute_mysql_query(connection, query);

            printf("\n");
          }





          if (is_string_match(buffer, "invite")) {
            char input_email[EMAIL_SIZE] = {0};
            char current_login_username[USERNAME_SIZE] = {0};
            int user_id = 0;
            unsigned int current_room_id = 0;
            int invitee_online_fd = 0;
            char inviter_email[EMAIL_SIZE] = {0};
            char inviter_username[USERNAME_SIZE] = {0};
            char invitee_username[USERNAME_SIZE] = {0};

            // parser email
            sscanf(buffer, "invite %s", input_email);


            // Fail(1) You are not logged in
            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id == 0) {
              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              printf("Fail(1)\n\n");
              continue;
            }


            // Fail(2) You did not join any game room
            if (!check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
              write(current_fd, "You did not join any game room\n", strlen("You did not join any game room\n"));

              printf("Fail(2)\n\n");
              continue;
            }

            // Fail(3) You are not private game room manager
            if (is_room_public(connection, current_room_id) || !is_user_host(connection, user_id)) {
              write(current_fd, "You are not private game room manager\n", strlen("You are not private game room manager\n"));

              printf("Fail(3)\n\n");
              continue;
            }


            // Fail(4) Invitee not logged in
            invitee_online_fd = get_online_fd_by_email(connection, input_email);
            if (invitee_online_fd == 0) {
              write(current_fd, "Invitee not logged in\n", strlen("Invitee not logged in\n"));

              printf("Fail(4)\n\n");
              continue;
            }


            // Success
            get_email_by_online_fd(connection, current_fd, inviter_email);

            get_username_by_online_fd(connection, current_fd, inviter_username);



            char invitee_message[ROW_SIZE] = {0};
            sprintf(invitee_message, "You receive invitation from %s<%s>\n", inviter_username, inviter_email);
            write(invitee_online_fd, invitee_message, strlen(invitee_message));



            char inviter_message[ROW_SIZE] = {0};
            get_username_by_online_fd(connection, invitee_online_fd, invitee_username);
            sprintf(inviter_message, "You send invitation to %s<%s>\n", invitee_username, input_email);
            write(current_fd, inviter_message, strlen(inviter_message));


            // check repeat invitation
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT * FROM invitations WHERE inviter_username='%s' AND invitee_online_fd=%d;", inviter_username,
                    invitee_online_fd);
            if (get_mysql_query_result_row_count(connection, query) > 0) {
              printf("\n");

              continue;
            }

            unsigned int invitation_code = get_room_invitation_code(connection, current_room_id);
            memset(query, 0, QUERY_SIZE);
            sprintf(query,
                    "INSERT INTO invitations (inviter_username, inviter_email, room_id, invitation_code, invitee_online_fd) VALUES "
                    "('%s', '%s', %u, %u, %d);",
                    inviter_username, inviter_email, current_room_id, invitation_code, invitee_online_fd);
            execute_mysql_query(connection, query);

            printf("\n");
          }



          if (is_string_match(buffer, "list invitations")) {
            memset(buffer, 0, BUFFER_SIZE);
            strcat(buffer, "List invitations\n");





            // No Invitations
            if (get_mysql_query_result_row_count(connection, "SELECT * FROM invitations;") == 0) {
              strcat(buffer, "No Invitations\n");
              write(current_fd, buffer, strlen(buffer));

              printf("\n");
              continue;
            }



            MYSQL_RES *result;
            MYSQL_ROW row;
            // query
            memset(query, 0, QUERY_SIZE);
            sprintf(query,
                    "SELECT inviter_username, inviter_email, room_id, invitation_code FROM invitations WHERE invitee_online_fd=%d ORDER BY "
                    "room_id;",
                    current_fd);
            execute_mysql_query(connection, query);
            result = mysql_use_result(connection);
            // fetch result
            int count = 0;
            while ((row = mysql_fetch_row(result)) != NULL) {
              count++;
              char temp[ROW_SIZE];
              sprintf(temp, "%d. %s<%s> invite you to join game room %s, invitation code is %s\n", count, row[0], row[1], row[2], row[3]);
              strcat(buffer, temp);
            }
            mysql_free_result(result);





            // return message
            write(current_fd, buffer, strlen(buffer));

            printf("\n");
          }



          // accept <inviter email> <invitation code>
          if (is_string_match(buffer, "accept")) {
            int user_id = 0;
            char current_login_username[USERNAME_SIZE] = {0};
            unsigned int current_room_id = 0;
            char inviter_email[EMAIL_SIZE] = {0};
            unsigned int invitation_code = 0;
            unsigned int room_id = 0;

            // parser inviter_email and invitation_code
            sscanf(buffer, "accept %s %u", inviter_email, &invitation_code);

            // Fail(1) You are not logged in
            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id == 0) {
              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              printf("Fail(1)\n\n");
              continue;
            }


            // Fail(2) You are already in game room <game room id>, please leave game room
            if (check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "You are already in game room %u, please leave game room\n", current_room_id);
              write(current_fd, fail_message, strlen(fail_message));

              printf("Fail(2)\n\n");
              continue;
            }




            // Fail(3) Invitation not exist
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT * FROM invitations WHERE inviter_email='%s' AND invitee_online_fd=%d", inviter_email, current_fd);
            if (get_mysql_query_result_row_count(connection, query) == 0) {
              write(current_fd, "Invitation not exist\n", strlen("Invitation not exist\n"));

              printf("Fail(3)\n\n");
              continue;
            }


            // Fail(4) Your invitation code is incorrect
            // memset(query, 0, QUERY_SIZE);
            // sprintf(query, "SELECT * FROM invitations WHERE inviter_email='%s' AND invitee_online_fd=%d AND invitation_code=%u",
            //         inviter_email, current_fd, invitation_code);
            // if (get_mysql_query_result_row_count(connection, query) == 0) {
            //   write(current_fd, "Your invitation code is incorrect\n", strlen("Your invitation code is incorrect\n"));

            //   printf("Fail(4)\n\n");
            //   continue;
            // }
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "SELECT room_id FROM invitations WHERE inviter_email='%s' AND invitee_online_fd=%d AND invitation_code=%u",
                    inviter_email, current_fd, invitation_code);
            execute_mysql_query(connection, query);
            MYSQL_RES *result = mysql_use_result(connection);
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row == NULL) {
              write(current_fd, "Your invitation code is incorrect\n", strlen("Your invitation code is incorrect\n"));

              printf("Fail(4)\n\n");
              continue;
            }
            sscanf(row[0], "%u", &room_id);
            mysql_free_result(result);





            // Fail(5) Game has started, you can't join now
            if (is_room_start(connection, room_id)) {
              write(current_fd, "Game has started, you can't join now\n", strlen("Game has started, you can't join now\n"));

              printf("Fail(5)\n\n");
              continue;
            }

            // success
            // You join game room <game room id>
            char success_message[ROW_SIZE] = {0};
            sprintf(success_message, "You join game room %u\n", room_id);
            write(current_fd, success_message, strlen(success_message));


            // send to others in the room
            char broadcast_message[ROW_SIZE] = {0};
            sprintf(broadcast_message, "Welcome, %s to game!\n", current_login_username);
            send_message_to_others_in_room(connection, room_id, broadcast_message);

            // 更改 users table 中現在 user 的 room_id
            int serial_number = get_room_user_count(connection, room_id);
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE users SET room_id=%d, serial_number_in_room=%d WHERE online_fd=%d;", room_id, serial_number, current_fd);
            execute_mysql_query(connection, query);

            printf("\n");
          }





          // start game <number of rounds> <guess number>
          if (is_string_match(buffer, "start game")) {
            int input_number_of_round = 0;
            int number_of_argument = 0;
            int user_id = 0;
            int current_playing_user_id = 0;
            unsigned int current_room_id = 0;
            char current_login_username[USERNAME_SIZE] = {0};
            char answer[ROW_SIZE] = {0};
            char current_playing_username[USERNAME_SIZE] = {0};

            // parser
            // memset(answer, '-', ROW_SIZE - 1);
            number_of_argument = sscanf(buffer, "start game %d %s", &input_number_of_round, answer);
            // printf("ROUND = %d, ANS = %s\n", input_number_of_round, answer);
            if (number_of_argument == 1) {
              // auto generate answer
              generate_answer(answer);
            }

            // Fail(1) You are not logged in
            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id == 0) {
              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              printf("Fail(1)\n\n");
              continue;
            }


            // Fail(2) You did not join any game room
            if (!check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
              write(current_fd, "You did not join any game room\n", strlen("You did not join any game room\n"));

              printf("Fail(2)\n\n");
              continue;
            }

            // Fail (3) You are not game room manager, you can't start game
            if (!is_user_host(connection, user_id)) {
              write(current_fd, "You are not game room manager, you can't start game\n",
                    strlen("You are not game room manager, you can't start game\n"));

              printf("Fail(3)\n\n");
              continue;
            }

            // Fail (4) Game has started, you can't start again
            if (is_room_start(connection, current_room_id)) {
              write(current_fd, "Game has started, you can't start again\n", strlen("Game has started, you can't start again\n"));

              printf("Fail(4)\n\n");
              continue;
            }

            // Fail (5) Please enter 4 digit number with leading zero
            if (number_of_argument == 2 && strlen(answer) != 4) {
              write(current_fd, "Please enter 4 digit number with leading zero\n",
                    strlen("Please enter 4 digit number with leading zero\n"));

              printf("Fail(5)\n\n");
              continue;
            }



            // Success (Broadcast to all players in game room)
            // Game start! Current player is <Current player name>
            current_playing_user_id = get_current_playing_user_id_in_room(connection, current_room_id);

            get_username_by_user_id(connection, current_playing_user_id, current_playing_username);



            char broadcast_message[ROW_SIZE] = {0};
            sprintf(broadcast_message, "Game start! Current player is %s\n", current_playing_username);
            send_message_to_others_in_room(connection, current_room_id, broadcast_message);

            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE rooms SET round=%d, current_serial_number=0, answer='%s' WHERE id=%u;", input_number_of_round, answer,
                    current_room_id);
            execute_mysql_query(connection, query);

            printf("\n");
          }




          if (is_string_match(buffer, "guess")) {
            char guess[ROW_SIZE] = {0};
            int user_id = 0;
            char current_login_username[USERNAME_SIZE] = {0};
            unsigned int current_room_id = 0;
            int current_playing_user_id = 0;
            char current_playing_username[USERNAME_SIZE] = {0};

            // parser guess number
            sscanf(buffer, "guess %s", guess);


            // Fail(1) You are not logged in
            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id == 0) {
              write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

              printf("Fail(1)\n\n");
              continue;
            }


            // Fail(2) You did not join any game room
            if (!check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
              write(current_fd, "You did not join any game room\n", strlen("You did not join any game room\n"));

              printf("Fail(2)\n\n");
              continue;
            }





            // Fail(3)
            if (!is_room_start(connection, current_room_id)) {
              if (is_user_host(connection, user_id)) {
                write(current_fd, "You are game room manager, please start game first\n",
                      strlen("You are game room manager, please start game first\n"));
              } else {
                write(current_fd, "Game has not started yet\n", strlen("Game has not started yet\n"));
              }
              printf("Fail(3)\n\n");
              continue;
            }

            // Fail(4) Please wait..., current player is <Current Player>
            current_playing_user_id = get_current_playing_user_id_in_room(connection, current_room_id);
            if (user_id != current_playing_user_id) {
              get_username_by_user_id(connection, current_playing_user_id, current_playing_username);
              char fail_message[ROW_SIZE] = {0};
              sprintf(fail_message, "Please wait..., current player is %s\n", current_playing_username);
              write(current_fd, fail_message, strlen(fail_message));

              printf("Fail(4)\n\n");
              continue;
            }

            // Fail(5) Please enter 4 digit number with leading zero
            // TODO: check "number" (0~9)
            if (strlen(guess) != 4) {
              write(current_fd, "Please enter 4 digit number with leading zero\n",
                    strlen("Please enter 4 digit number with leading zero\n"));

              continue;
            }


            // guessing...
            int a = 0;
            int b = 0;
            char answer[ROW_SIZE] = {0};
            get_room_answer(connection, current_room_id, answer);


            // Success (Bingo)
            if (game(answer, guess, &a, &b)) {
              char broadcast_message[ROW_SIZE] = {0};
              sprintf(broadcast_message, "%s guess '%s' and got Bingo!!! %s wins the game, game ends\n", current_login_username, guess,
                      current_login_username);
              send_message_to_others_in_room(connection, current_room_id, broadcast_message);

              // set room --> round=0, current_serial_number=0, answer=NULL
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "UPDATE rooms SET round=0, current_serial_number=0, answer=NULL WHERE id=%u;", current_room_id);
              execute_mysql_query(connection, query);

              printf("Success (Bingo)\n\n");

              continue;
            }




            // Success (Not Bingo)
            // TODO: updates current_serial_number + 1
            memset(query, 0, QUERY_SIZE);
            sprintf(query, "UPDATE rooms SET current_serial_number=current_serial_number+1 WHERE id=%u;", current_room_id);
            execute_mysql_query(connection, query);




            // TODO: if current_serial_number == get_room_user_count --> update round - 1
            if (get_current_serial_number_in_room(connection, current_room_id) == get_room_user_count(connection, current_room_id)) {
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "UPDATE rooms SET current_serial_number=0, round=round-1 WHERE id=%u;", current_room_id);
              execute_mysql_query(connection, query);
            }




            // if round == 0 --> Game end
            // Game ends : Bob guess '3214' and got '2A2B' Game ends, no one wins
            if (!is_room_start(connection, current_room_id)) {
              char broadcast_message[ROW_SIZE] = {0};
              sprintf(broadcast_message, "%s guess '%s' and got '%dA%dB'\nGame ends, no one wins\n", current_login_username, guess, a, b);
              send_message_to_others_in_room(connection, current_room_id, broadcast_message);

              // set room --> round=0, current_serial_number=0, answer=NULL
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "UPDATE rooms SET round=0, current_serial_number=0, answer=NULL WHERE id=%u;", current_room_id);
              execute_mysql_query(connection, query);

              printf("Success (No chances)\n\n");
              continue;
            }



            // Continue
            char broadcast_message[ROW_SIZE] = {0};
            sprintf(broadcast_message, "%s guess '%s' and got '%dA%dB'\n", current_login_username, guess, a, b);
            send_message_to_others_in_room(connection, current_room_id, broadcast_message);

            printf("Success (No Bingo)\n\n");



            continue;
            printf("BREAK POINT\n");

            // char broadcast_message[ROW_RESULT] = {0};
            // sprintf(broadcast_message, "%s guess '%s' and got '%dA%dB'\n", current_login_username, guess, a, b);
            // send_message_to_others_in_room(connection, current_room_id, broadcast_message);

            // printf("Success (Not Bingo)\n\n");
          }




          if (is_string_match(buffer, "exit")) {
            char current_login_username[USERNAME_SIZE] = {0};
            int user_id = 0;
            unsigned int current_room_id = 0;
            // close request



            user_id = get_user_id_by_current_fd(connection, current_fd, current_login_username);
            if (user_id != 0) {
              if (check_current_fd_is_in_room(connection, current_fd, &current_room_id)) {
                // TODO: leave room

                if (is_user_host(connection, user_id)) {  // the user is host

                  printf("USER IS HOST\n");
                  fflush(stdout);

                  // ! needed?
                  // char success_message[ROW_SIZE] = {0};
                  // sprintf(success_message, "You leave game room %u\n", current_room_id);
                  // write(current_fd, success_message, strlen(success_message));

                  // update current user's room to null
                  memset(query, 0, QUERY_SIZE);
                  sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE online_fd=%d;", current_fd);
                  execute_mysql_query(connection, query);

                  // ! needed?
                  // broadcast to others
                  // char broadcast_message[ROW_SIZE] = {0};
                  // sprintf(broadcast_message, "Game room manager leave game room %u, you are forced to leave too\n", current_room_id);
                  // send_message_to_others_in_room(connection, current_room_id, broadcast_message);

                  // update other user's room to null
                  memset(query, 0, QUERY_SIZE);
                  sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE room_id=%u;", current_room_id);
                  execute_mysql_query(connection, query);

                  // TODO: change the serial number of user after him

                  // delete room
                  // DELETE FROM table_name WHERE condition;
                  memset(query, 0, QUERY_SIZE);
                  sprintf(query, "DELETE FROM rooms WHERE id=%u;", current_room_id);
                  execute_mysql_query(connection, query);





                } else if (is_room_start(connection, current_room_id)) {  // the user is NOT host and the room is playing game

                  // !needed?
                  // char success_message[ROW_SIZE] = {0};
                  // sprintf(success_message, "You leave game room %u, game ends\n", current_room_id);
                  // write(current_fd, success_message, strlen(success_message));

                  // update current user's room to null
                  memset(query, 0, QUERY_SIZE);
                  sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE online_fd=%d;", current_fd);
                  execute_mysql_query(connection, query);

                  // ! needed?
                  // broadcast to others
                  // char broadcast_message[ROW_SIZE] = {0};
                  // sprintf(broadcast_message, "%s leave game room %u, game ends\n", current_login_username, current_room_id);
                  // send_message_to_others_in_room(connection, current_room_id, broadcast_message);


                  // TODO: change the serial number of user after him

                  // TODO: make room round = 0





                } else {  // the user is NOT host and the room is NOT playing game

                  // ! needed?
                  // char success_message[ROW_SIZE] = {0};
                  // sprintf(success_message, "You leave game room %u\n", current_room_id);
                  // write(current_fd, success_message, strlen(success_message));

                  // update current user's room to null
                  memset(query, 0, QUERY_SIZE);
                  sprintf(query, "UPDATE users SET room_id=NULL, serial_number_in_room=NULL WHERE online_fd=%d;", current_fd);
                  execute_mysql_query(connection, query);

                  // ! needed?
                  // broadcast to others
                  // char broadcast_message[ROW_SIZE] = {0};
                  // sprintf(broadcast_message, "%s leave game room %u\n", current_login_username, current_room_id);
                  // send_message_to_others_in_room(connection, current_room_id, broadcast_message);

                  // TODO: change the serial number of user after him
                }
              }





              // TODO: logout
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "UPDATE users SET online_fd=NULL WHERE id=%d", user_id);
              execute_mysql_query(connection, query);



              // update server table
              memset(query, 0, QUERY_SIZE);
              sprintf(query, "UPDATE servers SET number_of_user=number_of_user-1 WHERE id=%d;", SERVER);
              execute_mysql_query(connection, query);
            }




            // close request
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);  // delete from epoll
            close(current_fd);                                     // socket close
            printf("closed client: %d\n\n", current_fd);
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
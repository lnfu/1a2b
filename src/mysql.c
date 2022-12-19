#define QUERY_SIZE 256
#define ROW_SIZE 64

#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>  // exit(1)
#include <string.h>
#include <unistd.h>

void mysql_connect_remote_database(MYSQL *connection, char *ip_address, unsigned int port, char *username, char *password, char *database) {
  printf("Connect to remote database... ");
  if (!mysql_real_connect(connection, ip_address, username, password, database, port, NULL, 0)) {
    fprintf(stderr, "%s\n", mysql_error(connection));
    exit(1);
  }
  if (mysql_select_db(connection, database)) {
    fprintf(stderr, "%s\n", mysql_error(connection));
    mysql_close(connection);
    exit(1);
  }
  printf("done\n");
}

void execute_mysql_query(MYSQL *connection, const char *query) {
  // printf("%s\n", query);
  // printf("Querying... ");

  if (mysql_query(connection, query)) {
    fprintf(stderr, "%s\n", mysql_error(connection));
    mysql_close(connection);
    exit(1);
  }

  // printf("done\n");
}

void execute_mysql_query_and_print(MYSQL *connection, const char *query, const int column) {
  // * MySQL query
  execute_mysql_query(connection, query);
  printf("Print results...\n");
  MYSQL_RES *result = mysql_use_result(connection);
  MYSQL_ROW row;

  // * MySQL print results
  while ((row = mysql_fetch_row(result)) != NULL) {
    for (int i = 0; i < column; i++) {
      printf("%s ", row[i]);
    }
    printf("\n");
  }
  printf("---- end of results ----\n\n");

  mysql_free_result(result);
}

int get_mysql_query_result_row_count(MYSQL *connection, const char *query) {
  // * MySQL query
  execute_mysql_query(connection, query);

  MYSQL_RES *result = mysql_store_result(connection);
  int query_result_row_count = mysql_num_rows(result);
  mysql_free_result(result);

  return query_result_row_count;
}

unsigned int get_room_id_for_online_fd(MYSQL *connection, const int online_fd) {
  char query[QUERY_SIZE] = {0};
  sprintf(query, "SELECT roomid FROM users WHERE onlinefd=%d", online_fd);
  execute_mysql_query(connection, query);

  MYSQL_RES *result = mysql_use_result(connection);
  MYSQL_ROW row = mysql_fetch_row(result);

  if (row == NULL) {
    // not in room!
    printf("THIS USER IS NOT IN A ROOM\n");
    return 0;
  }

  unsigned int room_id = 0;
  sscanf(row[0], "%u", room_id);

  mysql_free_result(result);

  return room_id;
}

bool username_and_email_are_unique(MYSQL *connection, const char *username, const char *email) {
  char query[QUERY_SIZE] = {0};
  sprintf(query, "SELECT * FROM users WHERE username = '%s' OR email='%s';", username, email);

  execute_mysql_query(connection, query);

  MYSQL_RES *result = mysql_store_result(connection);
  int row_count = mysql_num_rows(result);
  mysql_free_result(result);

  return (row_count == 0);
}

void register_user(MYSQL *connection, const char *username, const char *email, const char *passwd) {
  printf("Register a new user... ");

  char query[QUERY_SIZE] = {0};
  sprintf(query, "INSERT INTO users (username, email, passwd) VALUES ('%s', '%s', '%s');", username, email, passwd);
  execute_mysql_query(connection, query);

  printf("done\n");
}



// check user is logged in and not in a room
// if not, get the user id in users table
// error message will send by tcp connection
int get_user_id_if_allowed_to_enter_room(MYSQL *connection, int current_fd) {
  printf("Check user is logged in and not in a room... ");


  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[QUERY_SIZE] = {0};


  memset(query, 0, QUERY_SIZE);
  sprintf(query, "SELECT room_id, id FROM users WHERE online_fd=%d", current_fd);
  execute_mysql_query(connection, query);

  result = mysql_use_result(connection);
  row = mysql_fetch_row(result);
  if (row == NULL) {  // Fail(1)
    printf("\nYou are not logged in\n\n");

    write(current_fd, "You are not logged in\n", strlen("You are not logged in\n"));

    mysql_free_result(result);
    return 0;
  }

  if (row[0] != NULL) {  // Fail(2)
    printf("\nYou are already in game room %s, please leave game room\n\n", row[0]);

    char fail_message[ROW_SIZE] = {0};
    sprintf(fail_message, "You are already in game room %s, please leave game room\n", row[0]);
    write(current_fd, fail_message, strlen(fail_message));

    mysql_free_result(result);
    return 0;
  }





  int user_id;
  sscanf(row[1], "%d", &user_id);
  mysql_free_result(result);
  printf("done\n");
  return user_id;
}


// 看有沒有重複 room_id
// error message will send by tcp connection
int room_id_is_unique(MYSQL *connection, int current_fd, int room_id) {
  printf("Check unique room_id... ");


  char query[QUERY_SIZE] = {0};

  sprintf(query, "SELECT * FROM rooms WHERE id=%u", room_id);
  if (get_mysql_query_result_row_count(connection, query) != 0) {
    printf("Game room ID is used, choose another one\n\n");

    write(current_fd, "Game room ID is used, choose another one\n", strlen("Game room ID is used, choose another one\n"));

    return false;
  }

  printf("done\n");
  return true;
}
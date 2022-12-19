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


unsigned int get_room_id_user_is_host(MYSQL *connection, const int current_fd) {
  printf("Check user the host of his room... ");

  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[QUERY_SIZE] = {0};

  // first query for current_fd's user id (users table)
  memset(query, 0, QUERY_SIZE);
  sprintf(query, "SELECT id FROM users WHERE online_fd=%d", current_fd);
  execute_mysql_query(connection, query);

  result = mysql_use_result(connection);
  row = mysql_fetch_row(result);
  int id;
  sscanf(row[0], "%d", &id);


  // second to check if he is host (rooms table)
  unsigned int room_id;
  memset(query, 0, QUERY_SIZE);
  sprintf(query, "SELECT id FROM rooms WHERE host=%d", id);
  execute_mysql_query(connection, query);
  result = mysql_use_result(connection);
  row = mysql_fetch_row(result);
  if (row == NULL) {
    // not a host
    mysql_free_result(result);
    return 0;
  }
  sscanf(row[0], "%u", room_id);


  printf("done\n");
  return room_id;
}





// *********************************
int get_user_id_by_current_fd(MYSQL *connection, const int current_fd, char *current_login_username) {
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[QUERY_SIZE] = {0};
  int user_id = 0;

  printf("Check current_fd is not logged in yet... ");

  sprintf(query, "SELECT username, id FROM users WHERE online_fd=%d;", current_fd);
  execute_mysql_query(connection, query);

  result = mysql_use_result(connection);
  row = mysql_fetch_row(result);

  if (row == NULL) {
    printf("done\n");

    mysql_free_result(result);

    return 0;
  }

  sscanf(row[0], "%s", current_login_username);
  sscanf(row[1], "%d", &user_id);
  printf("current_fd already logged in as %s\n", current_login_username);

  mysql_free_result(result);

  return user_id;
}

bool check_username_exist(MYSQL *connection, const char *username) {
  char query[QUERY_SIZE] = {0};

  printf("Check the username exists... ");

  sprintf(query, "SELECT * FROM users WHERE username='%s';", username);
  if (get_mysql_query_result_row_count(connection, query) == 0) {
    printf("failed to find this user\n");

    return false;
  }

  printf("done\n");

  return true;
}

bool check_user_is_not_logged_in(MYSQL *connection, const char *username) {
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[QUERY_SIZE] = {0};

  printf("Check the user is not logged in yet... ");

  sprintf(query, "SELECT online_fd FROM users WHERE username='%s';", username);
  execute_mysql_query(connection, query);

  result = mysql_use_result(connection);
  row = mysql_fetch_row(result);

  if (row[0] != NULL) {
    printf("user is already logged in\n");

    mysql_free_result(result);

    return false;
  }

  printf("done\n");

  mysql_free_result(result);

  return true;
}

bool check_passwd_is_correct(MYSQL *connection, const char *username, const char *passwd) {
  char query[QUERY_SIZE] = {0};

  printf("Check the password is correct... ");

  sprintf(query, "SELECT * FROM users WHERE username='%s' AND passwd='%s';", username, passwd);
  if (get_mysql_query_result_row_count(connection, query) == 0) {
    printf("wrong password\n");

    return false;
  }

  printf("done\n");

  return true;
}

bool check_current_fd_is_not_in_room(MYSQL *connection, const int current_fd, unsigned int *room_id) {
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[QUERY_SIZE] = {0};

  printf("Check current_fd is not in room... ");

  sprintf(query, "SELECT room_id FROM users WHERE online_fd=%d;", current_fd);
  execute_mysql_query(connection, query);

  result = mysql_use_result(connection);
  row = mysql_fetch_row(result);

  if (row[0] == NULL) {
    printf("done\n");

    mysql_free_result(result);

    return true;
  }

  sscanf(row[0], "%u", room_id);
  printf("current_fd is in room %u\n", *room_id);

  mysql_free_result(result);

  return false;
}


// 看有沒有重複 room_id
bool room_id_is_unique(MYSQL *connection, const unsigned int room_id) {
  char query[QUERY_SIZE] = {0};

  printf("Check unique room_id... ");

  sprintf(query, "SELECT * FROM rooms WHERE id=%u", room_id);
  if (get_mysql_query_result_row_count(connection, query) != 0) {
    printf("find game room ID\n");

    return false;
  }

  printf("done\n");
  return true;
}



// tell the room class is public or private
bool is_room_public(MYSQL *connection, const unsigned int room_id) {
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[QUERY_SIZE] = {0};

  printf("Check the class of room... ");

  sprintf(query, "SELECT class FROM rooms WHERE id=%u", room_id);
  execute_mysql_query(connection, query);
  result = mysql_use_result(connection);
  row = mysql_fetch_row(result);



  if (strcmp(row[0], "0") == 0) {
    printf("private\n");

    mysql_free_result(result);
    return false;
  }

  printf("public\n");

  mysql_free_result(result);
  return true;
}

bool is_room_start(MYSQL *connection, const unsigned int room_id) {
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[QUERY_SIZE] = {0};

  printf("Check the game in the room is not playing... ");

  sprintf(query, "SELECT round FROM rooms WHERE id=%u", room_id);
  execute_mysql_query(connection, query);
  result = mysql_use_result(connection);
  row = mysql_fetch_row(result);


  if (strcmp(row[0], "0") == 0) {
    printf("done\n");

    mysql_free_result(result);
    return false;
  }

  printf("game is playing\n");

  mysql_free_result(result);
  return true;
}

bool is_user_host(MYSQL *connection, const int user_id) {
  char query[QUERY_SIZE] = {0};

  printf("Check user is host... ");

  sprintf(query, "SELECT * FROM rooms WHERE host=%d", user_id);

  if (get_mysql_query_result_row_count(connection, query) == 0) {
    printf("the user is not the host\n");
    return false;
  }

  printf("done\n");
  return true;
}


void send_message_to_others_in_room(MYSQL *connection, const unsigned int room_id, const char *message) {
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[QUERY_SIZE] = {0};

  printf("Sending message to others in the same room... ");

  memset(query, 0, QUERY_SIZE);
  sprintf(query, "SELECT online_fd, username FROM users WHERE room_id=%u", room_id);
  execute_mysql_query(connection, query);

  result = mysql_use_result(connection);
  while ((row = mysql_fetch_row(result)) != NULL) {
    // printf("\nsend message to %s\n", row[1]);

    int other_user_in_room;
    sscanf(row[0], "%d", &other_user_in_room);
    write(other_user_in_room, message, strlen(message));
  }

  printf("done\n");

  mysql_free_result(result);
}


// ********************************

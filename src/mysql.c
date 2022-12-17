#define QUERY_SIZE 256

#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>  // exit(1)

void mysql_connect_remote_database(MYSQL *connection, char *ip_address,
                                   unsigned int port, char *username,
                                   char *password, char *database) {
  printf("Connect to remote database... ");
  if (!mysql_real_connect(connection, ip_address, username, password, database,
                          port, NULL, 0)) {
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

void execute_mysql_query_and_print(MYSQL *connection, const char *query) {
  MYSQL_RES *result;
  MYSQL_ROW row;

  // * MySQL query
  printf("%s\n", query);
  printf("Querying... ");

  if (mysql_query(connection, query)) {
    fprintf(stderr, "%s\n", mysql_error(connection));
    mysql_close(connection);
    exit(1);
  }
  printf("done\n");

  printf("Print results... ");
  result = mysql_use_result(connection);

  // * MySQL print results
  while ((row = mysql_fetch_row(result)) != NULL) {
    printf("%s %s %s %s\n", row[0], row[1], row[2], row[3], row[4],
           row[5]);  // ! careful the number of column
  }
  printf("---- end ----\n\n");

  mysql_free_result(result);
}

void execute_mysql_query(MYSQL *connection, const char *query) {
  printf("%s\n", query);
  printf("Querying... ");

  if (mysql_query(connection, query)) {
    fprintf(stderr, "%s\n", mysql_error(connection));
    mysql_close(connection);
    exit(1);
  }

  printf("done\n");
}

int get_mysql_query_result_row_count(MYSQL *connection, const char *query) {
  MYSQL_RES *result;
  MYSQL_ROW row;

  // * MySQL query
  printf("%s\n", query);
  printf("Querying... ");

  if (mysql_query(connection, query)) {
    fprintf(stderr, "%s\n", mysql_error(connection));
    mysql_close(connection);
    exit(1);
  }
  printf("done\n");

  result = mysql_store_result(connection);
  int query_result_row_count = mysql_num_rows(result);
  mysql_free_result(result);

  return query_result_row_count;
}

unsigned int get_room_id_for_online_fd(MYSQL *connection, int online_fd) {
  MYSQL_RES *result;
  MYSQL_ROW row;

  // * MySQL query
  char query[QUERY_SIZE] = { 0 };
  sprintf(query, "SELECT roomid FROM users WHERE onlinefd=%d", online_fd);
  printf("%s\n", query);
  printf("Querying... ");

  if (mysql_query(connection, query)) {
    fprintf(stderr, "%s\n", mysql_error(connection));
    mysql_close(connection);
    exit(1);
  }
  printf("done\n");

  result = mysql_use_result(connection);
  row = mysql_fetch_row(result);
  unsigned int room_id = 0;
  sscanf(row[0], "%u", room_id);
  
  mysql_free_result(result);

  return room_id;
}
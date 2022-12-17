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

void mysql_query_data_and_print_results(MYSQL *connection, const char *query) {
  MYSQL_RES *result;
  MYSQL_ROW row;

  // * MySQL query
  printf("Querying [%s]... ", query);
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
  printf("done\n");

  mysql_free_result(result);
}

void mysql_query_data(MYSQL *connection, const char *query) {
  printf("%s\n", query);
  printf("Querying... ");

  if (mysql_query(connection, query)) {
    fprintf(stderr, "%s\n", mysql_error(connection));
    mysql_close(connection);
    exit(1);
  }

  printf("done\n");
}

int mysql_query_data_number_of_result(MYSQL *connection, const char *query) {
  printf("%s\n", query);
  printf("Querying... ");

  if (mysql_query(connection, query)) {
    fprintf(stderr, "%s\n", mysql_error(connection));
    mysql_close(connection);
    exit(1);
  }
}
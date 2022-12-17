void mysql_connect_remote_database(MYSQL *connection, char *ip_address,
                                   unsigned int port, char *username,
                                   char *password, char *database);
void execute_mysql_query_and_print(MYSQL *connection, const char *query);
void mysql_query_data(MYSQL *connection, const char *query);
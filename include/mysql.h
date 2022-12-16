void mysql_connect_remote_database(MYSQL *connection, char *ip_address, unsigned int port, char *username, char *password, char *database);
void mysql_query_data_and_print(MYSQL *connection, const char *query);
void mysql_query_data(MYSQL *connection, const char *query);
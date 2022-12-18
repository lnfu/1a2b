void mysql_connect_remote_database(MYSQL *connection, char *ip_address,
                                   unsigned int port, char *username,
                                   char *password, char *database);
void execute_mysql_query_and_print(MYSQL *connection, const char *query,
                                   const int column);
void execute_mysql_query(MYSQL *connection, const char *query);
int get_mysql_query_result_row_count(MYSQL *connection, const char *query);

unsigned int get_room_id_for_online_fd(MYSQL *connection, int online_fd);
bool username_and_email_are_unique(MYSQL *connection, char *username,
                                   char *email);
void register_user(MYSQL *connection, char *username, char *email,
                   char *passwd);
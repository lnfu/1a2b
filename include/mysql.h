void mysql_connect_remote_database(MYSQL *connection, char *ip_address, unsigned int port, char *username, char *password, char *database);
void execute_mysql_query_and_print(MYSQL *connection, const char *query, const int column);
void execute_mysql_query(MYSQL *connection, const char *query);
int get_mysql_query_result_row_count(MYSQL *connection, const char *query);

unsigned int get_room_id_for_online_fd(MYSQL *connection, int online_fd);
bool username_and_email_are_unique(MYSQL *connection, char *username, char *email);
void register_user(MYSQL *connection, char *username, char *email, char *passwd);
int get_user_id_if_allowed_to_enter_room(MYSQL *connection, int current_fd);
int room_id_is_unique(MYSQL *connection, int room_id);
int get_room_class(MYSQL *connection, int room_id);
bool is_game_in_room_in_progress(MYSQL *connection, int room_id);
bool is_user_logged_in_and_in_room(MYSQL *connection, const int current_fd);

unsigned int get_room_id_user_is_host(MYSQL *connection, const int current_fd);




// **************************************
bool check_current_fd_is_not_logged_in(MYSQL *connection, const int current_fd, char *current_login_username);

bool check_username_exist(MYSQL *connection, const char *username);

bool check_user_is_not_logged_in(MYSQL *connection, const char *username);

bool check_passwd_is_correct(MYSQL *connection, const char *username, const char *passwd);
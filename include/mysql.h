void mysql_connect_remote_database(MYSQL *connection, char *ip_address, unsigned int port, char *username, char *password, char *database);
void execute_mysql_query_and_print(MYSQL *connection, const char *query, const int column);
void execute_mysql_query(MYSQL *connection, const char *query);
int get_mysql_query_result_row_count(MYSQL *connection, const char *query);



// **************************************

bool username_and_email_are_unique(MYSQL *connection, char *username, char *email);
unsigned int get_room_id_user_is_host(MYSQL *connection, const int current_fd);
void register_user(MYSQL *connection, char *username, char *email, char *passwd);

// -----------------------------------------------------------

bool check_current_fd_is_not_in_room(MYSQL *connection, const int current_fd, unsigned int *room_id);
bool check_passwd_is_correct(MYSQL *connection, const char *username, const char *passwd);
bool room_id_is_unique(MYSQL *connection, const unsigned int room_id);
void send_message_to_others_in_room(MYSQL *connection, const unsigned int room_id, const char *message);
int get_user_id_by_current_fd(MYSQL *connection, const int current_fd, char *current_login_username);

// -----------------------------------------------------------

bool check_username_exist(MYSQL *connection, const char *username);
bool check_user_is_not_logged_in(MYSQL *connection, const char *username);

// -----------------------------------------------------------
bool is_room_start(MYSQL *connection, const unsigned int room_id);
bool is_room_public(MYSQL *connection, const unsigned int room_id);

// -----------------------------------------------------------

bool is_user_host(MYSQL *connection, const int user_id);

// -----------------------------------------------------------

int get_room_user_count(MYSQL *connection, const unsigned int room_id);
int get_current_playing_user_id_in_room(MYSQL *connection, const unsigned int room_id);
void get_room_answer(MYSQL *connection, const unsigned int room_id, char *answer);
int get_current_serial_number_in_room(MYSQL *connection, const unsigned int room_id);

// -----------------------------------------
void get_username_by_user_id(MYSQL *connection, const int user_id, char *username);



// --------------------
// *******************

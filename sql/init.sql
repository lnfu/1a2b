CREATE DATABASE IF NOT EXISTS db;
USE db;

CREATE TABLE IF NOT EXISTS servers (
	id INT NOT NULL PRIMARY KEY,
	number_of_user INT
);

CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(255),
    email VARCHAR(255),
    passwd VARCHAR(255),
    online_fd INT,
    room_id BIGINT,
    serial_number_in_room INT
);
CREATE TABLE IF NOT EXISTS rooms (
	id BIGINT PRIMARY KEY,
    class SMALLINT, -- public (1) / private (0)
    code VARCHAR(255), -- password
    host INT, -- users.userid
    round INT NOT NULL, -- game is not start (0) / (?) otherwise
    current_serial_number INT,
    answer VARCHAR(255)
);

CREATE TABLE IF NOT EXISTS invitations (
    id INT AUTO_INCREMENT PRIMARY KEY,
	inviter_username VARCHAR(255),
	inviter_email VARCHAR(255),
	room_id BIGINT,
	invitation_code BIGINT,
	invitee_online_fd INT
);


SHOW TABLES;
DESC users;
DESC rooms;
DESC invitations;
DESC servers;



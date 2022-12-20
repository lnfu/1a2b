CREATE DATABASE IF NOT EXISTS db;
USE db;

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

SHOW TABLES;
DESC users;
DESC rooms;



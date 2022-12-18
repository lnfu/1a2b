CREATE DATABASE IF NOT EXISTS db;
USE db;

CREATE TABLE IF NOT EXISTS users (
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(255),
    email VARCHAR(255),
    passwd VARCHAR(255),
    online_fd INT,
    room_id BIGINT
);
CREATE TABLE IF NOT EXISTS rooms (
	id BIGINT NOT NULL PRIMARY KEY,
    class SMALLINT, -- public (1) / private (0)
    code VARCHAR(255), -- password
    host INT, -- users.userid
    round INT -- game is not start (0) / (?) otherwise
);

SHOW TABLES;
DESC users;
DESC rooms;



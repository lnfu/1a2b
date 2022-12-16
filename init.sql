CREATE DATABASE IF NOT EXISTS db;
USE db;

CREATE TABLE IF NOT EXISTS users (
    userid INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(255),
    useremail VARCHAR(255),
    userpassword VARCHAR(255),
    onlinefd INT,
    roomid INT
);
CREATE TABLE IF NOT EXISTS rooms (
	roomid INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    roomclass SMALLINT, -- public / private
    roomcode VARCHAR(255), -- password
    roomhost INT -- users.userid
);

SHOW TABLES;
DESC users;
DESC rooms;



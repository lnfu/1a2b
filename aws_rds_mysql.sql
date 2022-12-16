create database if not exists db;
use db;
create table if not exists users (
    userid int not null auto_increment primary key,
    username varchar(255),
    useremail varchar(255),
    userpassword varchar(255),
    onlinefd int,
    roomid int
);
create table if not exists room (
	roomid int not null auto_increment primary key,
    roomclass smallint, -- public / private
    roomcode varchar(255), -- password
    roomhost int -- users.userid
);
show databases;
show tables;
-- desc users;
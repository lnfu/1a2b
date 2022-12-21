#!/bin/sh

export CC=gcc

if [ ! -d "build" ]; then
mkdir build
fi


if [ $# -eq 0 ]
then
    echo "SERVER = 1"
    $CC src/*  $(mysql_config --libs) $(mysql_config --cflags) -Iinclude -o build/server
else
    echo "SERVER = $1"
    $CC src/*  $(mysql_config --libs) $(mysql_config --cflags) -Iinclude -o build/server -DSERVER=$1
fi
# -Iinclude
# -L/usr/lib64/ -lmariadb
# -I/usr/include/mysql -I/usr/include/mysql/mysql

# -DSIZE=30

#define SERVER 1

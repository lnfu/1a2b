#!/bin/sh

export CC=gcc

if [ ! -d "build" ]; then
mkdir build
fi

$CC src/*  $(mysql_config --libs) $(mysql_config --cflags) -Iinclude -o build/server

# -Iinclude
# -L/usr/lib64/ -lmariadb
# -I/usr/include/mysql -I/usr/include/mysql/mysql
#!/bin/bash

if [ -z "$MYSQL_ROOT_PASSWORD" ]; then
        MYSQL_ROOT_PASSWORD=root
fi

if [ ! -e "/data/mysql/ibdata1" ]; then
    echo "Populating data dir..."
    rm -Rf /data/*
    cp -R /var/lib/mysql /data/mysql
	chown -R mysql:mysql /data
fi
rm -Rf /var/lib/mysql

echo "Starting database server"
exec "$@"

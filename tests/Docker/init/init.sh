#!/bin/bash

if [ -z "$MYSQL_DATABASE" ]; then
        echo >&2 'error: database is uninitialized and MYSQL_DATABASE not set'
        echo >&2 '  Did you forget to add -e MYSQL_DATABASE=... ?'
        exit 1
fi

if [ -z "$MYSQL_ROOT_PASSWORD" ]; then
        MYSQL_ROOT_PASSWORD=root
fi

echo "Setting MySQL server options..."
sed -i -e"s/^wait_timeout\s*=\s*[0-9]\+/wait_timeout = 31536000/" /etc/mysql/my.cnf # => One year, which is also the maximum allowed value
sed -i -e"s/^bind-address\s*=\s*127.0.0.1/bind-address = 0.0.0.0/" /etc/mysql/my.cnf

echo "Starting MySQL server..."
mysqld_safe &
mysqladmin --silent --wait=30 ping || exit 1

echo "Securing installation..."
sed -i -e"s/\\\$MYSQL_DATABASE/$MYSQL_DATABASE/" /init/mysql-secure-installation.sql
sed -i -e"s/\\\$MYSQL_ROOT_PASSWORD/$MYSQL_ROOT_PASSWORD/" /init/mysql-secure-installation.sql
mysql < /init/mysql-secure-installation.sql

echo "Creating users..."
sed -i -e"s/\\\$MYSQL_DATABASE/$MYSQL_DATABASE/" /init/create-users.sql
mysql -uroot -p${MYSQL_ROOT_PASSWORD} $MYSQL_DATABASE < /init/create-users.sql

echo "Cleaning up temporary files..."
rm -R /init
echo "Success."

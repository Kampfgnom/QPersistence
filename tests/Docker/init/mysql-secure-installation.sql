# This script does roughly the same as the mysql_secure_installation executable

USE mysql ;
GRANT ALL PRIVILEGES ON *.* TO "root"@"%" WITH GRANT OPTION;
UPDATE mysql.user SET Password=PASSWORD('$MYSQL_ROOT_PASSWORD') WHERE User='root';
DELETE FROM mysql.user WHERE User='';
DELETE FROM mysql.db WHERE Db='test' OR Db='test\_%';
DROP DATABASE IF EXISTS test ;
CREATE DATABASE IF NOT EXISTS $MYSQL_DATABASE ;
FLUSH PRIVILEGES ;
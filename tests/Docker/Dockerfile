FROM debian:jessie
RUN \
  apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 0xcbcb082a1bb943db && \
  echo "deb http://mariadb.mirror.iweb.com/repo/10.0/debian wheezy main" > /etc/apt/sources.list.d/mariadb.list && \
  apt-get update && \
  DEBIAN_FRONTEND=noninteractive apt-get install -y mariadb-server && \
  rm -rf /var/lib/apt/lists/*

ENV MYSQL_DATABASE qpersistence_testing
ADD ./init /init
RUN /init/init.sh
ADD ./entrypoint.sh /

EXPOSE 3306
VOLUME [ "/data" ]
ENTRYPOINT [ "/entrypoint.sh" ]
CMD ["mysqld_safe", "--datadir=/data/mysql", "--user=mysql" ]

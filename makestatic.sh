#!/bin/bash
cd Debug
g++  -o "pid2pgsql-static"  ./Clock.o ./main.o ./Pgsql.o ./Pid.o ../libpq-8.1.23.a -lpthread -lkrb5 -L /home/jhhudso/pid2pgsql/Debug/ -lcom_err -lssl -lcrypto -lcrypt

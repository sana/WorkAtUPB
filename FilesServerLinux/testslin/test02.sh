#!/bin/bash
#Tema 4 SO - Test 02 - din oficiu ep. 2 :)
#testeaza daca serverul foloseste API-ul dorit

source f.sh
startTest 02

nm ../server | grep epoll_ctl &&\
nm ../server | grep epoll_wait &&\
nm ../server | grep epoll_create &&\
nm ../server | grep io_setup &&\
nm ../server | grep io_submit &&\
nm ../server | grep eventfd

if [ $? -ne 0 ]
	then do_exit 1
	else do_exit 0
fi

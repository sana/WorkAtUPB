make clean
make
#valgrind ./client 127.0.0.1 wr sana.txt 0 100
valgrind ./client 127.0.0.1 rd sana.txt 0 100
#valgrind ./client 127.0.0.1 ls /home/sana/Desktop/

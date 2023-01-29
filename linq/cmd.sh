#! /bin/sh
gcc -g -o  linq -I/usr/include/SDL2 linq.c -lSDL2_image -lSDL2_ttf -lSDL2 -lpthread
gcc -g -o  server_linq server_linq.c

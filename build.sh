#!/bin/bash

gcc -o bin/jafhe src/main.c -g $(pkg-config --cflags --libs gtk+-3.0) 
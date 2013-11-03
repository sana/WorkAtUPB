#!/bin/bash

make clean
make
make -f Makefile.checker build-pre && make build && make -f Makefile.checker build-post && make -f Makefile.checker

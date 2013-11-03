#!/bin/bash

killall -9 tnameserv
make clean
make
make run_debugger


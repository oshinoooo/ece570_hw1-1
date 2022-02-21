#!/bin/bash

g++ disk.cc lib/thread.o lib/libinterrupt.a -ldl -o disk
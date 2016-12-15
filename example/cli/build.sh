#!/bin/sh
g++ pink.pb.cc pink.pb.h cli.cc -std=c++11 -lprotobuf -o cli

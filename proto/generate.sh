#!/bin/zsh
protoc --cpp_out=. debugger.proto
protoc-c --c_out=. debugger.proto
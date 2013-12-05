#!/bin/sh

num=$(ps aux | grep "$1" |grep -v grep|wc -l)
if [ "$num" -eq 0 ];then
  echo 0
else
  echo 1
fi


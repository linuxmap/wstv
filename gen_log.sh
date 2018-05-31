#!/bin/bash

LOG_FILE=ipc_linux_log.txt
git log --pretty=format:"%ad  %h    %<(12)%an: %s" --date=format:"%Y-%m-%d %H:%M:%S" | grep -v  "Merge branch" > ${LOG_FILE}
unix2dos ${LOG_FILE}

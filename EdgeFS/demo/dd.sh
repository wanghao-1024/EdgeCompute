#!/bin/bash

# 1GB
dd if=/dev/urandom of=filename1.txt bs=1m count=1024

# 1GB + 100M
dd if=/dev/urandom of=filename2.txt bs=1m count=1124

# 1M + 10B 
dd if=/dev/urandom of=filename3.txt bs=1 count=1048586

# 10K + 9B
dd if=/dev/urandom of=filename4.txt bs=1 count=10249

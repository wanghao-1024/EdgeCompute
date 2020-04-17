#!/bin/bash

rm -rf ../data/edgefs*
mkdir -p ../data/

rm -f filename1.txt
rm -f filename2.txt
rm -f filename3.txt
rm -f filename4.txt

# 1GB
dd if=/dev/urandom of=filename1.txt bs=1m count=1024

# 1GB + 100M
dd if=/dev/urandom of=filename2.txt bs=1m count=1124

# 1M + 10B
dd if=/dev/urandom of=filename3.txt bs=1 count=1048586

# 10K + 9B
dd if=/dev/urandom of=filename4.txt bs=1 count=10249


./edgefs_write_main filename1.txt 1073741824 
./edgefs_write_main filename2.txt 1178599424
./edgefs_write_main filename3.txt 1048586
./edgefs_write_main filename4.txt 10249

./edgefs_read_main filename1.txt 1073741824 
./edgefs_read_main filename2.txt 1178599424
./edgefs_read_main filename3.txt 1048586
./edgefs_read_main filename4.txt 10249

md5 *filename*.txt

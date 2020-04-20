#!/bin/bash


rm -rf ../data/edgefs*
mkdir -p ../data/

touchfile()
{
    filename=$1
    bs=$2
    count=$3
    if [ ! -e $filename ];then
        echo "create ${filename}"
        dd if=/dev/urandom of=${filename} bs=${bs} count=${count}
    fi
}

exewrite()
{
    filename=$1
    filesize=$2
    ./edgefs_write_main ${filename} ${filesize}
}

exeread()
{
    filename=$1
    filesize=$2
    ./edgefs_read_main ${filename} ${filesize}
}

#1GB
touchfile filename1.txt 1m 1024
#1GB + 100M
touchfile filename2.txt 1m 1124
#1M + 10B
touchfile filename3.txt 1  1048586
#10K + 9B
touchfile filename4.txt 1  10249



exewrite filename1.txt 1073741824 
exewrite filename2.txt 1178599424
exewrite filename3.txt 1048586
exewrite filename4.txt 10249

exeread filename1.txt 1073741824 
exeread filename2.txt 1178599424
exeread filename3.txt 1048586
exeread filename4.txt 10249

md5 *filename*.txt

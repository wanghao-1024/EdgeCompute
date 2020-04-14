# 适用范围

EdgeFS适用于家用路由器，边缘设备，lot等

如果不考虑服务器的各种优化，仅仅从原理而言，也适合于服务器设备



# 名词解释

chunk : 块，一个块有一个元数据（MetaInfo）

chunkNum： 磁盘被分成块的个数

chunkSize ： 每个chunk块的大小

coverableDiskSize ： 文件系统所覆盖的磁盘大小，diskSize = chunkNum * chunkSize

Index文件 ： 存放文件系统的索引信息

usableMemory ： index文件映射到内存中的大小，等于index文件的大小



# 一期设计

## index 文件格式

![1](/Users/wanghao/worker/mycode/EdgeCompute/EdgeFS/doc/1.png)

EdgeFSHead : 固定42字节，存储文件系统头部的信息

bitmap : 每一个bit表示一个chunk块是否被使用，动态值，根据磁盘大小和内存大小动态变化

MetaPool : 每个chunk块的元数据信息



## 元数据区

![2](/Users/wanghao/worker/mycode/EdgeCompute/EdgeFS/doc/2.png)

当Res1，Res2，Res3的hash key都相同时，则metapool中则出现了上图的结构，采用开链法来解决hash冲突问题。

Res1存在放chunkid为36708和898块中，

Res2存放在4578和10090的块中，

Res3存放在4367的块中



## write 函数

## read 函数

## 统计项

磁盘的利用率，例如chunkSize为1M，则1M+1字节的文件会占用两个chunk，则需要统计线网的磁盘利用率来调整chunkSize，调整chunkSize需要考虑到路由器的内存和磁盘分别。

相比于cdn而言，edgefs更加难，因为cdn都是统计机型（或则某几类）且有存储服务器（大内存，大存储）的，

路由器都是不同内存大小和存储，所以设定chunkSize时考虑的因素更多，磁盘利用率的优化就更难。

index缓存命中率，来衡量增大内存带来的好处。如果三期中的切换subindex消耗不大，则不存在这样的问题。

## 测试

文件系统一旦发生问题，则会发生数据错乱，需要进行压测。多文件写入和大文件写入

## 文件淘汰策略

目前还不清楚



# 二期

此处给出目前现有路由器（玩客云，newwifi）的内存大小和不同存储大小的一个表格，包括EdgeFS使用的内存大小，chunkNum，chunkSize，和按照目前腾讯视频的文件大小，磁盘的最大利用率等

mmap创建是内存在虚拟内存上面

路由器内存比较小，如果很大部分内存用于缓存index文件，但是其实大部分文件都是访问不到的，则会造成内存的浪费。

可以将index文件按照chunkid % idxfilenum 切分为多个subindex文件，每次只缓存一个subindex文件，则可以减少内存访问。

如果访问的文件没有命中缓存的subindex文件时，可以将当前subindex文件换出，换入新的subindex文件。（类似于操作系统的缺页中断）



# 三期

二期的方案，如果多个热点文件分散在不同的subindex文件中，则可能会导致频繁的内存换入换出操作

>  目前还不确定性能消耗有多大，需要先测试在路由器上面close，open十几M的文件，然后mmap映射到内存的时间。如果时间大于用户于路由器之间的rtt（猜测值），可能才有优化意思
>
>  看mmap的实现只是创建映射指针，不拷贝数据

可以改进为后台在分发时携带文件的热度级别（例如分为五级，非常热，热，普通，冷，非常冷）

如果index刚好分为5个subindex，则非常热的文件存放在subindex0中，依次类推。

则可以避免多个热点文件分散在不同subinex中。

具体实现是需要考虑：

写入的时候没有问题，扩展MetaInfo的nextChunkid为nextChunkids[]，数组大小是5，如果当前文件非常热，则链接到nextChunkids[0]的链中

读取的时候，如果当前缓存的是subindex0（就是非常热文件的索引），则在当前nextChunkid[0]中查找，

未找到则依次查找nextChunkid[1]到nextChunkid[4]，如果在nextChunkid[4]中找打了，则加载subindex4。

这样一来，就会增大MetaInfo的大小，需要评估和切换索引开销的比较

需要考虑文件从冷变热，从热变冷的索引的变化。这里只需要移动索引就可以了。



# 四期

如果三期的方案，可以实现那么后台分发策略是什么呢？


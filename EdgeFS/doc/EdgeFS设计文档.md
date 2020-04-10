# 名词解释

chunk : 块，一个块有一个元数据（MetaInfo）

chunkNum： 磁盘被分成块的个数

chunkSize ： 每个chunk块的大小

coverableDiskSize ： 文件系统所覆盖的磁盘大小，diskSize = chunkNum * chunkSize

Index文件 ： 存放文件系统的索引信息

usableMemory ： index文件映射到内存中的大小，等于index文件的大小



# index 文件格式

![1](/Users/wanghao/worker/mycode/EdgeCompute/EdgeFS/doc/1.png)

EdgeFSHead : 固定42字节，存储文件系统头部的信息

bitmap : 每一个bit表示一个chunk块是否被使用，动态值，根据磁盘大小和内存大小动态变化

MetaPool : 每个chunk块的元数据信息



# 元数据区

![2](/Users/wanghao/worker/mycode/EdgeCompute/EdgeFS/doc/2.png)

当Res1，Res2，Res3的hash key都相同时，则metapool中则出现了上图的结构，采用开链法来解决hash冲突问题。

Res1存在放chunkid为36708和898块中，

Res2存放在4578和10090的块中，

Res3存放在4367的块中



# write 函数



# read 函数
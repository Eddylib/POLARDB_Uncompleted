## All you have to do:

Complete engine_race/engine_race.[h,cc], and execute

```
make
```
to build your own engine

## Example for you

For quick start, we have already implemented a simple
example engine in engine_example, you can view it and execute

```
make TARGET_ENGINE=engine_example
```
to build this example engine

## 目前进展

* 实现了hashtable与buffer来支持写缓冲。
* 使用跳表来支持顺序dump。当缓冲表满后dump成库文件。
* hashtable的value指向buffer，这里的线程互斥还未完成。
* 实现了多跳表来支持多线程并发读写，dump时多条表归并dump。
```
互斥一完成，再调调读接口，调调参，跑个三百秒内应该没啥问题。
```
* 其实跳表不太需要， 用优先队列来构建缓冲区即可，对于多线程，构建多个优先队列来同时缓存kv，dump时再用优先队列将多个队列串起来实现更快速的归并dump。这样用标准库会避免很多数据结构上的问题。

* 祭这写了一半的代码Orz
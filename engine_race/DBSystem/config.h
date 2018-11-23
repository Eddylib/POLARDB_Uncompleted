//
// Created by libaoyu on 18-11-11.
//

#ifndef ENGINE_CONFIG_H
#define ENGINE_CONFIG_H

#define HASH_TABLE_SIZE 68000000

#define SKIP_LIST_MAX_HEIGHT 12
#define SKIP_LIST_INIT_CAPACITY 10
#define POSSIBILITY_GEN_UPPER 4

#define SKLILT_MAX_SIZE 100000


#define BUFF_NUM 10
//每条消息4096字节，每个buff块存储4096条消息，即16M，10块缓冲区，需要160M。




#define HOT_LOG "hot_log"
#define DIED_LOG "died_log"
#define MANIFIST_FILE "manifist"
#define SST_FILE_APPENDIX ".sst"
#define MAX_LOG_RECORD_NUM 8192
#define MAX_RECORD_LVL 10
//16M

#define RECORD_MATIC 0xFFC42495
//"LBY"

//跳表个数
#define MEMTABLE_CO_WRITE_NUM 40
//对跳表进行dump的门限， 门限乘以跳表个须小于BUFFSIZE，否则会陷入阻塞。
#define SKIPLIST_DUMP_THERSHOLD 4096

//缓存大小约等于4096*81920
#define RECORD_VALUE_LEN 4096
#define RECORD_BUFF_SIZE 1024



#define SST_KEY_NUM RECORD_BUFF_SIZE

#endif //ENGINE_CONFIG_H

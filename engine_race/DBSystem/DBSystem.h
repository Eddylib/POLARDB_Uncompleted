//
// Created by libaoyu on 18-11-11.
//

#ifndef ENGINE_DBSYSTEM_H
#define ENGINE_DBSYSTEM_H


#include <string>
#include <map>
#include "utils.h"
#include "Index.h"
#include "SkipList.h"
#include "WaitQueue.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string.h>

struct Record{
    uint32_t magic = RECORD_MATIC; //donot know if const if can write
    KEY_T key;
    struct ValueStruct{
        uint8_t value_ptr[RECORD_VALUE_LEN]{0};
        uint32_t check_field;
    };
    ValueStruct value;
    Record():key(0),value({0}){}
    void set_value(uint8_t *data){memcpy((uint8_t*)value.value_ptr,data,RECORD_VALUE_LEN);};
    void get_value(uint8_t *data){memcpy(data,value.value_ptr,RECORD_VALUE_LEN);};
    void make_check();
    bool check_valid();
};

class Manifester{
    std::mutex this_mutex;
    friend class DiskManager;
    struct MFRecord{
        int32_t lvl;//四字节对齐
        uint8_t file_name[256];
        KEY_T kstart;
        KEY_T kend;
        int32_t nrecord;
    };
    std::list<MFRecord> all_records;
    Manifester();
    static Manifester *instance;
    MFRecord currRecord;
    int record_cnt[MAX_LOG_RECORD_NUM] = {0};
public:
    static void init();
    static Manifester *get_instance(){return instance;}
    int get_records_num(){return static_cast<int>(all_records.size());}
    void insert_record(int lvl, KEY_T kstart, KEY_T kend, uint32_t nrecord);
};

class Buffer{
    Record data[RECORD_BUFF_SIZE];
    u_char accessing[RECORD_BUFF_SIZE] = {0};
    std::list<int> idxes_empty;//这个表只会从头取东西，从头放回
    std::list<int> idxes_wrote;//这个表，从头压入，从任意地方取东西（释放缓存）！
    std::map<int,std::list<int>::iterator> wrote_mapping;
    int id;
    //为了实现从任意地方剔出，建立一个int到迭代器的map！

     //对buffer的索引，使得能够快速在buffer中找到数据！
    std::mutex idx_mutex;//此锁仅仅在获取content_idx，或者更新之时上锁，粒度非常细
    std::mutex slot_mutex;
    std::condition_variable slot_waiter;
    std::condition_variable idx_waiter;
public:
    Buffer();
    int request_record_buff();
    void release_buff(int idx);

    Record * wait_for_access(int idx);
    void end_access(int idx);
    Record *get_base(){return data;}

    bool if_full();
    int get_id();
    void set_id(int id);
};
class MemTables{
    WaitQueue<std::vector<SkipList*> *,10> memtables_wait_for_dump; //只存缓存里的索引，如果在文件里，只会在hash表里！
    WaitQueue<SkipList*,5> memtables_can_insert;

    //只有在尝试获取skiplist时才会阻塞等待，只要这个表不大，就很快。
    std::mutex current_memtable_list_mutex;
    std::list<SkipList*> current_memtables; //只会在这里面写！
    std::thread *creater_thread = nullptr;
    std::thread *dumper_thread = nullptr;
    bool if_working = true;
    typedef int FUNC_DUMP_T(std::vector<KeyIdxPair*> &kvidxes);
    static int dump_all(std::vector<KeyIdxPair *> &kvidxes);
public:
    MemTables();
    void buffers_creater();
    void buffers_dumper();
    int get_total_items();

    //当前线程会获取该skiplist的锁，其他的线程无法再访问！
    std::list<SkipList*>::iterator get_memtable();

    void stop_working();
    void start_working();
    //submit只管died log。而died log只有hot log满时才会产生！
    void submit_dump();
    ~MemTables();
};
class DBSystem {
    explicit DBSystem(const std::string &base_path);
    Buffer buffer;
    Indexer hashTable;
    static std::shared_ptr<DBSystem> instance_spt;
    int running = 1;
public:
    static DBSystem* init(const std::string &base_path);

    static DBSystem*get_instance();

    void read(const std::string &key, uint8_t *value);
    void write(const std::string &key,uint8_t *value);
    void write_recover(Record *gather);
    Buffer &get_buffer(){return buffer;}
    Indexer &get_hash_table(){return hashTable;}
    MemTables memTables;

};


#endif //ENGINE_DBSYSTEM_H

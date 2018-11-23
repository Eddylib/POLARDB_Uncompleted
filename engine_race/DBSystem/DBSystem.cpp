//
// Created by libaoyu on 18-11-11.
//

#include "DBSystem.h"
#include "io.h"
#include <sstream>
#include <cstring>
#include <assert.h>
#include <iostream>
#include <fcntl.h>
#include <zconf.h>
#include <sys/mman.h>


using namespace std;

std::shared_ptr<DBSystem> DBSystem::instance_spt;
Manifester *Manifester::instance = nullptr;

DBSystem *DBSystem::init(const std::string &base_path) {
    if(!instance_spt)
        instance_spt = std::shared_ptr<DBSystem>(new DBSystem(base_path));

    DiskManager::init(base_path);
    Manifester::init();
    //处理died log
    DiskManager::get_instance()->init_log_file_and_fetch_data(DiskManager::DIED_LOG_T);

    //处理hot log
    DiskManager::get_instance()->init_log_file_and_fetch_data(DiskManager::HOT_LOG_T);
    return instance_spt.get();
}

DBSystem::DBSystem(const std::string &base_path) {
}

DBSystem *DBSystem::get_instance() {
    assert(instance_spt);
    return instance_spt.get();
}


void DBSystem::write_recover(Record *gather) {
    //不写入到磁盘
//    KEY_T keyT = get_key(key);
//    auto *gather = new Record();
//    gather->key = keyT;
//    gather->set_value(value);
//    gather->make_check();
//    DiskManager::get_instance()->write_to_log(gather,sizeof(Record));

    //申请buff空间，写入到缓存
    int idx_buffer = gather-buffer.get_base();
    assert(idx_buffer < RECORD_BUFF_SIZE);

    //获取memtable，写入到memtable
    auto memtable = memTables.get_memtable();
    Index indexToBuffer;
    indexToBuffer.set_fid(0);
    indexToBuffer.set_idx(idx_buffer);
    indexToBuffer.set_in_file(false);
    (*memtable)->update(gather->key,indexToBuffer);
    (*memtable)->un_lock();

    //写入到hash表。
    hashTable.update(gather->key,indexToBuffer);

}
void DBSystem::write(const std::string &key, uint8_t *value) {
    //立即写入到磁盘
    KEY_T keyT = get_key(key);
    auto *gather = new Record();
    gather->key = keyT;
    gather->set_value(value);
    gather->make_check();
    int ifFUll = DiskManager::get_instance()->write_to_log(gather,sizeof(Record));

    //如果在缓存中，只更新缓存里面的数据！

    //申请buff空间，写入到缓存
    int idx_buffer = buffer.request_record_buff();
    Record *place = buffer.wait_for_access(idx_buffer);
    memcpy(place,gather,sizeof(Record));
    buffer.end_access(idx_buffer);

    //写入到hash表。
    Index indexToBuffer;
    indexToBuffer.set_fid(0);
    indexToBuffer.set_idx(idx_buffer);
    hashTable.update(keyT,indexToBuffer);

    //获取memtable，写入到memtable，以备dump时能够找到buff的地址
    auto memtable = memTables.get_memtable();
    (*memtable)->update(keyT,indexToBuffer);
    (*memtable)->un_lock();

    if(memTables.get_total_items() >= RECORD_BUFF_SIZE){
//        dbcout<<"testing<"<<endl;
        dbcout<<memTables.get_total_items()<<endl;
        DiskManager::get_instance()->switch_log();
        memTables.submit_dump();
    }


    delete(gather);
}
void DBSystem::read(const std::string &key, uint8_t *value) {
    //直接从hashtable里读，只可能有两种情况，一种是在缓存里，一种是在硬盘里。
    KEY_T keyT = get_key(key);
    Index *idx_ptr = nullptr;
    hashTable.get(keyT,&idx_ptr);

    Record *buff = buffer.wait_for_access(idx_ptr->get_idx());
    buff->get_value(value);
    buffer.end_access(idx_ptr->get_idx());
}
void Manifester::insert_record(int lvl, KEY_T kstart, KEY_T kend, uint32_t nrecord) {
    lock_guard<mutex> guard(this_mutex);
    DiskManager *diskManager = DiskManager::get_instance();
    MFRecord newmfR;
    newmfR.lvl = lvl;
    newmfR.kstart = kstart;
    newmfR.kend = kend;
    stringstream sstream;
    sstream<<all_records.size()<<SST_FILE_APPENDIX;
    memcpy(newmfR.file_name,sstream.str().c_str(),sstream.str().length());
    newmfR.nrecord = nrecord;

    IOJob job{};
    for (auto &one_record : all_records) {
        job.job.push_back(iovec{.iov_base = &one_record,.iov_len = sizeof(MFRecord)});
    }
    job.job.push_back(iovec{.iov_base = &newmfR,.iov_len = sizeof(MFRecord)});
    //先写入到磁盘
    diskManager->write_to_manifist(job);
    //再写入到内存
    all_records.push_back(newmfR);
    record_cnt[newmfR.lvl]++;
}

Manifester::Manifester() {
    DiskManager::get_instance()->recover_manifist(all_records);
    for(auto iter = all_records.begin();iter!=all_records.end();iter++){
        record_cnt[iter->lvl]++;
    }
}

void Manifester::init() {
    instance = new Manifester();
}




void Record::make_check() {
    value.check_field = 0;
}

bool Record::check_valid() {
    return true;
}

int Buffer::request_record_buff() {
    unique_lock<mutex> lock(idx_mutex);
    while(idxes_empty.empty()){
        idx_waiter.wait(lock);
    }
    int ret = idxes_empty.back();
    idxes_empty.pop_back();
    idxes_wrote.push_front(ret);
    wrote_mapping.insert(make_pair(ret,idxes_wrote.begin()));
    idx_waiter.notify_one();
    return ret;
}
void Buffer::release_buff(int id) {
    unique_lock<mutex> lock(idx_mutex);
//    while(idxes_wrote.empty()){
//        idx_waiter.wait(lock);
//    }
    auto map_ite = wrote_mapping.find(id);
    assert(map_ite != wrote_mapping.end());
    int idx = *map_ite->second;
    idxes_wrote.erase(map_ite->second);
    wrote_mapping.erase(map_ite);
    idxes_empty.push_front(idx);
    idx_waiter.notify_one();
}
Record * Buffer::wait_for_access(int idx) {
    unique_lock<mutex> lock(slot_mutex);
    while(accessing[idx]){
        slot_waiter.wait(lock);
    }
    accessing[idx] = 1;
    return &data[idx];
}

void Buffer::end_access(int idx) {
    unique_lock<mutex> lock(slot_mutex);
    accessing[idx] = 0;
    slot_waiter.notify_one();
}
bool Buffer::if_full() {
    lock_guard<mutex> guard(idx_mutex);
    return idxes_empty.empty();
}


Buffer::Buffer() {
    cout<<"buffer inited"<<endl;
    for (int i = 0; i < RECORD_BUFF_SIZE; ++i) {
        idxes_empty.push_back(i);
    }
}

int Buffer::get_id() {
    return id;
}

void Buffer::set_id(int _id) {
    id = _id;
}




MemTables::MemTables() {
    start_working();
    for (int i = 0; i < MEMTABLE_CO_WRITE_NUM; ++i) {
        SkipList *memtable_ptr = nullptr;
        memtables_can_insert.wait_and_pop(memtable_ptr);
        assert(memtable_ptr);
        current_memtables.push_front(memtable_ptr);
    }
}

void MemTables::buffers_creater() {
    while(if_working){
        memtables_can_insert.wait_and_push(new SkipList(0));
    }
}

void MemTables::buffers_dumper() {
    while(if_working){
        std::vector<SkipList*> *curr_writing = nullptr;
        //todo write to disk!
        memtables_wait_for_dump.wait_and_pop(curr_writing);
        if(curr_writing){
            int list_total = static_cast<int>(curr_writing->size());
            int list_left = static_cast<int>(curr_writing->size());
            KeyIdxPair *tmp;
            std::vector<KeyIdxPair *> kvs;
            int dump_cnt = 0;
            while(true){
                KEY_T minK = INT64_MAX;
                auto erase = curr_writing->end();
                auto min_iter = curr_writing->end();
                //找到最小的记录
                for (auto iter = curr_writing->begin(); iter != curr_writing->end(); ++iter) {
                    tmp = (*iter)->front_lvl_0();
                    if(tmp){
                        if(tmp->key < minK){
                            minK = tmp->key;
                            min_iter = iter;
                        }
                    } else{
                        //记录不存在，删除此迭代器
                        erase = iter;
                    }
                }
                //如果最小的为结束则推出
                if(min_iter == curr_writing->end()){
                    dbcout<<"stop here"<<endl;
                    break;
                }
                KeyIdxPair *record_writing = (*min_iter)->pop_lvl_0();
                //开始写文件！
                int bfidx = record_writing->idx.get_idx();

                kvs.push_back(record_writing);

                if(erase != curr_writing->end()){
                    SkipList *del = *erase;
                    delete del;
                    curr_writing->erase(erase);
                }
            }
            dbcout<<"break while dumping: "<<kvs.size()<<", records"<<endl;
            dump_all(kvs);
            delete curr_writing;
        }
    }
}

void MemTables::start_working() {
    if_working = true;
    creater_thread = new thread(&MemTables::buffers_creater,this);
    dumper_thread = new thread(&MemTables::buffers_dumper,this);
}

void MemTables::stop_working() {
    if_working = false;
    assert(creater_thread&&dumper_thread);
    SkipList *tmp;
    memtables_can_insert.wait_and_pop(tmp);
    creater_thread->join();
    memtables_wait_for_dump.wait_and_push(nullptr);
    dumper_thread->join();
    delete(creater_thread);
    delete(dumper_thread);
}

std::list<SkipList*>::iterator MemTables::get_memtable() {
    lock_guard<mutex> lock(current_memtable_list_mutex);
    auto iter = current_memtables.begin();
    while (true){
        if((*iter)->try_lock()){
            break;
        }
        iter++;
        if(iter == current_memtables.end()){
            iter = current_memtables.begin();
        }
    }
    return iter;
}

void MemTables::submit_dump() {
    unique_lock<mutex> lock(current_memtable_list_mutex);

    std::vector<SkipList*> *dump_seq = new std::vector<SkipList*>();
    while(!current_memtables.empty()){
        auto iter = current_memtables.begin();
        SkipList *skipList = *iter;
        skipList->set_dumping();
        current_memtables.pop_front();
        dump_seq->push_back(skipList);
    }
    for (int i = 0; i < MEMTABLE_CO_WRITE_NUM; ++i) {
        SkipList *skipList = nullptr;
        memtables_can_insert.wait_and_pop(skipList);
        current_memtables.push_back(skipList);
    }
    lock.unlock();
    //
    memtables_wait_for_dump.wait_and_push(dump_seq);
}

int MemTables::dump_all(std::vector<KeyIdxPair *> &kvidxes) {

    stringstream sstream;
    sstream<<Manifester::get_instance()->get_records_num()<<SST_FILE_APPENDIX;
    DiskManager *diskManager = DiskManager::get_instance();
    Indexer &hashtable = DBSystem::get_instance()->get_hash_table();
    diskManager->dumpint_keys(sstream.str(),kvidxes);
    for (int i = 0; i < kvidxes.size(); ++i) {
        Buffer &buffer = DBSystem::get_instance()->get_buffer();
        Record *record = buffer.wait_for_access(kvidxes[i]->idx.get_idx());
        diskManager->append_dumping(record->value.value_ptr,RECORD_VALUE_LEN);
        buffer.release_buff(kvidxes[i]->idx.get_idx());
        buffer.end_access(kvidxes[i]->idx.get_idx());



        Index index;
        index.set_fid(Manifester::get_instance()->get_records_num());
        index.set_idx(i);
        index.set_in_file(true);
        hashtable.update(kvidxes[i]->key,index);
    }
    std::string filename = sstream.str();
    Manifester::get_instance()->insert_record(0, kvidxes.front()->key, kvidxes.back()->key,
                                              static_cast<uint32_t>(kvidxes.size()));
    DiskManager::get_instance()->end_dumping();
    //仅会删除idle log。而idle log在日志满时由hot log转化而来。
    //初始化时idle和hot分别进行，idle先进行，但是没有将hot log转为idle log的步骤。
    //这个步骤只有在运行时，buff满时才会进行！
    dbcout<<"dump ok"<<endl;
    return 0;
}

int MemTables::get_total_items() {
    lock_guard<mutex> lock(current_memtable_list_mutex);
    int ret = 0;
    for(auto iter = current_memtables.begin();iter != current_memtables.end();iter++){
        ret += (*iter)->cnt_ns();
    }
    dbcout<<ret<<endl;
    return ret;
}

MemTables::~MemTables() {
    dbcout<<"testing mem destory"<<endl;
    stop_working();
}


//
// Created by libaoyu on 18-11-11.
//

#ifndef ENGINE_INDEXHASHMAP_H
#define ENGINE_INDEXHASHMAP_H


#include <mutex>
#include <condition_variable>
#include "utils.h"
#include "config.h"

class Index{
    typedef uint32_t IdxStatus;
    IdxStatus status;
public:
    //     xxxx xxxx xxxx xxxx xxxx     xxxx     xxxxxxxx
    //          fileidx                status   file name idx
    //          sk_data idx                      sk struct idx
    //                                             buff idx
    //             20                     4          8
    //                            ? | is writing? | hashed | dumped
    //读被调用时，数据在buff里，索引在跳表以及hash表中。调表存的是buff索引，hash表存的是调表索引。
    //后台线程会将跳表回收，回收跳表时，将hash表中相应字段索引设置为磁盘索引，跳表被销毁
    explicit Index(int _status){status = 0;set_idx(_status);}
    Index():status(0){}
    inline void clear_fid();
    inline void clear_idx();
    inline int get_fid();
    inline int get_idx() const;
    inline void set_fid(int fid);
    inline void set_idx(int idx);
    inline uint32_t if_valid();
    inline uint32_t if_in_hash();
    inline void set_in_hash(int ifset);
    inline uint32_t if_in_file();
    inline void set_in_file(int ifset);
    inline uint32_t if_writing();
    inline void set_if_writing(int ifset);
    inline int equal(const Index &another);
};


class KeyIdxPair{
public:
    KEY_T key{};
    Index idx;
};
class IndexHashMap {
    KeyIdxPair data[HASH_TABLE_SIZE];
public:
    IndexHashMap();
    int update(KEY_T key, Index index);
    int get(KEY_T key, Index **arg);
};
class Indexer{//弥补leveldb的读缺陷，维护一个map，存着从key到文件信息/内存信息的映射
    //比赛至多64000000次读写，有机会将映射存到内存中
private:
    IndexHashMap *hashMap;
    std::mutex data_mutex;
    std::condition_variable writing_contion;
public:
    Indexer();
    int update(KEY_T key, Index idx);
    int get(KEY_T key, Index **idx);
    void set_if_writing(KEY_T key, bool if_writing);
    bool get_if_writing(KEY_T key);
    ~Indexer();
};


inline int Index::get_fid() {

    return status&0x000000ff;
}

inline int Index::get_idx() const {
    return (status&0xfffff000)>>12;
}

inline void Index::set_fid(int fid) {
    clear_fid();
    status|=fid;
}

inline void Index::set_idx(int idx) {
    clear_idx();
    status |= idx<<12;
}

inline void Index::clear_fid() {
    status&=0xffffff00;
}

inline void Index::clear_idx() {
    status&=0xc0000fff;
}

inline uint32_t Index::if_valid() {
    return status & 0x00000f00;
}

inline uint32_t Index::if_in_hash() {
    return status & 0x00000200;
}

inline void Index::set_in_hash(int ifset) {
    if(ifset){
        status |= 0x00000200;
    }else{
        status &= 0xfffffcff;
    }

}
inline uint32_t Index::if_in_file() {
    return status & 0x00000100;
}

inline void Index::set_in_file(int ifset) {
    if(ifset){
        status |= 0x00000100;
    }else{
        status &= 0xfffffeff;
    }
}

inline int Index::equal(const Index &another) {
    return get_idx() == another.get_idx();
}

uint32_t Index::if_writing() {
    return status & 0x00000400;
}

void Index::set_if_writing(int ifset) {
    if(ifset){
        status |= 0x00000400;
    }else{
        status &= 0xfffffaff;
    }

}

#endif //ENGINE_INDEXHASHMAP_H

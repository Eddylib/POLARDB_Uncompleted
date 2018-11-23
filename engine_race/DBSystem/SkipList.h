//
// Created by libaoyu on 18-11-11.
//

#ifndef ENGINE_MEMTABEL_H
#define ENGINE_MEMTABEL_H

#include <list>
#include "Index.h"
#include <vector>
#include <stack>
#include <queue>
#include <mutex>

struct SkipListNode{
    KeyIdxPair data;
    uint32_t next;
//    uint32_t upper;
    uint32_t bottom;
    SkipListNode():next(0),bottom(0)/*,upper(0)*/{}
};
class SkipList {
    std::mutex data_mutex;
    std::mutex dumping_mutex;
    std::vector<std::vector<SkipListNode>> sk_data;
    std::vector<int> sk_capacity;
    int id;
    bool dumping = false;
    //-----------------------------------
    //head指向头一个节点，头一个节点不存数据，也不会被删除，在头一个节点的后一个节点来进行存取
    int check_and_resize(int level);
    int insert_after(KEY_T key, Index index, int level, int idx);
    int get_insert_height();
public:
    std::vector<int> &get_capacities(){return sk_capacity;}
    std::vector<std::vector<SkipListNode>> &get_data(){return sk_data;}
    std::vector<SkipListNode> &get_data_0(){return sk_data[0];}
    explicit SkipList(int _id);
    int update(KEY_T key, Index index);
    int get(KEY_T key, Index *arg, std::queue<int> *trace = nullptr) const;
    Index direct_get(int lvl0_idx)const;
    int cnt_ns();//thread not safe
    int get_heighest_level()const;
    int get_id()const{return id;}
    bool try_lock();
    void set_dumping();
    void un_lock(){data_mutex.unlock();}
    KeyIdxPair *pop_lvl_0();
    KeyIdxPair *front_lvl_0();
};


#endif //ENGINE_MEMTABEL_H

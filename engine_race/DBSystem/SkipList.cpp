//
// Created by libaoyu on 18-11-11.
//

#include <assert.h>
#include <iostream>
#include <queue>
#include "SkipList.h"
using namespace std;
//int sizes[] = {1000000,};
SkipList::SkipList(int _id):id(_id) {
    sk_data.resize(SKIP_LIST_MAX_HEIGHT);
    sk_capacity.resize(SKIP_LIST_MAX_HEIGHT);
    for (int i = 0; i < SKIP_LIST_MAX_HEIGHT; ++i) {
        sk_data[i].resize(SKIP_LIST_INIT_CAPACITY);
        sk_capacity[i] = static_cast<int>(sk_data[i].size()) - 1;
        //head指向头一个节点，头一个节点不存数据，也不会被删除，在头一个节点的后一个节点来进行存取
        //skdata为一个vector，里面维护着两张链表，data表，空表
        //其中第一个节点的next域为data表的表头，bottom域为空表的表头
        //插入数据时从空表拿出一个元素，放到data表里即可
        //如果满了调用resize，将分配出来的内存块链到链表里
        uint32_t  empty_prefix= 1;
        for (int j = 0; j < sk_capacity[i]; ++j) {
            uint32_t node_adding_to_empty = j+empty_prefix;
            sk_data[i][node_adding_to_empty].next = sk_data[i][0].bottom;
            sk_data[i][0].bottom = node_adding_to_empty;
        }
    }
}


int SkipList::update(KEY_T key, Index index) {
    Index tmp;
    std::queue<int> trace;
    int idx = get(key, &tmp, &trace);
    if(idx > 0){
        //当前树中有此key的索引，很nice
        sk_data[0][idx].data.idx.set_idx(index.get_idx());
        sk_data[0][idx].data.idx.set_fid(id);
    }else{
        int lvl0_dis = 0;
        // 没有在当前树找到，所以要插入节点到当前字数
        // 每一层的插入点都已存在trace中
        int insert_lvl = get_insert_height()-1;
//        cout<<"insert_lvl  lvl:"<<insert_lvl<<endl;
        int prev_insert_place = 0;
        while(insert_lvl>=0){
            int insert_after_point = 0;
            if(trace.size() <= insert_lvl){
                insert_after_point = 0;
            } else{
                insert_after_point = trace.front();
                trace.pop();
            }
            if(trace.size() > insert_lvl){
                continue;
            }
            //insert to list
            int insert_place =  insert_after(key, index, insert_lvl, insert_after_point);

            //建立上下链表！
            if(prev_insert_place){
                sk_data[insert_lvl+1][prev_insert_place].bottom = static_cast<uint32_t>(insert_place);
            }
            idx = insert_place;
            prev_insert_place = insert_place;
            insert_lvl--;
        }

    }
    return idx;
}

int SkipList::get(KEY_T key, Index *arg, std::queue<int> *trace)const {
    int ret = -1;
    int search_cnt = 0;
    //hash缓存中没有成功找到，需要重头开始找咯
    int heighestLevel = get_heighest_level();
    //第一次空的level的下一层既是最高层
    for (int lvl = heighestLevel,curridx = 0; lvl >= 0; --lvl) {
        const vector<SkipListNode> &sklist_level = sk_data[lvl];
//        cout<<sklist_level[curridx].next<<",";
//        if(sklist_level[curridx].next){
//            cout<<sklist_level[sklist_level[curridx].next].data.key<<","<<key;
//        }
//        cout<<endl;
        while (sklist_level[curridx].next && sklist_level[sklist_level[curridx].next].data.key <= key){
            curridx = sklist_level[curridx].next;
            search_cnt++;
        }
        if((curridx||key)&&sklist_level[curridx].data.key == key){
            //如果找到，继续压入剩余的节点
            while(lvl){
                if(trace)
                    trace->push(curridx);
                curridx = sk_data[lvl][curridx].bottom;
                lvl--;
            }
            ret = curridx;
            *arg = sk_data[0][ret].data.idx;
            break;
        }
        //如果没找到，那么压入最后一个小于当前节点的节点索引
        if(trace)
            trace->push(curridx);
        if(curridx)//如果还是在头结点，不应该再指向头结点的bottom，头结点的bottom是空表表头！
            curridx = sklist_level[curridx].bottom;
        search_cnt++;
    }
    return ret;
}

int SkipList::check_and_resize(int level) {
    //如果空表没空间了，resize，将新内容加入到链表里
    if(sk_capacity[level] <= 0){
        std::vector<SkipListNode> &list_level = sk_data[level];
        size_t pre_size = list_level.size();
        list_level.resize(pre_size*2);
        sk_capacity[level] = static_cast<int>(list_level.size() - pre_size);
//        cout<<"update level: "<<level<<", ";
//        for (int j = 0; j < sk_capacity.size(); ++j) {
//            cout<<"("<<sk_capacity[j]<<","<<sk_data[j].size()<<"),";
//        }
//        cout<<endl;
        for (int i = 0; i < pre_size; ++i) {
            uint32_t node_adding_to_empty = static_cast<uint32_t>(i + pre_size);
            sk_data[level][node_adding_to_empty].next = sk_data[level][0].bottom;
            sk_data[level][0].bottom = node_adding_to_empty;
        }

    }
    return 0;
}

int SkipList::get_insert_height() {
    int k = 1;
    while (!(random()%POSSIBILITY_GEN_UPPER) && k<SKIP_LIST_MAX_HEIGHT){
        k++;
    }
    return k;
}

int SkipList::cnt_ns() {
    lock_guard<mutex> lock(data_mutex);
    return static_cast<int>(sk_data[0].size() - sk_capacity[0]) - 1;
}

int SkipList::get_heighest_level() const{
    int ret = 0;
    //确定最高
    for (ret = static_cast<int>(sk_data.size())-1; ret > 0 ; --ret) {
        if(sk_data[ret][0].next) break;
    }
    return ret;
}

int SkipList::insert_after(KEY_T key, Index index, int level, int insert_node_idx) {
    std::vector<SkipListNode> &list_level = sk_data[level];
    assert(sk_capacity[level] && "no capacity!");
    int empyt_node_idx = list_level[0].bottom;
    list_level[0].bottom = list_level[empyt_node_idx].next;

    list_level[empyt_node_idx].next = list_level[insert_node_idx].next;
    list_level[insert_node_idx].next = static_cast<uint32_t>(empyt_node_idx);

    list_level[empyt_node_idx].data.key = key;
    list_level[empyt_node_idx].data.idx = index;

    sk_capacity[level]--;
    check_and_resize(level);
    return empyt_node_idx;
}

Index SkipList::direct_get(int lvl0_idx)const {
    return sk_data[0][lvl0_idx].data.idx;
}

bool SkipList::try_lock() {
    bool ret = data_mutex.try_lock();
    if(ret){
        unique_lock<mutex> dumping_lock(dumping_mutex);
        ret = ret && !dumping;
    }
    return ret;
}

void SkipList::set_dumping() {
    unique_lock<mutex> dumping_lock(dumping_mutex);
    dumping = true;
}

KeyIdxPair *SkipList::pop_lvl_0() {
    auto &data = sk_data[0];
    KeyIdxPair *ret = nullptr;
    if(data[0].next){
        uint32_t pop_idx = data[0].next;
        data[0].next = data[pop_idx].next;

        data[pop_idx].next = data[0].bottom;
        data[0].bottom = pop_idx;
        ret = &data[pop_idx].data;
    }
    return ret;
}

KeyIdxPair *SkipList::front_lvl_0() {
    auto &data = sk_data[0];
    KeyIdxPair *ret = nullptr;
    if(data[0].next){
        ret = &data[data[0].next].data;
    }
    return ret;
}


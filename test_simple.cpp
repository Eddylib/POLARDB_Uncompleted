//
// Created by libaoyu on 18-11-11.
//

//#include <engine_race.h>
#include <iostream>
#include <zconf.h>
#include <assert.h>
#include <DBSystem/SkipList.h>
#include <fstream>
#include <fcntl.h>
#include <DBSystem/io.h>
#include <sys/mman.h>
#include <DBSystem/DBSystem.h>

using namespace std;
void test_hash_index(){

    Indexer indexer;
    int64_t search_cht = 0;
    Index *tmp;
    for (int i = 0; i < 6400*10000; ++i) {
        Index inserted(i);
        search_cht += indexer.update(KEY_T(i),inserted);
        if(i%(10000*100) == 0){
            cout<<i<<endl;
        }
        indexer.get(KEY_T(i),&tmp);
        assert(inserted.equal(*tmp));
    }
    cout<<"insert ok"<<endl;
    cout<<"total: "<<search_cht<<" conflicts"<<endl<<flush;
}
void test_skip_list(){
    SkipList skipList(0);
    for (int i = 0; i < 10000*100; ++i) {
        Index inserted(i);
//        cout<<skipList.cnt_ns()<<","<<i<<endl;
//        if(i%(10000*100) == 0){
//            cout<<i<<endl;
//        }
//        if(i%1000 == 0){
//            cout<<"test"<<endl;
//        }
        skipList.update(KEY_T(i), inserted);

//        Index geted;
//        auto *data = skipList.get(KEY_T(i),&geted,nullptr);

//        assert(inserted.equal(geted));
    }
//    for (int i = 0; i < 10000; ++i) {
//        Index inserted(i);
////        cout<<skipList.cnt()<<","<<i<<endl;
//        if(i%(10000*100) == 0){
//            cout<<i<<endl;
//        }
//        Index geted;
//        auto databefore = skipList.get(KEY_T(i),&geted,nullptr);
//        int before = geted.get_idx();
//        skipList.update(KEY_T(i),inserted,nullptr);
//        auto *data = skipList.get(KEY_T(i),&geted,nullptr);
//        int after = geted.get_idx();
//        cout<<before<<","<<after<<endl;
//        assert(inserted.equal(geted));
//    }
}
void test_skiplist_indexer(){

    SkipList skipList(0);
    cout<<"begin insert"<<endl;
    int times = 10000*1000;
    for (int i = 0; i < times; ++i) {
        Index inserted(i);
        skipList.update(KEY_T(i), inserted);
    }
    cout<<"insert ok"<<endl;
    for (int i = 0; i < times; ++i) {
        Index check(i);
        Index fetched;
        skipList.get(KEY_T(i), &fetched);
        assert(check.equal(fetched));
    }

}
void test_write(){
    int fd = open("test.txt",O_CREAT|O_TRUNC|O_WRONLY);

    write(fd,"len",3);
    while(1){
    }
}
void test_disk_manager(){
    DiskManager::init(".");
    DiskManager *diskManager = DiskManager::get_instance();

}
void test_list(){
    list<int> a;
    a.push_back(1);
    a.push_back(2);
    auto begin = a.begin();
    a.push_front(3);
    cout<<(begin == a.begin())<<endl;
    cout<<*begin<<endl;
    cout<<*a.begin()<<endl;
    a.pop_front();
    cout<<(begin == a.begin())<<endl;
}

void test_database(){
    DBSystem::init("./db_dir");
//    DBSystem::get_instance()->write("libaoy",(uint8_t *)"what funck3");
    uint8_t buff[4096];
    DBSystem::get_instance()->read("libaoy",buff);
    cout<<(char*)buff<<endl;
//    int times = 100;
//    for (int i = 0; i < times; ++i) {
//        DBSystem::get_instance()->write(std::string())
//    }
}
int main(){
    test_database();
}
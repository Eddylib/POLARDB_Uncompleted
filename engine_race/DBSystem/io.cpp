//
// Created by libaoyu on 18-11-12.
//

#include <zconf.h>
#include <fcntl.h>
#include <assert.h>
#include "io.h"
#include "config.h"
#include "utils.h"
#include "DBSystem.h"
#include <iostream>

const static std::string curr_log_name=std::string(HOT_LOG);
const static std::string idle_log_name=std::string(DIED_LOG);
const static std::string manifist_file_name=std::string(MANIFIST_FILE);
using namespace std;
DiskManager *DiskManager::instance = nullptr;

void DiskManager::seq_worker() {
    IOJob tmp{};
    while(!if_stop){
    }
}

DiskManager::DiskManager() {
}

void DiskManager::submit_job(const IOJob &job) {
}

DiskManager *DiskManager::get_instance() {
    return instance;
}

int DiskManager::write_to_log(void *buff, int len) {
    std::lock_guard<std::mutex> lock(diskManagerMutex);
    assert(curr_log_fd>=0);
    curr_log_len+=len;
    write(curr_log_fd,buff,len);
    cout<<__FUNCTION__<<","<<curr_log_len<<endl;
    return (curr_log_len/sizeof(Record))>=MAX_LOG_RECORD_NUM;
}


void DiskManager::init(const std::string &basepath) {
    instance = new DiskManager();
    instance->basepath = basepath;

}

void DiskManager::write_to_manifist(const IOJob &job) {
    std::string filename = path_join({basepath,manifist_file_name});
    manifest_fd = open(filename.c_str(),O_CREAT|O_TRUNC|O_WRONLY);
    writev(manifest_fd, &job.job[0], static_cast<int>(job.job.size()));
    close(manifest_fd);
}

void DiskManager::init_log_file_and_fetch_data(DiskManager::LogType type) {
    int *fd_to_open;
    int *log_len;
    const std::string *filename = nullptr;
    switch(type){
        case HOT_LOG_T:
            fd_to_open = &curr_log_fd;
            filename = &curr_log_name;
            log_len = &curr_log_len;
            break;
        case DIED_LOG_T:
            fd_to_open = &idle_log_fd;
            filename = &idle_log_name;
            log_len = &idle_log_len;
            break;
        default:
            assert(false);
    }


    //先只读打开，获取所有数据，之后再关闭，以append写打开
    std::string logname = path_join({basepath,*filename});
    if(if_file_exists(logname)){
        *fd_to_open = open(logname.c_str(),O_RDONLY);
        assert(*fd_to_open >= 0);
        //直接读入所有数据进内存
        int idx = DBSystem::get_instance()->get_buffer().request_record_buff();
        Record *buffer = DBSystem::get_instance()->get_buffer().wait_for_access(idx);
        int tmp = 0;
        while((tmp = static_cast<int>(read(*fd_to_open, buffer, sizeof(Record)))) != 0){
            assert(tmp == sizeof(Record));
            DBSystem::get_instance()->get_buffer().end_access(idx);
            DBSystem::get_instance()->write_recover(buffer);
            if(DBSystem::get_instance()->get_buffer().if_full()){
                if(type == HOT_LOG_T){
                    DiskManager::switch_log();
                }
                DBSystem::get_instance()->memTables.submit_dump();
            }
            idx = DBSystem::get_instance()->get_buffer().request_record_buff();
            buffer = DBSystem::get_instance()->get_buffer().wait_for_access(idx);
        }
        DBSystem::get_instance()->get_buffer().end_access(idx);
        DBSystem::get_instance()->get_buffer().release_buff(idx);
        close(*fd_to_open);
    }
    //只有host log才以append写打开，以便以后继续
    if(type == HOT_LOG_T){
        *fd_to_open = open(logname.c_str(),O_WRONLY|O_APPEND|O_CREAT,S_IRWXU|S_IRWXG);
        assert(*fd_to_open >= 0);
    }

}

int DiskManager::recover_manifist(std::list<Manifester::MFRecord> &all_records) {
    std::string filename = path_join({basepath,manifist_file_name});
    if(if_file_exists(filename)){
        int fd = open(filename.c_str(),O_RDONLY);
        assert(fd>=0);
        Manifester::MFRecord tmp{};
        while(read(fd,&tmp,sizeof(Manifester::MFRecord))){
            all_records.push_back(tmp);
        }
    }
    return 0;
}

void DiskManager::append_dumping(void *buff, int len) {
    write(dumping_sstable_file_fd, buff, static_cast<size_t>(len));
}

void DiskManager::end_dumping() {
    std::unique_lock<std::mutex> lock(diskManagerMutex);
    //关闭sstable文件的写过程
    close(dumping_sstable_file_fd);
    dumping_sstable_file_fd = -1;
    //删除idle log
    std::string idle_full = path_join({basepath,idle_log_name});
    if(if_file_exists(idle_full)){
        remove(idle_full.c_str());
    }
    wait_switch_log.notify_one();
    dbcout<<"finish<"<<endl;
}

int DiskManager::dumpint_keys(const std::string &filename, std::vector<KeyIdxPair *> &kvs) {

    dumping_sstable_file_fd = open(path_join({basepath,filename}).c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_APPEND);
    uint32_t nrecords = kvs.size();
    write(dumping_sstable_file_fd,&nrecords,sizeof(uint32_t));
    for (int i = 0; i < kvs.size(); ++i) {
        write(dumping_sstable_file_fd, &kvs[i]->key, sizeof(kvs[i]->key));
    }
    return 0;
}

int DiskManager::switch_log() {
    std::unique_lock<std::mutex> lock(diskManagerMutex);
    close(curr_log_fd);
    std::string old = path_join({basepath,curr_log_name});
    std::string newl = path_join({basepath,idle_log_name});
    while (if_file_exists(newl)){
        wait_switch_log.wait(lock);
    }
    rename(old.c_str(),newl.c_str());

    curr_log_fd = open(old.c_str(),O_WRONLY|O_APPEND|O_CREAT,S_IRWXU|S_IRWXG);
    return 0;
}



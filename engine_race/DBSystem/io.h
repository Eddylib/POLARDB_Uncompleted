//
// Created by libaoyu on 18-11-12.
//

#ifndef ENGINE_IO_H
#define ENGINE_IO_H

#include <string>
#include <vector>
#include <sys/uio.h>
#include "DBSystem.h"
struct IOJob{
    //系统只有两种写的方式，一种是append，一种是直接写，append就不管了因为必须立马写好。
    //直接写是内存跳表dump到磁盘后的产物
    std::string file_name;
    std::vector<iovec> job;

};
class DiskManager{
    DiskManager();
    bool if_stop;
    static DiskManager *instance;
    int curr_log_fd = -1;
    int curr_log_len = 0;
    int idle_log_fd = -1;
    int idle_log_len = -1;
    int manifest_fd = -1;

    int dumping_sstable_file_fd = -1;
    int curr_log_record_num = 0;
    std::string basepath;
    std::mutex diskManagerMutex;
    std::condition_variable wait_switch_log;
public:
    enum LogType{HOT_LOG_T,DIED_LOG_T };
    static DiskManager*get_instance();
    static void init(const std::string &basepath);
    void init_log_file_and_fetch_data(DiskManager::LogType type);
    void seq_worker();
    void submit_job(const IOJob &job);
    int write_to_log(void *buff, int len);
    void write_to_manifist(const IOJob &job);
    int recover_manifist(std::list<Manifester::MFRecord> &all_records);
    int switch_log();


    //dumping record
    bool candump = true;
    void append_dumping(void *buff, int len);
    void end_dumping();
    int dumpint_keys(const std::string &filename, std::vector<KeyIdxPair *> &kvs);
};



#endif //ENGINE_IO_H

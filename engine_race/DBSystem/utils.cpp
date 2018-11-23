//
// Created by libaoyu on 18-11-11.
//

#include <fcntl.h>
#include <zconf.h>
#include "utils.h"
#include <sys/stat.h>
#include <assert.h>

bool if_directory_exists(const std::string &path){
    return false;
}
bool if_regular_exists(const std::string &path){
    return false;
}
bool if_file_exists(const std::string &path){
    return access(path.c_str(),F_OK) == 0;;
}

int get_file_size(const std::string &path){
    struct stat statbuff{};
    assert(stat(path.c_str(), &statbuff) >= 0);
    return static_cast<int>(statbuff.st_size);
}
KEY_T get_key(const char *data){
    KEY_T ret=0;
    for (int i = 0; i < 8; ++i) {
        ret |= ((KEY_T)data[i])<<(7-i)*8;
    }
    return ret;
}
KEY_T get_key(const std::string &path){
    return get_key(path.data());
}
std::string path_join(std::initializer_list<std::string> strlist) {
    using namespace std;
    stringstream ret_path;
    auto iter = strlist.begin();
    for(int i = 0; i < strlist.size();i++,iter++){
        auto &item = *iter;
        ret_path<<item;
        if(item[item.length()-1] != '/'&&i<strlist.size()-1){
            ret_path<<'/';
        }
    }
    return ret_path.str();
}

std::string itoa(int64_t inter){
    char tmp[9] = {0};
    sprintf(tmp,"%08ld",inter);
    return std::string(tmp);
}
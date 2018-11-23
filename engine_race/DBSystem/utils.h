//
// Created by libaoyu on 18-11-11.
//

#ifndef ENGINE_UTILS_H
#define ENGINE_UTILS_H

#include <string>
#include <sstream>

typedef uint64_t KEY_T;
#define dbcout cout<<__FUNCTION__<<","
bool if_directory_exists(const std::string &path);
bool if_regular_exists(const std::string &path);
bool if_file_exists(const std::string &path);
int get_file_size(const std::string &path);

std::string path_join(std::initializer_list<std::string> strlist);


KEY_T get_key(const char *key);
KEY_T get_key(const std::string &path);
std::string itoa(int64_t inter);

#endif //ENGINE_UTILS_H

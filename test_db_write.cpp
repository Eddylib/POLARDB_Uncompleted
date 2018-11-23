//
// Created by libaoyu on 18-11-16.
//

#include <DBSystem/DBSystem.h>
#include <iostream>
using namespace std;
void test_database(){
    DBSystem::init("./db_dir");
    DBSystem *dbSystem = DBSystem::get_instance();
    for (int i = 0; i < 9999; ++i) {
        std::string tmp = itoa(i);
        cout<<__FUNCTION__<<","<<i<<endl;
        dbSystem->write(tmp,(uint8_t *)"testing");
    }
}
int main(){
    test_database();
}
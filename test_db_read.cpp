//
// Created by libaoyu on 18-11-16.
//

#include <DBSystem/DBSystem.h>
#include <iostream>
using namespace std;
void test_database(){

    DBSystem::init("./db_dir");
    DBSystem *dbSystem = DBSystem::get_instance();
    dbSystem->write("libaoy",(uint8_t *)"what funck3");
    uint8_t bf[4096];
    for (int i = 0; i < 1; ++i) {
        std::string tmp = itoa(i);
        dbSystem->read(tmp,(uint8_t *)bf);
        cout<<tmp<<", "<<bf<<endl;
    }
}
int main(){
    cout<<itoa(1026)<<endl;
    test_database();
}
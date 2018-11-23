//
// Created by libaoyu on 18-11-13.
//

#ifndef ENGINE_WAITQUEUE_H
#define ENGINE_WAITQUEUE_H

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

template <class T, int capacity = 10>
class WaitQueue{
public:
    bool empty(){
        std::lock_guard<std::mutex> lk(data_mut);
        return data_queue.empty();
    }
    size_t size(){
        std::lock_guard<std::mutex> lk(data_mut);
        return data_queue.size();
    }
    bool try_pop(T& value){
        std::lock_guard<std::mutex> lk(data_mut);
        if(data_queue.empty()){
            return false;
        }
        value = std::move(*data_queue.front());
        data_queue.pop();
        return true;
    }
    void wait_and_pop(T& value){
        std::unique_lock<std::mutex> lk(data_mut);
        cnd_if_not_empty.wait(lk,[this]{ return !data_queue.empty();});
        value = std::move(*data_queue.front());
        data_queue.pop();
        cnd_if_not_full.notify_one();
    }
    bool try_push(T &&new_value){
        std::unique_lock<std::mutex> lk(data_mut);
        if(data_queue.size() >= capacity){
            return false;
        }
        std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
        data_queue.push(data);
        cnd_if_not_empty.notify_one();
        return true;
    }
    void wait_and_push(T &&new_value) {
        std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
        std::unique_lock<std::mutex> lk(data_mut);
        if(data_queue.size() >= capacity){
            cnd_if_not_full.wait(lk,[this]{return data_queue.size() < capacity;});
        }
        data_queue.push(data);
        cnd_if_not_empty.notify_one();
    }
    void wait_and_push(const T& new_value) {
        std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
        std::unique_lock<std::mutex> lk(data_mut);
        if(data_queue.size() >= capacity){
            cnd_if_not_full.wait(lk,[this]{return data_queue.size() < capacity;});
        }
        data_queue.push(data);
        cnd_if_not_empty.notify_one();
    }
    void wait_and_push(T &&new_value,int idx) {
        std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
        std::unique_lock<std::mutex> lk(data_mut);
        if(data_queue.size() >= capacity){
            cnd_if_not_full.wait(lk,[this]{return data_queue.size() < capacity;});
        }
        data_queue.push(data);
        cnd_if_not_empty.notify_one();
    }
private:
    std::queue<std::shared_ptr<T>> data_queue;
    std::mutex data_mut;
    std::condition_variable cnd_if_not_full;
    std::condition_variable cnd_if_not_empty;
};
#endif //ENGINE_WAITQUEUE_H

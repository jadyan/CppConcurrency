/*
 * @Author: jadyan
 * @Date: 2021-01-27 15:13:25
 * @LastEditTime: 2021-01-28 09:05:29
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /jadyan/code/book/CppConcurrency/read/03_sharedData/3.7.cpp
 */




//  3.7.cpp 避免死锁的进阶指导
//  1. 避免嵌套锁
//  2. 避免在持有锁时调用用户提供的代码
//  3. 使用固定顺序获取锁
//  4. 使用锁的层次结构

//  使用层次锁来避免死锁

#include <iostream>
#include <mutex>
#include <unistd.h>

using  namespace std;

class hierarchical_mutex {
public:
    explicit hierarchical_mutex(unsigned level) {}
    void lock() {}
    void unlock() {}
};

hierarchical_mutex high_level_mutex(1000);
hierarchical_mutex low_level_mutex(5000);

int do_low_level_stuff() {
    return 42;
}

int low_level_func() {
    cout<<"底层加锁"<<endl;
    std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
    return do_low_level_stuff();
}

void high_level_stuff(int some_param) {
    
}

void high_level_func() {
    cout<<"高层加锁"<<endl;
    std::lock_guard<hierarchical_mutex> lk(high_level_mutex); // 让 high_level_mutex 上锁，层次为10000
    high_level_stuff(low_level_func()); // 给低层次5000的上锁
}

void thread_a() {
    high_level_func();
}   // 遵守规则，运行成功

hierarchical_mutex other_mutex(100);
void do_other_stuff() {
    cout<<"other加锁"<<endl;
    std::lock_guard<hierarchical_mutex> lk(other_mutex); //other加锁
}

void other_stuff() {
    high_level_func();
    do_other_stuff();
}

void thread_b() {
#if 0
    std::lock_guard<hierarchical_mutex> lk(other_mutex); // 锁住了ohter_mutex，层级只有100
    other_stuff(); // 想要调用10000层级的，违反层级结构
    // 无视规则，运行失败
#endif 
    other_stuff();
}   


int main() {
    thread_a();
    cout<<"XXXXXXXX"<<endl;
    thread_b();
    return 0;
}

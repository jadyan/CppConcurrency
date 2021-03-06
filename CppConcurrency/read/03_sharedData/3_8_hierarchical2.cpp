/*
 * @Author: your name
 * @Date: 2021-01-22 17:59:58
 * @LastEditTime: 2021-01-28 09:55:26
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /jadyan/code/book/CppConcurrency/read/3.8.cpp
 */
//
//  3.8.cpp 避免死锁的进阶指导
//  1. 避免嵌套锁
//  2. 避免在持有锁时调用用户提供的代码
//  3. 使用固定顺序获取锁
//  4. 使用锁的层次结构
//https://www.cnblogs.com/albizzia/p/8595843.html
//  Cpp-Concurrency
//  简单的层级互斥量的实现

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <limits.h>

using namespace std;

class hierarchical_mutex {
    std::mutex internal_mutex;
    unsigned long const hierarchy_value;
    unsigned long previous_hierarchy_value;
    static thread_local unsigned long this_thread_hierarchy_value; // 使用thread_local 的值来表示当前线程的层级值
                                                                   // 因为声明中有thread_local 所以每个线程都有其拷贝副本，线程变量状态完全独立
    void check_for_hierarchy_violation() {
       // cout<<"hierarchy_value = "<<hierarchy_value<< "\tthis_thread_hierarchy_value = "<<this_thread_hierarchy_value<<endl;
       // cout<<"\t****this_thread_hierarchy_value = "<<this_thread_hierarchy_value<<endl;
        if (this_thread_hierarchy_value <= hierarchy_value) { // 检查层级
            throw std::logic_error("违反互斥锁层级");
        }
    }
    void update_hierarchy_value() {
        //cout<<"this_thread_hierarchy_value = "<<this_thread_hierarchy_value<<endl;
        previous_hierarchy_value = this_thread_hierarchy_value; // 持有inernal_mutex
        this_thread_hierarchy_value = hierarchy_value;
        //cout<<"hierarchy_value = "<<hierarchy_value<< "\tthis_thread_hierarchy_value = "<<this_thread_hierarchy_value<<endl;
    }
public:
    explicit hierarchical_mutex(unsigned long value) : hierarchy_value(value), previous_hierarchy_value(0) 
    {
        cout<<"构造函数"<<endl;
    }
    void lock() {
        check_for_hierarchy_violation();
        internal_mutex.lock(); // 内部已经锁住
        update_hierarchy_value(); // 更新层级值
    }
    void unlock() {
        this_thread_hierarchy_value = previous_hierarchy_value; // 对层级值进行保存
        internal_mutex.unlock();
    }
    // 类似lock
    bool try_lock() {
        check_for_hierarchy_violation();
        if (!internal_mutex.try_lock())
            return false;
        update_hierarchy_value();
        return true;
    }
    void show()
    {
        cout<<"hierarchy_value = "<<hierarchy_value
            << "\tprevious_hierarchy_value= "<<previous_hierarchy_value
            << "\tthis_thread_hierarchy_value = "<<this_thread_hierarchy_value
            <<endl;
    }
};

// 初始化层级为最大值，所以最初所有线程都能被锁住
thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);

int main() {
    cout<<"ULONG_MAX="<<ULONG_MAX<<endl;
        hierarchical_mutex m2(2000);
    hierarchical_mutex m1(10000);

    hierarchical_mutex m3(1000);
    hierarchical_mutex m4(100);


    m1.lock();
    m2.lock();
    m3.lock();
    m4.lock();

    m1.show();
    m2.show();
    m3.show();
    m4.show();

    m1.unlock();
    m2.unlock();
    m3.unlock();
    m4.unlock();

}

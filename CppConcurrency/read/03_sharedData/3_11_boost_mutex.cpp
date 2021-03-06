/*
 * @Author: your name
 * @Date: 2021-01-26 09:10:16
 * @LastEditTime: 2021-01-28 15:07:58
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /jadyan/code/book/CppConcurrency/read/3.11.cpp
 */
//
//  3.11.cpp 保护很少更新的数据结构
//  Cpp-Concurrency

//  一个写者，多个读者的互斥量
//  使用 boost::shared_mutex 对数据结构进行保护->简单的DNS缓存

#include <iostream>
#include <thread>
#include <map>
#include <string>
#include <mutex>
#include <boost/thread/shared_mutex.hpp>
using namespace std;



#define READLOCK(mtx)  boost::shared_lock<boost::shared_mutex> lck(mtx);

class dns_entry {


};

class dns_cache {
    std::map<std::string, dns_entry> entries;
    mutable boost::shared_mutex entry_mutex;
public:
    dns_entry find_entry(std::string const& domain) const {
        //boost::shared_lock<boost::shared_mutex> lk(entry_mutex); // 保护共享和只读权限
        READLOCK(entry_mutex);
        sleep(5);
        std::map<std::string, dns_entry>::const_iterator const it =entries.find(domain);
        return (it == entries.end()) ? dns_entry() : it->second;
    }
    void update_or_add_entry(std::string const& domain, dns_entry const& dns_details) {
        cout<<"写之前"<<endl;
        std::lock_guard<boost::shared_mutex> lk(entry_mutex); //  表格需要更新时独占
        cout<<"写之后"<<endl;
        entries[domain] = dns_details;
    }
    static  dns_cache& getInstance()
    {
        static dns_cache  dnscache;
        return dnscache;
    }
};


#if 0 
dns_cache  dnscache;

void readfun( )
{
    dnscache.find_entry("xxxxxx"); 
}

void writefun( )
{
    dns_entry dnseny;
    dnscache.update_or_add_entry("xxxxxx", dnseny); 
}
#else

void readfun( )
{
    dns_cache::getInstance().find_entry("xxxxxx"); 
}

void writefun( )
{
    dns_entry dnseny;
    dns_cache::getInstance().update_or_add_entry("xxxxxx", dnseny); 
}

#endif



int main()
{
    
    std::thread t1(readfun);
    std::thread t2(writefun);
    t1.join();
    t2.join();
    return 0;
}
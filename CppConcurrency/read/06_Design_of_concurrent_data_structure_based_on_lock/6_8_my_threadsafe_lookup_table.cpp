//
//  6.8.cpp 基于锁设计更加复杂的数据结构->编写一个使用锁的线程安全查询表
//  Cpp-Concurrency
//

//  最复杂的接口->迭代器(虽然可能在多线程下会有提供安全访问的迭代器，但是当迭代器引用的元素被其他线程删除时...)，所以我们先绕过它
//  实现的基本操作:
//  1、添加一对 "key-value"
//  2、修改指定key的value
//  3、删除一组值
//  4、通过给定key,获取value
//  !!!不要返回一个引用!!!
//  三种关联容器：
//  1. 二叉树(红黑树)->根节点需要上锁，向下移动时需要释放，所以相比横跨整个数据结构的单锁，没有优势
//  2. 有序数组->最坏的选择，无法提前说明数组哪段是有序的，所以要锁住整个数组
//  3. 哈希表->每个桶都有一个键值及散列函数，所以可以安全对每个桶上锁。再次使用互斥量（支持多读者单作者）时，可以将并发访问的可能性增加N倍，N是桶的数量。并且可以直接用std::hash<>当做哈希函数
//  线程安全的查询表的实现


/*
可以参照
https://codeleading.com/article/63482712217/
*/


#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <list>
#include <utility>
#include <thread>
#include <boost/thread/shared_mutex.hpp>

template <typename Key, typename Value, typename Hash=std::hash<Key> >
class threadsafe_lookup_table {
private:
    class bucket_type {
    private:
        typedef std::pair<Key, Value> bucket_value;
        typedef std::list<bucket_value> bucket_data;
        typedef typename bucket_data::iterator bucket_iterator;
        typedef typename bucket_data::const_iterator bucket_const_iterator;
        
    public:
        bucket_data& getData()
        {
            return data;
        }
        boost::shared_mutex & getMutex()
        {
            return mutex;
        }
    private:
        bucket_data data;
        mutable boost::shared_mutex mutex; // 用 boost::shared_mutex 实例锁保护每个桶，来允许并发读取 或只有一个线程对一个桶修改

        // 确定数据是否在桶上，每个桶都包含一个 "key-value"的 std::list<>列表，所以添加和删除数据容易
        bucket_iterator find_entry_for(Key const& key)  {
            return std::find_if(data.begin(), data.end(),
                                [&](bucket_value const & item) { return item.first == key; });
        }
#if 1
        bucket_const_iterator find_entry_for(Key const& key) const
		{
			//由于要返回非const的迭代器所以将数据放置到非const变量返回
			return std::find_if(data.begin(), data.end(),
			                [&](bucket_value const& item) {return item.first==key;});
		}
#endif

    public:
        // 通过给定key,获取value -> ”异常-安全“
        Value value_for(Key const& key, Value const& default_value)  {
            // 共享（只读）所有权的时候上锁
            boost::shared_lock<boost::shared_mutex> lock(mutex);
            bucket_iterator const found_entry = find_entry_for(key);
            return (found_entry == data.end()) ? default_value : found_entry->second;
        }
        
        // 添加或者修改一对 "key-value" -> ”异常-安全“
        void add_or_update_mapping(Key const& key, Value const& value) {
            // 获取唯一（读/写）权的时候上锁
            std::unique_lock<boost::shared_mutex> lock(mutex);
            bucket_iterator const found_entry = find_entry_for(key);
            if (found_entry == data.end()) {
                data.push_back(bucket_value(key, value));
            }
            else {
                found_entry->second = value;
            }
        }
        
        // 删除一组值
        void remove_mapping(Key const& key) {
            // 获取唯一（读/写）权的时候上锁
            std::unique_lock<boost::shared_mutex> lock(mutex);
            bucket_iterator const found_entry = find_entry_for(key);
            if (found_entry != data.end()) {
                data.erase(found_entry);
            }
        }
    };
    
    std::vector<std::unique_ptr<bucket_type> > buckets; // 保存桶
    Hash hasher;
    
    // 因为每个桶的数量是固定的，所以get_bucket()可以无锁调用
    bucket_type& get_bucket(Key const& key) const {
        std::size_t const bucket_index = hasher(key) % buckets.size();
        return *buckets[bucket_index];
    }
    
public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Hash hash_type;
    
    // 在构造函数中指定构造桶的数量（质数），默认为19
    threadsafe_lookup_table(unsigned num_buckets = 19, Hash const& hasher_ = Hash()) : buckets(num_buckets), hasher(hasher_) {
        for (unsigned i = 0; i < num_buckets; ++i) {
            buckets[i].reset(new bucket_type);
        }
    }
    
    threadsafe_lookup_table(threadsafe_lookup_table const& other) = delete;
    threadsafe_lookup_table& operator=(threadsafe_lookup_table const& other) = delete;
    
    Value value_for(Key const& key, Value const& default_value = Value()) 
    {
        return get_bucket(key).value_for(key, default_value); // 无锁调用
    }
    
    void add_or_update_mapping(Key const& key, Value const& value) {
        get_bucket(key).add_or_update_mapping(key, value); // 无锁调用
    }
    
    void remove_mapping(Key const& key) {
        get_bucket(key).remove_mapping(key); // 无锁调用
    }

    std::map<Key, Value> get_map() const
	{
		std::vector<std::unique_lock<boost::shared_mutex> > locks;
		for (unsigned i = 0; i<buckets.size(); ++i)
		{
			locks.push_back(
				//std::unique_lock<boost::shared_mutex>(buckets[i]->mutex));
				std::unique_lock<boost::shared_mutex>(buckets[i]->getMutex()));
		}
		std::map<Key, Value> res;
		for (unsigned i = 0; i<buckets.size(); ++i)
		{
			//for (auto it = buckets[i]->data.begin(); it != buckets[i]->data.end();++it)
			for (auto it = buckets[i]->getData().begin(); it != buckets[i]->getData().end();++it)
			{
				res.insert(*it);
			}
		}
		return res;
	}

	std::vector<Key> get_Key() const
	{
		std::vector<std::unique_lock<boost::shared_mutex> > locks;
		for (unsigned i = 0; i<buckets.size(); ++i)
		{
			locks.push_back(
				//std::unique_lock<boost::shared_mutex>(buckets[i]->mutex));
                std::unique_lock<boost::shared_mutex>(buckets[i]->getMutex()));
		}
		std::vector<Key> res;
		for (unsigned i = 0; i<buckets.size(); ++i)
		{
			//for (auto it = buckets[i]->data.begin();it != buckets[i]->data.end();++it)
            for (auto it = buckets[i]->getData().begin(); it != buckets[i]->getData().end();++it)
			{
				res.push_back(it->first);
			}
		}
		return res;
	}
};




threadsafe_lookup_table<int,int> g_lookup_table;

void add_data()
{
	int loop_count = 10;
	for (int i = 0; i < loop_count; ++i)
	{
		g_lookup_table.add_or_update_mapping(i, i);
	}
}

void print_data()
{
	int loop_count = 10;
	for (int i = 0; i < loop_count; ++i)
	{
		std::cout << g_lookup_table.value_for(i, 0) << std::endl;
	}

    std::map<int, int> strmap;
    strmap = g_lookup_table.get_map();

    for(auto it : strmap)
    {
        std::cout<<it.first<<" : "<<it.second<<std::endl;
    }
}

int main()
{
	auto t1 = std::thread(add_data);
	auto t2 = std::thread(print_data);

	t1.join();
	t2.join();
}
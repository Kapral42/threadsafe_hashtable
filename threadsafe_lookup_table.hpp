#ifndef THREAD_SAFE_HASH_TABLE
#define THREAD_SAFE_HASH_TABLE

/*
* Threadsafe hash-table with fineness locks
* Used threadsafe list with fineness locks
*/

#include <vector>
#include <functional>
#include <list>
#include <utility>
#include <thread>
#include  <mutex>
#include "threadsafe_list.h"

void f_test(std::pair<std::string, int> p)
{
    for (int i = 0; i < 500; ++i) {
           p.second++;
    }
}

/* Template of hash-table class */
template<typename Key, typename Value, typename Hash = std::hash<Key> >
class threadsafe_lookup_table
{
private:
    /* Class of cluster (Node of hash-table) */
    class bucket_type
    {
    private:
        typedef std::pair<Key, Value> bucket_value;
        typedef threadsafe_list<bucket_value> bucket_data;
        typedef typename std::shared_ptr<bucket_value> bucket_entry;

        bucket_data data;

        /* Checking of existing key in cluster */
        bucket_entry find_entry( Key const& key )
        {
            return data.find_first_if([&] (bucket_value const& x) { return x.first == key; });
        }


    public:

        Value get_value( Key const& key, Value const& d_val)
        {
            bucket_entry const entry = find_entry( key );
            return (entry == NULL) ?
                    d_val : entry->second;
        }

        bool set_entry( Key const& key, Value const& value )
        {
            bucket_entry const entry = find_entry( key );
            if( entry == NULL )
            {
                data.push_front( bucket_value( key, value ) );
                return true;
            }
            else
            {
                entry->second = value;
                return false;
            }
        }

        void delete_entry ( Key const& key )
        {
                data.remove_if([&] (bucket_value const& x) { return x.first == key; });
        }

        void test (Key const& key)
        {
            data.for_each(f_test);
        }
    };

    /* All clusters (table of clusters) */
    std::vector<std::unique_ptr<bucket_type>> buckets;
    Hash hasher;
    size_t nbuckets;

    bucket_type& get_bucket( Key const& key ) const
    {
        std::size_t const bucket_index = hasher( key ) % buckets.size();
        return *buckets[bucket_index];
    }
public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Hash hash_type;


    threadsafe_lookup_table(
            size_t num_buckets = 19, Hash const& hasher_ = Hash() ) :
            buckets( num_buckets ), hasher( hasher_ )
    {
        nbuckets = num_buckets;
        for( size_t i = 0; i < num_buckets; ++i )
        {
            buckets[i].reset(new bucket_type) ;
        }
    }

    threadsafe_lookup_table(threadsafe_lookup_table const& other) = delete;
    threadsafe_lookup_table& operator=(threadsafe_lookup_table const& other) = delete;

    Value get_value( Key const& key, Value const& d_val  ) const
    {
        return get_bucket( key ).get_value( key, d_val );
    }

    bool set_entry( Key const& key, Value const& value )
    {
        return get_bucket( key ).set_entry( key, value );
    }

    void delete_entry( Key const& key )
    {
        get_bucket( key ).delete_entry( key );
    }

    void test(Key const& key)
    {
        get_bucket(key).test(key);
    }
};

#endif // ifndef THREAD_SAFE_HASH_TABLE

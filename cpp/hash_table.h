#pragma once

#include "main.h"
#include "list.h"
#include "vector.h"

template <class K, class V>
class HashTable
{
public:
    HashTable(MemType memType, size_t nrBuckets, int err,
              int (*keyCmp)(const K& key1, const K& key2),
              size_t (*keyHash)(const K& key))
        : Buckets(memType), KeyCmp(keyCmp), KeyHash(keyHash),
          MemoryType(memType)
    {
        if (err)
            return;

        if (!Buckets.Reserve(nrBuckets))
        {
            err = E_NO_MEM;
            return;
        }

        for (size_t i = 0; i < nrBuckets; i++)
        {
            LinkedList<HashEntry> list(memType);
            if (!Buckets.PushBack(util::move(list)))
            {
                err = E_NO_MEM;
                return;
            }
        }
    }

    bool Insert(const K& key, const V& value)
    {
        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry>& list = Buckets[bucket];
        typename LinkedList<HashEntry>::Iterator it(list);
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
            {
                return false;
            }
        }
        HashEntry entry(key, value);
        list.AddTail(util::move(entry));

        return true;
    }

    bool Remove(const K& key)
    {
        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry>& list = Buckets[bucket];
        typename LinkedList<HashEntry>::Iterator it(list);
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
            {
                it.Erase();
                return true;
            }
        }
        return false;
    }

    V& Get(const K& key)
    {
        KBUG_ON(!Exists(key));

        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry>& list = Buckets[bucket];
        typename LinkedList<HashEntry>::Iterator it(list);
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
            {
                return entry.GetValue();
            }
        }
        return EmptyValue;
    }

    bool Exists(const K& key)
    {
        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry>& list = Buckets[bucket];
        typename LinkedList<HashEntry>::Iterator it(list);
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
            {
                return true;
            }
        }
        return false;
    }

    virtual ~HashTable()
    {
    }
private:
    class HashEntry
    {
    public:
        HashEntry() {}
        HashEntry(const K& key, const V& value)
        {
            Key = key;
            Value = value;
        }
        virtual ~HashEntry() {}
        HashEntry(HashEntry&& other)
        {
            Key = util::move(other.Key);
            Value = util::move(other.Value);
        }

        HashEntry& operator=(HashEntry&& other)
        {
            Key = util::move(other.Key);
            Value = util::move(other.Value);
            return *this;
        }

        K& GetKey()
        {
            return Key;
        }

        V& GetValue()
        {
            return Value;
        }
    private:
        K Key;
        V Value;
        HashEntry(const HashEntry& other) = delete;
        HashEntry& operator=(const HashEntry& other) = delete;
    };
    Vector<LinkedList<HashEntry>> Buckets;
    int (*KeyCmp)(const K& key1, const K& key2);
    size_t (*KeyHash)(const K& key);
    MemType MemoryType;
    V EmptyValue;
};
#pragma once

#include "memory.h"
#include "error.h"

class Page
{
public:
    Page(Memory::PoolType poolType, Error& err);
    void* Map();
    void Unmap();
    void* GetPage();
    int GetPageSize();
    virtual ~Page();
private:
    Page(const Page& other) = delete;
    Page(Page&& other) = delete;
    Page& operator=(const Page& other) = delete;
    Page& operator=(Page&& other) = delete;

    Memory::PoolType PoolType;
    void* PagePtr;
};
#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <queue>

class MemPool
{
public:
    MemPool();
    ~MemPool();

    struct BlockDesc
    {
        int id;
        int use;
        int front;
        int rear;
        int size;
        char *addr;
    };

    int add_block(int id, int block_size);
    int delete_block(int id);

    int push(int id, char *ptr, int len);
    int pop(int id, char *ptr, int len);
    int read_data(int id, char *ptr, int len);

    int get_block_num();
    BlockDesc get_block_desc(int i);
    int get_len(int id);
    void show_mem_pool_status();

private:
    std::vector<BlockDesc> m_vec_block_desc;

    inline bool is_empty(BlockDesc *block);
    inline bool is_full(BlockDesc *block);
    int push_a(BlockDesc *block, char value);
    char pop_a(BlockDesc *block);
    char read_a(BlockDesc *block, int i_offset);
};

#endif

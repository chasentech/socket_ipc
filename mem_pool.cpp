#include "mem_pool.h"

MemPool::MemPool()
{
    // printf("MemPool()\n");
    m_vec_block_desc.clear();
}

MemPool::~MemPool()
{
    for (int i = 0; i < (int)m_vec_block_desc.size(); i++) {
        free(m_vec_block_desc[i].addr);
    }
    m_vec_block_desc.clear();
}

int MemPool::add_block(int id, int block_size) {
    int i = 0;
    for (i = 0; i < (int)m_vec_block_desc.size(); i++) {
        BlockDesc &block = m_vec_block_desc[i];
        if (block.id == id) {
            //printf("id[%d] exist, create failed!\n", i);
            return -1;
        }
    }

    BlockDesc block_desc;
    block_desc.id = id;
    block_desc.use = 0;
    block_desc.front = 0;
    block_desc.rear = 0;
    block_desc.size = block_size;
    block_desc.addr = new char[block_size]();
    m_vec_block_desc.push_back(block_desc);
    return 0;
}

int MemPool::delete_block(int id)
{
    std::vector<BlockDesc>::iterator it;
    for (it = m_vec_block_desc.begin(); it != m_vec_block_desc.end(); ) {
        if (it->id == id) {
            free(it->addr);
            it = m_vec_block_desc.erase(it);
        }
        else it++;
    }
    return 0;
    // int i = 0;
    // for (i = 0; i < (int)m_vec_block_desc.size(); i++) {
    //     BlockDesc &block = m_vec_block_desc[i];
    //     if (block.id == id) {
    //         //printf("id[%d] exist, create failed!\n", i);
    //         return -1;
    //     }
    // }
}

int MemPool::get_block_num()
{
    return m_vec_block_desc.size();
}

MemPool::BlockDesc MemPool::get_block_desc(int i)
{
    return m_vec_block_desc[i];
}

int MemPool::get_len(int id)
{
    for (int i = 0; i < (int)m_vec_block_desc.size(); i++) {
        BlockDesc &block = m_vec_block_desc[i];
        if (block.id == id) {
            return block.use;
        }
    }
    return 0;
}

void MemPool::show_mem_pool_status()
{
    printf("mem pool info:\n");
    for (int i = 0; i < (int)m_vec_block_desc.size(); i++) {
        BlockDesc &block = m_vec_block_desc[i];
        printf("id=%d ", block.id);
        printf("size=%d ", block.size);
        printf("use=%dB ", block.use);
        printf("front=%d ", block.front);
        printf("rear=%d ", block.rear);
        //printf("data=0x%x ", block.addr);
        printf("\n");
    }
    printf("\n");
}

bool MemPool::is_empty(BlockDesc *block)
{
    if (block->rear == block->front && block->use < block->size) {
        return true;
    }
    else return false;
}
bool MemPool::is_full(BlockDesc *block)
{
    if (block->use >= block->size) {
        return true;
    }
    else return false;
}

int MemPool::push_a(BlockDesc *block, char value)
{
    if (is_full(block) == false) {
        block->addr[block->rear] = value;
        block->rear = (block->rear + 1) % block->size;
        block->use++;
    }
    else {
        block->addr[block->rear] = value;
        block->rear = (block->rear + 1) % block->size;
        block->front = block->rear;
    }

    return 0;
}

int MemPool::push(int id, char *ptr, int len)
{
    int i = 0;
    for (i = 0; i < (int)m_vec_block_desc.size(); i++) {
        if (m_vec_block_desc[i].id == id) {
            //printf("use %d block in mem pool.\n", i);
            break;
        }
    }
    if (i == (int)m_vec_block_desc.size()) {
        //printf("block[%d] not exist, please add block.\n", i);
        return -1;
    }

    BlockDesc *block = &m_vec_block_desc[i];

    if (len > block->size) {
        ptr = &ptr[len - block->size];
        len = block->size;
    }

    //TODO full station(rewrite or pop)
    if (len + block->use > block->size) {
        printf("mem pool block[%d] full, please pop data\n", i);
        return -1;
    }

    for (i = 0; i < len; i++) {
        push_a(block, ptr[i]);
    }
    return len;
}

char MemPool::pop_a(BlockDesc *block)
{
    char ret = -1;
    // printf("block->use = %d\n", block->use);
    // if (block->use >= 0) {
    //     ret = block->addr[block->front];
    //     // printf("block->front %d\n", block->front);
    //     block->front = (block->front + 1) % block->size;
    //     block->use--;
    // }
    if (is_empty(block) == false) {
        ret = block->addr[block->front];
        // printf("block->front %d\n", block->front);
        block->front = (block->front + 1) % block->size;
        block->use--;
    }

    return ret;
}

int MemPool::pop(int id, char *ptr, int len)
{
    int i = 0;
    for (i = 0; i < (int)m_vec_block_desc.size(); i++) {
        if (m_vec_block_desc[i].id == id) {
            //printf("use %d block in mem pool.\n", i);
            break;
        }
    }
    if (i == (int)m_vec_block_desc.size()) {
        //printf("block[%d] not exist, please add block.\n", i);
        return -1;
    }

    BlockDesc *block = &m_vec_block_desc[i];

    if (len > block->size) {
        len = block->size;
    }

    for (i = 0; i < len; i++) {
        ptr[i] = pop_a(block);
    }
    return len;
}

char MemPool::read_a(BlockDesc *block, int i_offset)
{
    char ret = -1;
    // printf("block->use = %d\n", block->use);
    int pos = (block->front + i_offset) % block->size;
    if (is_empty(block) == false) {
        ret = block->addr[pos];
    }

    return ret;
}

int MemPool::read_data(int id, char *ptr, int len)
{
    int i = 0;
    for (i = 0; i < (int)m_vec_block_desc.size(); i++) {
        if (m_vec_block_desc[i].id == id) {
            //printf("use %d block in mem pool.\n", i);
            break;
        }
    }
    if (i == (int)m_vec_block_desc.size()) {
        //printf("block[%d] not exist, please add block.\n", i);
        return -1;
    }

    BlockDesc *block = &m_vec_block_desc[i];

    if (len > block->size) {
        len = block->size;
    }

    for (i = 0; i < len; i++) {
        ptr[i] = read_a(block, i);
    }
    return len;
}

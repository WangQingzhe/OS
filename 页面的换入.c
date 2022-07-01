void read_from_swap(unsigned long page, int swapno) /* 从swap读一个页面到内存 */
{
    struct buffer_head *bh[4];
    int i;
    for (i = 0; i < 4; i++)
    {
        bh[i] = getblk(SWAP_DEV, i + swapno * SWAP_BLOCK_SIZE / BUFFER_BLOCK_SIZE);
        ll_rw_block(READ, bh[i]);
        wait_on_buffer(bh[i]);
        COPYBLK(bh[i]->b_data, page);
        brelse(bh[i]);
        page += BUFFER_BLOCK_SIZE * 512;
    }
}

int find_bucket(pid, address) /* 查找散列表，查询页面存放在swap的哪个块 */
{
    int swapno = (hash_a * pid + hash_b * address + hash_c) % swapblocks;
    while (swapno < swapblocks)
    {
        if (swap_hash_table[swapno].valid)
            if (swap_hash_table[swapno].pid == pid && swap_hash_table[swapno].pageno == address)
            {
                swap_hash_table[swapno].valid = 0; /* 若查询到块号，将页面从散列表中去除 */
                return swapno;
            }
        swapno++;
    }
    return -1;
}

void do_no_page(unsigned long error_code, unsigned long address)
{
    int nr[4];
    unsigned long tmp;
    unsigned long page;
    int block, i;
    int swapno;
    struct buffer_head *bh[4];

    address &= 0xfffff000;
    tmp = address - current->start_code;
    if (!current->executable || tmp >= current->end_data)
    {
        get_empty_page(address);
        return;
    }
    if (share_page(tmp))
        return;
    if (!(page = get_free_page()))
        oom();
    swapno = find_bucket(current->pid, address);
    if (swapno >= 0) /* 页面位于swap中，查到块号 */
    {
        read_from_swap(page, swapno);
        put_page(page, address); /* 更新页表项 */
        /* 在clock队列中增加节点 */
        struct clock_ring_node *new_node = (struct clock_ring_node *)malloc(sizeof(struct clock_ring_node));
        new_node->pid = current->pid;
        new_node->pageno = address;
        /* 将节点插到swap_pos的prev */
        new_node->prev = swap_pos->prev;
        new_node->next = swap_pos;
        swap_pos->prev->next = new_node;
        swap_pos->prev = new_node;
        swap_hash_table[swapno].valid = 0;
        pages_in_clock++;
    }
    /* remember that 1 block is used for header */
    else /* 页面位于第一块磁盘，从该磁盘换入 */
    {
        block = 1 + tmp / BLOCK_SIZE;
        for (i = 0; i < 4; block++, i++)
            nr[i] = bmap(current->executable, block);
        bread_page(page, current->executable->i_dev, nr);
        i = tmp + 4096 - current->end_data;
        tmp = page + 4096;
        while (i-- > 0)
        {
            tmp--;
            *(char *)tmp = 0;
        }
        struct clock_ring_node *new_node = (struct clock_ring_node *)malloc(sizeof(struct clock_ring_node));
        new_node->pid = current->pid;
        new_node->pageno = address;
        if (pages_in_clock == 0) /* clock队列为空 */
        {
            scan_pos = new_node;
            swap_pos = new_node;
            new_node->next = new_node;
            new_node->prev = new_node;
        }
        else if (pages_in_clock == 1) /* clock队列只有1个节点 */
        {
            scan_pos->next = new_node;
            scan_pos->prev = new_node;
            new_node->next = scan_pos;
            new_node->prev = scan_pos;
            swap_pos = new_node;
        }
        else if (pages_in_clock == 2) /* 2个节点 */
        {
            scan_pos->prev = new_node;
            swap_pos->next = new_node;
            new_node->next = scan_pos;
            new_node->prev = swap_pos;
        }
        else if (pages_in_clock == 3) /* 3个节点 */
        {
            new_node->prev = swap_pos;
            new_node->next = swap_pos->next;
            swap_pos->next->prev = new_node;
            swap_pos->next = new_node;
        }
        else /* 4个及以上节点 */
        {
            new_node->prev = swap_pos->prev;
            new_node->next = swap_pos;
            swap_pos->prev->next = new_node;
            swap_pos->prev = new_node;
        }
        pages_in_clock++;
        if (put_page(page, address))
            return;
        free_page(page);
        oom();
    }
}
void clock_scan() /* clock_scan指针的实现 */
{
    unsigned long scan_address = scan_pos->pageno;
    unsigned long *dir = (scan_address >> 20) & 0xffc;
    unsigned long *pg_table = (0xfffff000) & (*dir);
    unsigned long pg_entry = pg_table[(scan_address >> 12) & 0x3ff];
    pg_table[(scan_address >> 12) & 0x3ff] = pg_entry & (0xffffffdf);
    scan_pos = scan_pos->next;
}

int get_new_bucket(int pid, int pageno) /* 为即将换出的页面分配swap中的块号 */
{
    int swapno = (hash_a * pid + hash_b * pageno + hash_c) % swapblocks;
    while (swapno < swapblocks)
    {
        if (!swap_hash_table[swapno].valid)
        {
            swap_hash_table[swapno].valid = 1;
            swap_hash_table[swapno].pid = pid;
            swap_hash_table[swapno].pageno = pageno;
            swap_hash_table[swapno].next = -1;
            return swapno;
        }
        swap_hash_table[swapno].next = swapno + 1;
        swapno++;
    }
    return swapno;
}

void write_to_swap(unsigned long phys_addr, int swapno) /* 将物理页面写到swap分区 */
{
    struct buffer_head *bh[4];
    int i;
    for (i = 0; i < 4; i++) /* swap的块大小为缓冲区块大小的4倍 */
    {
        bh[i] = getblk(SWAP_DEV, i + swapno * SWAP_BLOCK_SIZE / BUFFER_BLOCK_SIZE);
        COPYBLK(phys_addr + i * 1024, bh[i]->b_data);
        bh[i]->b_dirt = 1;
        ll_rw_block(WRITE, bh[i]);
        wait_on_buffer(bh[i]);
        brelse(bh[i]);
    }
}

void clock_swap() /* clock_swap指针的实现 */
{
    unsigned long swap_address = swap_pos->pageno;
    unsigned long *dir = (swap_address >> 20) & 0xffc;
    unsigned long *pg_table = (0xfffff000) & (*dir);
    unsigned long pg_entry = pg_table[(swap_address >> 12) & 0x3ff]; /* 获取页表项 */
    unsigned long phys_addr;
    if (pg_entry & 0x20 == 0) /* clock节点访问位R为0 */
    {
        phys_addr = pg_entry & 0xfffff000; /* 获取物理地址 */
        phys_addr -= LOW_MEM;
        phys_addr >>= 12;
        mem_map[phys_addr]--;
        pg_table[(swap_address >> 12) & 0x3ff] = pg_entry & 0;
        swap_pos->next->prev = swap_pos->prev;
        swap_pos->prev->next = swap_pos->next;
        pages_in_clock--;                                                                        /* 从clock队列中去掉该节点 */
        write_to_swap((pg_entry & 0xfffff000), get_new_bucket(swap_pos->pid, swap_pos->pageno)); /* 将该页面写到swap分区 */
    }
    swap_pos = swap_pos->next;
}

void do_timer(long cpl)
{
    extern int beepcount;
    extern void sysbeepstop(void);
    beepcount1++;
    if (beepcount1 == 10) /* 每10次中断，指针移动一次 */
    {
        if (pages_in_clock > 4) /* clock队列有4个以上节点时才执行换出 */
        {
            clock_scan();
            clock_swap();
        }
        beepcount1 = 0;
    }
    if (beepcount)
        if (!--beepcount)
            sysbeepstop();

    if (cpl)
        current->utime++;
    else
        current->stime++;

    if (next_timer)
    {
        next_timer->jiffies--;
        while (next_timer && next_timer->jiffies <= 0)
        {
            void (*fn)(void);

            fn = next_timer->fn;
            next_timer->fn = NULL;
            next_timer = next_timer->next;
            (fn)();
        }
    }
    if (current_DOR & 0xf0)
        do_floppy_timer();
    if ((--current->counter) > 0)
        return;
    current->counter = 0;
    if (!cpl)
        return;
    schedule();
}
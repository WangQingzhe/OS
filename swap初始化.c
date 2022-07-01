void swap_init(void *BIOS)
{
    int i;
    swap_dev.cyl = *(unsigned short *)BIOS;        // cylinders
    swap_dev.head = *(unsigned short *)(2 + BIOS); // heads
    swap_dev.wpcom = *(unsigned short *)(5 + BIOS);
    swap_dev.ctl = *(unsigned char *)(8 + BIOS);
    swap_dev.lzone = *(unsigned short *)(12 + BIOS);
    swap_dev.sect = *(unsigned char *)(14 + BIOS); // sectors per track
    printk("swap dev:cyls: %d, heads: %d, sects: %d\n",
           swap_dev.cyl, swap_dev.head, swap_dev.sect);
    swapblocks = swap_dev.cyl * swap_dev.head * swap_dev.sect /
                 SWAP_BLOCK_SIZE;
    swap_hash_table = (struct swap_hash_node *)malloc(swapblocks * sizeof(struct swap_hash_node));
    for (i = 0; i < swapblocks; i++)
        swap_hash_table[i].valid = 0;
}
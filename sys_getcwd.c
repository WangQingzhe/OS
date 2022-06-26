long sys_getcwd(char *buf, size_t size)
{
    if ((!buf) || (!size))
        return 0;
    struct m_inode *now_inode, *root_inode;

    char ans[128], base[128];
    struct buffer_head *bh;
    struct dir_entry *dir_now, *temp;
    unsigned short dir_nownode, dir_rootnode;
    int now_block, now_idev;
    int entries, i;

    ans[0] = 0; //清空字符串

    now_inode = current->pwd, root_inode = current->root; //获取索引节点
    now_block = now_inode->i_zone[0];                     //获取新数据块
    if (!now_block)
        return 0;
    now_idev = now_inode->i_dev; /*get now_idev*/

    if (!now_idev)
        return 0;
    if (!(bh = bread(current->pwd->i_dev, now_block))) /*read bh*/
        return 0;

    dir_now = (struct dir_entry *)bh->b_data;
    dir_nownode = dir_now->inode;
    dir_now++;
    dir_rootnode = dir_now->inode;

    while (now_inode != root_inode)
    {
        now_inode = iget(now_idev, dir_rootnode);
        if (!(now_block = now_inode->i_zone[0]))
            return 0;
        now_idev = now_inode->i_dev; //获取当前设备号
        if (!now_idev)
            return 0;
        if (!(bh = bread(now_inode->i_dev, now_block))) //读取数据块
            return 0;
        dir_now = (struct dir_entry *)bh->b_data;
        temp = dir_now;

        entries = now_inode->i_size / (sizeof(struct dir_entry));

        for (i = 0; i < entries; ++i)
        {
            if (temp->inode == dir_nownode)
            {
                base[0] = 0;
                strcat(base, "/");
                strcat(base, temp->name);
                strcat(base, ans);
                strcpy(ans, base);
                break;
            }
            ++temp;
        }

        dir_nownode = dir_rootnode;
        dir_rootnode = (++dir_now)->inode;
    }

    size_t len = strlen(ans);
    if (len == 0)
    {
        strcpy(ans, "/");
        ++len;
    }
    if (size < len)
    {
        return 0;
    }
    char *p = buf;
    for (i = 0; i < len; ++i)
        put_fs_byte(ans[i], p++);

    return buf;
}

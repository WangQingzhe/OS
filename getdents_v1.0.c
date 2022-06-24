int sys_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count)
{
    struct m_inode *inode;// 当前目录的索引节点
    struct buffer_head *bh;
    struct dir_entry *dir;
    struct linux_dirent usr;//暂存
    int nread = 0;
    int i = 0, j = 0;
    inode = current->filp[fd]->f_inode;//获取该文件描述符对应文件的索引节点
    bh = bread(inode->i_dev, inode->i_zone[0]);//获取索引节点对应的数据块
    dir = (struct dir_entry *)bh->b_data;//将数据块转换为目录项的数据结构
    while (dir[i].inode > 0)//遍历目录项
    {
        if (nread + sizeof(struct linux_dirent) > count)
            break;
        //存数据
        usr.d_ino = dir[i].inode;
        usr.d_off = 0;
        usr.d_reclen = sizeof(struct linux_dirent);
        for (j = 0; j < 14; j++)
            usr.d_name[j] = dir[i].name[j];
        //将数据从usr迁移到dirp
        for (j = 0; j < sizeof(struct linux_dirent); j++)
        {
            put_fs_byte(((char *)(&usr))[j], (char *)dirp + nread);
            nread++;
        }
        i++;
    }
    return nread;
}

int sys_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count)
{
    struct m_inode *inode;
    struct buffer_head *bh;
    struct dir_entry *dir;
    struct linux_dirent usr;
    int nread = 0;
    int i = 0, j = 0;
    inode = current->filp[fd]->f_inode;
    bh = bread(inode->i_dev, inode->i_zone[0]);
    dir = (struct dir_entry *)bh->b_data;
    while (dir[i].inode > 0)
    {
        if (nread + sizeof(struct linux_dirent) > count)
            break;
        usr.d_ino = dir[i].inode;
        usr.d_off = 0;
        usr.d_reclen = sizeof(struct linux_dirent);
        for (j = 0; j < 14; j++)
            usr.d_name[j] = dir[i].name[j];
        for (j = 0; j < sizeof(struct linux_dirent); j++)
        {
            put_fs_byte(((char *)(&usr))[j], (char *)dirp + nread);
            nread++;
        }
        i++;
    }
    return nread;
}

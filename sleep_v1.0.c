int sys_sleep(unsigned int seconds)
{
    int a = jiffies;
    sys_signal(SIGALRM, SIG_IGN); //设置接收到SIGALRM后默认处理为忽略

    sys_alarm(seconds); // seconds秒后向自己发送一个SIGALRM信号
    pause();            //开始休眠
    if ((jiffies - a) / 100 == seconds)
        return 0; //当休眠的秒数等于seconds时返回0
    return -1;
}

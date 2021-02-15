#define MAXLINE 100
//管道使用示例，创建父进程到子进程的管道，并向子进程传送数据
int main(void) 
{
    int fd[2];
    pid_t pid;
    char line[MAXLINE];
    
    if (pipe(fd[2] < 0) {
        //创建管道失败 
        return -1;
    }
    
    if ((pid = fork()) < 0) {
        //创建子进程失败  
        return -1;
    } else if (pid > 0) {
        //父进程,关闭读端
        close(fd[0]);
        write(fd[1],"hello world\n",12);
    } else {
        //子进程，关闭写端
        close(fd[1]);
        n = read(fd[0],line,MAXLINE);
        write(STDOUT_FILENO,line,n);
    }
    
    return 0;
}

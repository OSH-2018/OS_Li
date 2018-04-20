#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>

extern char **environ;
int pipefd[2];
int pipe_num;
char **printEnviron()
{
     return environ;
}
char *printWorkingDirectory()
{
    char wd[4096];
    return getcwd(wd,4096);
}


void pipeCommand(char **args)						//处理管道命令
{
    char *new_args[pipe_num + 1][128];
    int position[pipe_num];

    for(int i = 0,j = 0;; i++)
    {
        if(!args[i])break;
        if(strcmp(args[i],"|") == 0)
        {
            position[j] = i;
            j++;
        }
    }
    int i = 0;

    for(int j = 0; j < pipe_num + 1; j++)				//分割字符串，将命令以|为界限分开
    {
        if(j == 0)
        {
            for(; i < position[0]; i++)new_args[0][i] = args[i];
            new_args[0][i] = NULL;
        }
        else if(j == pipe_num)
        {
            for(; args[i]; i++)new_args[j][i - position[j - 1] - 1] = args[i];
            new_args[j][i] = NULL;
        }
        else
        {
            for(; i < position[j]; i++)new_args[j][i - position[j - 1] - 1] = args[i];
            new_args[j][i] = NULL;
        }
    }
	
    //以下执行管道命令的部分
    //原本想用循环来处理任意多个管道，但总是有错误，所以先交一个比较粗暴的版本（根据管道个数分类）
    pipe(pipefd);
    if(pipe_num == 2)
    {
        pid_t pid1,pid2;
        pid_t pid;
        pipe(pipefd);
        pid = fork();
        if(pid == 0)								//处理命令的第一部分
        {
            pid1 = fork();
            if(pid1 == 0)
            {
                dup2(pipefd[1],fileno(stdout));					//输出重定向
                close(pipefd[0]);
                close(pipefd[1]);
                if(strcmp(new_args[0][0],"env") == 1)printEnviron();
                else if(strcmp(new_args[0][0],"pwd") == 1)printWorkingDirectory();
                else execvp(new_args[0][0],new_args[0]);
            }
            else
            {
                dup2(pipefd[0],fileno(stdin));					//输入重定向
                close(pipefd[0]);
                close(pipefd[1]);
                pipe(pipefd);
                pid2 = fork();
                if(pid2 == 0)							//处理命令的第二部分
                {
                    dup2(pipefd[1],fileno(stdout));				//输出重定向
                    close(pipefd[0]);
                    close(pipefd[1]);
                    if(strcmp(new_args[1][0],"env") == 1)printEnviron();
                    else if(strcmp(new_args[1][0],"pwd") == 1)printWorkingDirectory();
                    else execvp(new_args[1][0],new_args[1]);
                }
                else								//处理命令的第三部分
                {
                    dup2(pipefd[0],fileno(stdin));				//输入重定向
                    close(pipefd[0]);
                    close(pipefd[1]);
                    if(strcmp(new_args[2][0],"env") == 1)printEnviron();
                    else if(strcmp(new_args[2][0],"pwd") == 1)printWorkingDirectory();
                    else execvp(new_args[2][0],new_args[1]);
                }
                close(pipefd[0]);
                close(pipefd[1]);
                waitpid(pid2,NULL,0);
            }
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid1,NULL,0);
        }
        else
        {
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid,NULL,0);
        }
    }
    if(pipe_num == 1)
    {
        pid_t pid1;
        pid_t pid;
        pipe(pipefd);
        pid = fork();
        if(pid == 0)
        {
            pid1 = fork();
            if(pid1 == 0)
            {
                dup2(pipefd[1],fileno(stdout));
                close(pipefd[0]);
                close(pipefd[1]);
                if(strcmp(new_args[0][0],"env") == 1)printEnviron();
                else if(strcmp(new_args[0][0],"pwd") == 1)printWorkingDirectory();
                else execvp(new_args[0][0],new_args[0]);
            }
            else
            {
                dup2(pipefd[0],fileno(stdin));
                close(pipefd[0]);
                close(pipefd[1]);
                if(strcmp(new_args[1][0],"env") == 1)printEnviron();
                else if(strcmp(new_args[1][0],"pwd") == 1)printWorkingDirectory();
                else execvp(new_args[1][0],new_args[1]);
            }
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid1,NULL,0);
        }
        else
        {
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid,NULL,0);
        }
    }

}

int main() {
    char cmd[256];
    char *args[128];
    while (1)
    {
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);
        int i;
        for (i = 0; cmd[i] != '\n'; i++);
        cmd[i] = '\0';
        args[0] = cmd;

        pipe_num = 0;
        for(int j = 0; cmd[j]; j++)
            if(cmd[j] == '|')pipe_num++;
        for (i = 0; *args[i]; i++)
            for (args[i + 1] = args[i] + 1; *args[i + 1]; args[i + 1]++)
            {
                int whether_break = 0;
                if (*args[i + 1] == ' ')
                {
                    *args[i + 1] = '\0';
                    args[i + 1]++;
                    for(;*args[i + 1] == ' ';)args[i + 1]++;
                    whether_break = 1;
                }
                if(*args[i + 1] == '|')
                {
                    *args[i + 1] = '\0';
                    args[i + 1]++;
                    args[i + 2] = args[i + 1];
                    for(;*args[i + 2] == ' ';)args[i + 2]++;
                    args[i + 1] = "|";
                    i++;
                    break;
                }
                if(whether_break)break;
            }
        args[i] = NULL;
        if (!args[0])continue;
        if(pipe_num == 0)
        {
            if (strcmp(args[0], "cd") == 0)
            {
                if (args[1])chdir(args[1]);
                continue;
            }
            if(strcmp(args[0],"env") == 0)
            {
                char **result = printEnviron();
                while(*result)
                {
                    puts(*result);
                    result++;
                }
                continue;
            }
            if(strcmp(args[0],"pwd") == 0)
            {
                puts(printWorkingDirectory());
                continue;
            }
            if(strcmp(args[0],"export") == 0)
            {
                char *name = args[1],*value;
                for(int j = 0; *args[1]; args[1]++,j++)
                    if(*args[1] == '=')
                    {
                        name[j] = '\0';
                        value = ++args[1];
                    }
                setenv(name,value,1);
                continue;
            }
            if (strcmp(args[0], "exit") == 0)return 0;
            pid_t pid = fork();
            if (pid == 0)
            {
                execvp(args[0], args);

                return 255;
            }
            wait(NULL);
        }
        else
        {
            pipeCommand(args);
            continue;
        }
    }
}


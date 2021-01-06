#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> //wait

// в программе создается два процесса-потомка от одного процесса-предка
// первый процесс-потомок выполняет ps, второй процесс потомок выполняет ls
int main()
{
    printf("2 children are created in following program.\n\n");
    int children[2] = {-1, -1};
    char* exec_paths[2] = {"./usr/bin/ps", "./usr/bin/ls"};
    char* exec_calls[2] = {"ps", "ls"};
    char* exec_params[2] = {"-al", "-l"};

    // будет создано два потомка
    for (int i = 0; i < 2; i++)
    {
        children[i] = fork();
        if (children[i] == -1)
        {
            printf("Can't fork child\n");
            return 1;
        }

        if (children[i] == 0)
        {
            // код процесса-потомка
            sleep(1);
            printf("Child: pid=%d, pidid=%d, groupid=%d\n", getpid(), getppid(), getpgrp());
            
            // процесс-потомок вызывает системный вызов execl
            if (execl(exec_paths[i], exec_calls[i], exec_params[i], (char*)NULL) == -1)
                printf("Child with pid=%d failed to exec\n", getpid());

            return 0;
        }
        else
        {
            // код процесса-предка
            // ожидаем завершения процесса-потомка
            int status;
            int ch_pid = wait(&status);

            // интерпретация значения status
            // дочерний процесс завершен нормально
            if (WIFEXITED(status))  
                printf("Child with pid = %d has finished with exit code %d\n", ch_pid, WEXITSTATUS(status));
            // дочерний процесс завершился неперехватываемым сигналом
            else if (WIFSIGNALED(status))   
                printf("Child with pid = %d has finished by signal %d\n", ch_pid, WTERMSIG(status));
            // дочерний процесс остановился
            else if (WIFSTOPPED(status))    
                printf("Child with pid = %d has been stopped by signal %d\n", ch_pid, WSTOPSIG(status));

            sleep(2);
            printf("Parent: pid=%d, childpid=%d, groupid=%d\n\n", getpid(), children[i], getpgrp());
            
            // завершаем процесс-предок на последней итерации
            if (i == 2)
                return 0;
        }
    }
}

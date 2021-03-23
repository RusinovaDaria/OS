// в программе создается три процесса-потомка от одного процесса-предка
int main()
{
    printf("3 children are created in following program.\n\n");

    // будет создано три потомка
    for (int i = 0; i < 3; i++)
    {
        int child = fork();
        if (child == -1)
        {
            printf("Can't fork child\n");
            return 1;
        }

        if (child == 0)
        {
            // код процесса-потомка
            sleep(1);
            printf("Child: pid=%d, pidid=%d, groupid=%d\n", getpid(), getppid(), getpgrp());
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
            printf("Parent: pid=%d, childpid=%d, groupid=%d\n\n", getpid(), child, getpgrp());
        
            // завершаем процесс-предок на последней итерации
            if (i == 2)
                return 0;
        }
    }
}

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> //wait

// в данной программе написан обработчик сигнала SIGTSTP
// (Ctrl + Z с клавиатуры)
// если сигнал был получен, процессы-потомки передают процессу-предку
// соответсвующие сообщения, процесс-предок выводит сообщения на экран 
// если сигнал не был получен, сообщение не передается

int is_catched = 0;
void catcher_SIGTSTP(int signum)
{
    printf("\n");
    is_catched = 1;
}

int main()
{
    int descr[2];
    if (pipe(descr) == -1)
    {
        printf("Failed to pipe.\n");
        return 1;
    }

    // сохранение старого обработчика для дальнейшего восстановления 
    void (*old_handler)(int) = signal(SIGTSTP, catcher_SIGTSTP);

    // будет создано два потомка
    int children[2];
    for (int i = 0; i < 2; i++)
    {
        // создание потомка
        children[i] = fork();
        if (children[i] == -1)
        {
            printf("Can't fork child\n");
            return 1;
        }

        if (children[i] == 0)
        {
            // код процесса-потомка
            sleep(5);
            
            // если сигнал был получен 
            if (is_catched)
            {
                // подготовка сообщения
                char child_msg[64];
                sprintf(child_msg, "Message from child with pid = %d - signal was catched", getpid());
                
                // процесс-потомок передает сообщение 
    // процессу предку через программный канал
                close(descr[0]); // закрываем на чтение
                if (write(descr[1], child_msg, 64) > 0)
                    printf("Child with pid = %d, ppid = %d has written to pipe\n", getpid(), getppid()); // пишем в канал 
            }
            // если сигнал не был получен ничего не происходит
            
            return 0;
        }

    }

    if (children[0] > 0 && children[1] > 0)
    {
        // код процесса - предка

        // ожидание завершения обоих процессов-потомков
        int st1, st2;
        if (waitpid(children[0], &st1, 0) == -1)
        {
            printf("Error while waiting for child with pid = %d", children[0]);
            return 1;
        }
        if (waitpid(children[1], &st2, 0) == -1)
        {
            printf("Error while waiting for child with pid = %d", children[1]);
            return 1;
        }

        if (is_catched)
        {
            // закрытие программного канала на запись
            close(descr[1]);
            char msg1[64];
            char msg2[64];

            // чтение сообщений от обоих потомков
            read(descr[0], msg1, 64);
            read(descr[0], msg2, 64);

            // выводы сообщений от потомков в консоль
            printf("Parent with pid = %d has read from pipe: %s",getpid(),msg1);
            printf("Parent with pid = %d has read from pipe: %s",getpid(),msg2);
        }
        
        printf("\nEnd of programm.\n");
        return 0;
    }
}

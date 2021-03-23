#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> //wait

int main()
{
    // заводим массив файловых дескрипторов
    // массив файловыйх дескрипторов - средство взаимоисключения
    // необходимо для организации монопольного доступа:
    // в канал нельзя писать, если из него читают; 
    // нельзя читать, если в него пишут
    // descr[0] - на чтение  
    // descr[1] - на запись
    // Канал уничтожается, когда закрыты все файловые дескрипторы,
    // ссылающиеся на него.

    // массив из двух файловых дескрипторов
    int descr[2];
    if (pipe(descr) == -1)
    {
        printf("Failed to pipe.\n");
        return 1;
    }

    printf("2 children are created in following program.\n\n");
    int children[2] = {-1, -1};

    // будет создано два потомка
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

            // подготовка сообщения
            char msg[50];
            sprintf(msg, "Message from child with pid = %d", getpid());

            // процесс-потомок передает сообщение 
// процессу предку через программный канал
            close(descr[0]); // закрываем на чтение
            if (write(descr[1], msg, 50) > 0)
                printf("Child with pid = %d, ppid = %d has written to pipe\n", 
getpid(), getppid()); // пишем в канал 
            
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

        // закрытие программного канала на запись
        close(descr[1]);
        char msg1[50];
        char msg2[50];

        // чтение сообщений от обоих потомков
        read(descr[0], msg1, 50);
        read(descr[0], msg2, 50);

        // выводы сообщений от потомков в консоль
        printf("Parent with pid = %d has read from pipe: %s\n", getpid(), msg1);
        printf("Parent with pid = %d has read from pipe: %s\n", getpid(), msg2);
        
        return 0;
    }
}

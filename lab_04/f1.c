#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>

// в программе создается три процесса-потомка от одного процесса-предка
int main()
{
    printf("3 children are created in following program.\n\n");

    // пусть будет создано три потомка
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
            printf("Child: pid=%d, parentpid=%d, groupid=%d\n", getpid(), getppid(), getpgrp());

            // чтобы предок завершился раньше своих потомков
            sleep(10);
            return 0;
        }
        else
        {
            // код процесса-предка
            sleep(2);
            printf("Parent: pid=%d, childpid=%d, groupid=%d\n\n", getpid(), child, getpgrp());
        
            // завершаем процесс предок после последней итерации
            if (i == 2)
                return 0;
        }
    }
}

// не менее 3 писателей
// не менее 5 читателей

// читатели могут только читать данные, могут работать параллельно, тк не мешают друг другу
// писатели могут только изменять данные, могут работать только в режиме монопольного доступа
// когда работает писатель, то другие писатели и читатели не могут получить доступ к этой переменной

#include <sys/sem.h> // struct sembuf
#include <sys/stat.h> // IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO
#include <sys/shm.h> // разделяемая память
#include <sys/wait.h> // wait(int *status)
#include <stdio.h> // printf
#include <stdlib.h> // rand()
#include <unistd.h> // fork

#define COUNT 14  // как только значение ресурса достигает 14, 
                  // завершаем работу программы
#define WRITERS 3
#define READERS 5

const int FLAGS = IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO;
#define SEM_ID 321  // идентификатор для создания набора семафоров
#define SHM_ID 321  // идентификатор для создание сегмента разделяемой памяти

// идентификаторы семафоров в наборе
#define ACTIVE_READERS_SEM 0 
#define ACTIVE_WRITER_SEM 1  
#define WAITING_READERS_SEM 2 
#define WAITING_WRITERS_SEM 3 

// до входа писателя в крит секцию 
struct sembuf start_write[5] = { 
    { WAITING_WRITERS_SEM,  1, 0 },     
    { ACTIVE_READERS_SEM,   0, 0 },
    { ACTIVE_WRITER_SEM,    0, 0 },  
    { ACTIVE_WRITER_SEM,    1, 0 }, 
    { WAITING_WRITERS_SEM, -1, 0 }  
}; 

// после выхода писателя из крит секции
struct sembuf stop_write[1] = {     
    { ACTIVE_WRITER_SEM, -1, 0 }
}; 

// до входа читателя в крит секцию
struct sembuf start_read[5] = {     
    { WAITING_READERS_SEM,  1, 0 }, 
    { WAITING_WRITERS_SEM,  0, 0 }, 
    { ACTIVE_WRITER_SEM,    0, 0 },
    { ACTIVE_READERS_SEM,   1, 0 }, 
    { WAITING_READERS_SEM, -1, 0 } 
}; 

// после выхода читателя из крит секции
struct sembuf stop_read[1] = {     
    { ACTIVE_READERS_SEM, -1, 0 }
}; 

// РЕСУРС
int* resource;

void start_reader(int sem_fd) { semop(sem_fd, start_read, 5); }
void stop_reader(int sem_fd) { semop(sem_fd, stop_read, 1); }
void start_writer(int sem_fd) { semop(sem_fd, start_write, 5); }
void stop_writer(int sem_fd) { semop(sem_fd, stop_write, 1); }

// процесс-писатель
void writer(int i, int sem_fd) 
{
    while(*resource < COUNT)
    {
        start_writer(sem_fd);

        // изменяем ресурс
        (*resource)++;
        printf("\t\tWriter № %d increased resource up to %d\n", i, *resource);

        stop_writer(sem_fd);
        sleep(rand() % 4);
    }
}

// процесс-читатель
void reader(int i, int sem_fd) 
{
    int stop = 0;
    while(!stop)
    {
        start_reader(sem_fd);

        // читаем ресурс
        printf("Reader № %d read resource: %d\n", i, *resource);
        if (*resource == COUNT)
            stop = 1;

        stop_reader(sem_fd);
        sleep(rand() % 4);
    }
}

int create_shared_memory(int *shm_fd)
{
	// shmget() ; создает разделяемый сегмент
	*shm_fd = shmget(SHM_ID, sizeof(int), FLAGS);
	if (*shm_fd == -1)
	{
		printf("Error in shmget\n");
		return -1;
	}

	// shmat() attach получение указателя на разделяемый сегмен
	resource = (int*) shmat(*shm_fd, 0, 0);
    if (resource == (int*)-1)
    {
        printf("Error in shmat\n");
	    return -1;
    }

	(*resource) = 0;
	return 0;
}

int create_semaphores(int *sem_fd)
{
	// semget(); создаёт набор семафоров
	if ((*sem_fd = semget(SEM_ID, 4, FLAGS)) == -1)
	{
		printf("Error in semget\n");
		return 1;
	}
	semctl(*sem_fd, ACTIVE_READERS_SEM, SETVAL, 0); 
	semctl(*sem_fd, ACTIVE_WRITER_SEM, SETVAL, 0);  
	semctl(*sem_fd, WAITING_READERS_SEM, SETVAL, 0); 
	semctl(*sem_fd, WAITING_WRITERS_SEM, SETVAL, 0); 

	return 0;
}

int main()
{
	// создание и присоединение сегмента разделяемой памяти.
	int shm_fd; 
	if (create_shared_memory(&shm_fd) == -1) 
		return -1;

	// создание набора семафоров
	int sem_fd; 
	if (create_semaphores(&sem_fd) == -1) 
		return -1;
	
	for (int i = 0; i < WRITERS; i++) 
    {
        int pid;
        if ((pid = fork()) == -1) {
            printf("Can't fork writer");
            exit(1);
        }

        if (pid == 0) 
        {
            // код процесса потомка
            writer(i+1, sem_fd);
            return 0;
        }
    }

    for (int i = 0; i < READERS; i++) 
    {
        int pid;
        if ((pid = fork()) == -1) {
            printf("Can't fork reader");
            exit(1);
        }

        if (pid == 0) 
        {
            // код процесса потомка
            reader(i+1, sem_fd);
            return 0;
        }
    }

    for (int i = 0; i < WRITERS + READERS; i++) 
    {
        // процесс предок дожидается завершения процессов потомков
		int status;
        wait(&status);
    }

    shmdt(resource);
    semctl(sem_fd, 0, IPC_RMID);
    shmctl(shm_fd, IPC_RMID, 0);

	return 0;
}

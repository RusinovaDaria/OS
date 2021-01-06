// задача "производство потребление"
// имеется буфер фиксированного размера 
// проивзодитель может произвести единичный объект 
// и поместить его в буфер 
// потребитель может выбирать объекты из буфера по одному и 
// потреблять их
// необходимо обеспечить монопольный доступ производителей и потребителей 
// к буферу 

// для решения задачи используется два считающих семафора:
// * буфер полон - отслеживает количество элементов в буфере в любой момент времени
// * буфер пуст - -//- количество пустых элементов буфера 
// и один бинарный, отслеживающий доступ процессов к буферу

#include <sys/sem.h> // struct sembuf
#include <sys/stat.h> // IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO
#include <sys/shm.h> // разделяемая память
#include <sys/wait.h> // wait(int *status)
#include <stdio.h> // printf
#include <stdlib.h> // rand()
#include <unistd.h> // fork

const int FLAGS = IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO;
#define SEM_ID 123  // идентификатор для создания набора семафоров
#define SHM_ID 123  // идентификатор для создание сегмента разделяемой памяти

#define PRODS 3 // кол-во производитлей
#define CONS 3  // кол-во потребителей

// идентификаторы семафоров в наборе
#define FULL_SEM_NUM  0
#define EMPTY_SEM_NUM 1
#define BIN_SEM_NUM   2

// буфер организован по принципу FIFO 
char* buf;
char* letter;
const int bufsize = 10;
int* produce_pos; // индекс последней свободной ячейки - туда пишем
int* consume_pos; // индекс первой занятой ячейки - оттуда читаем
int* stop_consuming; // флаг, по установке которого в 1, процессы потребители завершаются
                     // в программе этот флаг устанавливается, когда процесс протребитель обнаруживает букву z

// в semop передается массив sembuf - операций над семафорами набора
// над семафорами определены следующие операции:

// перед входом в крит секцию производителя
struct sembuf producer_P[2] = { 
	{EMPTY_SEM_NUM, -1, 0}, 
	{BIN_SEM_NUM, -1, 0}  }; 

// после выхода из крит секции производителя
struct sembuf producer_V[2] = { 
	{BIN_SEM_NUM, 1, 0},    
	{FULL_SEM_NUM, 1, 0}  }; 

// перед входом в крит секцию потребителя
struct sembuf consumer_P[2] = { 
	{FULL_SEM_NUM, -1, 0},  
	{BIN_SEM_NUM, -1, 0}  }; 

// после выхода из крит секции потребителя
struct sembuf consumer_V[2] = { 
	{BIN_SEM_NUM, 1, 0},    
	{EMPTY_SEM_NUM, 1, 0} }; 

// процесс-потребитель
// выбирает из буфера единичный объект
int consume(int i, int sem_fd)
{
	while(!(*stop_consuming))
	{
		// ожидание заполнения хотя бы одной ячейки буфера и
		// ожидание выхода из крит секции другого процесса
		semop(sem_fd, consumer_P, 2); 

		// потребление единичного объекта
		char let = buf[*consume_pos];
		printf("Consumer № %d consumed object on position %d: %c\n", i, *consume_pos, let);
		(*consume_pos) = ((*consume_pos) + 1) % bufsize;

		// освобождение критической секции и
		// инкремент количества пустых ячеек 
		semop(sem_fd, consumer_V, 2); 

		sleep(rand() % 3);

		// как только хотя бы один из процессов обнаруживает букву z, 
		// все процессы потребители завершаются 
		if (let == 'z') 
            (*stop_consuming) = 1;
	}

	return 0;
}

// процесс-производитель
// производит единичный объект и помещает его в буфер 
int produce(int i, int sem_fd)
{
	while(*letter <= 'z')
	{
		// ожидание, пока освободится хотя бы одна ячейка буфера и
		// ожидание выхода из крит секции другого процесса
		semop(sem_fd, producer_P, 2); 

		// производство единичного объекта
		buf[*produce_pos] = *letter;
		printf("\t\t\t\t\tProducer № %d produced object on position %d: %c\n", i, *produce_pos, buf[*produce_pos]);
		
		(*produce_pos) = ((*produce_pos) + 1) % bufsize;
		(*letter)++;

		// освобождение критической секции и
		// инкремент количества заполненных ячеек
		semop(sem_fd, producer_V, 2); 

		sleep(rand() % 6);
	}

	return 0;	
}


int create_shared_memory(int *shm_fd)
{
	// shmget() ; создает разделяемый сегмент
	size_t size = 3 * sizeof(int) + sizeof(char) * (bufsize + 1);
	*shm_fd = shmget(SHM_ID, size, FLAGS);
	if (*shm_fd == -1)
	{
		printf("Error in shmget\n");
		return -1;
	}

	// shmat() attach получение указателя на разделяемый сегмен
	// On success, shmat() returns the address of the attached shared memory
    // segment; on error, (void *) -1 is returned
	produce_pos = (int*) shmat(*shm_fd, 0, 0); 
	if (produce_pos == (int*) -1)
	{
		printf("Error in shmat\n");
		return -1;
	}
	else
	{
		consume_pos = (produce_pos + sizeof(int));
		stop_consuming = (consume_pos + sizeof(int));
		letter = (char *)(stop_consuming + sizeof(int));
		buf = letter + sizeof(char);

		*letter = 'a';
		*produce_pos = 0;
		*consume_pos = 0;
		*stop_consuming = 0;
	}

	return 0;
}

int create_semaphores(int *sem_fd)
{
	// semget(); создаёт набор семафоров
	if ((*sem_fd = semget(SEM_ID, 3, FLAGS)) == -1)
	{
		printf("Error in semget\n");
		return 1;
	}

	//  int semctl(int semid, int semnum, int cmd, ...);
	// performs the control operation specified by cmd on the semnum-th
    // the semaphore set identified by semid
	semctl(*sem_fd, 0, SETVAL, 0); // установить количество занятых ячеек буфера 0
	semctl(*sem_fd, 1, SETVAL, bufsize);  // установить количество пустых ячеек bufsize == все 
	semctl(*sem_fd, 2, SETVAL, 1); // критическая секция свободна - установить бинарный семафор в 1

	return 0;
}

int main()
{
	// создание 
	int shm_fd; 
	if (create_shared_memory(&shm_fd) == -1) // сообщения об ошибках выводятся внутри функции
		return -1;

	// создание набора семафоров
	int sem_fd; 
	if (create_semaphores(&sem_fd) == -1) // сообщения об ошибках выводятся внутри функции
		return -1;
	
	for (int i = 0; i < PRODS; i++) 
    {
        int pid;
        if ((pid = fork()) == -1) {
            printf("Can't fork producer");
            exit(1);
        }

        if (pid == 0) 
        {
            // код процесса потомка
            produce(i+1, sem_fd);
            return 0;
        }
    }

    for (int i = 0; i < CONS; i++) 
    {
        int pid;
        if ((pid = fork()) == -1) {
            printf("Can't fork consumer");
            exit(1);
        }

        if (pid == 0) 
        {
            // код процесса потомка
            consume(i+1, sem_fd);
            return 0;
        }
    }

    for (int i = 0; i < PRODS + CONS; i++) 
    {
        // процесс предок дожидается завершения процессов потомков
		int status;
        wait(&status);
    }

    // после чего сегмент может быть отделен от адр пространства процесса
    if (shmdt(produce_pos) == -1) 
    {
    	printf("Error in shmdt\n");
    	return -1;
    }

	return 0;
}

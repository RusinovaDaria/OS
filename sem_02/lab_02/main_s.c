#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

typedef struct
{
    // короткое имя 
    char *filename;

    // указатель на каталог, 
    // в котором находится файл 
    // зачем его хранить: чтобы закрыть 
    // каталоги, когда мы будем из них подниматься
    DIR *curr_dir;
    
} path_t;

typedef struct node
{
    struct node *next_node;
    path_t *data;
} node_t;

typedef node_t *stack_t;

path_t *pop_stack(stack_t *stack)
{
    stack_t p = *stack;

    if (p == NULL)
        return NULL;

    stack_t *data = p->data;
    *stack = p->next_node;
    free(p);

    return data;
}

// стек растет с головы (пуш в голову списка)
int push_stack(stack_t *stack, DIR *dp, const char *filename)
{
    path_t *data = (path_t *)malloc(sizeof(path_t)); 
    if (data == NULL)
    {
        printf("Error - malloc.");
        return -1;
    }

    stack_t head = (stack_t)malloc(sizeof(node_t));
    if (head != NULL)
    {
        data->filename = filename;
        data->curr_dir = dp;
        head->data = data;
        head->next_node = *stack;
        *stack = head;

        return 0;
    }

    return -1;
}

/*
struct stat {
    dev_t         st_dev;       устройство 
    ino_t         st_ino;       inode 
    mode_t        st_mode;      режим доступа 
    nlink_t       st_nlink;     количество жестких ссылок 
    uid_t         st_uid;       идентификатор пользователя-влдельца 
    gid_t         st_gid;       идентификатор группы-владельца 
    dev_t         st_rdev;      тип устройства 
                                (если это устройство) 
    off_t         st_size;      общий размер в байтах 
    blksize_t     st_blksize;   размер блока ввода-вывода 
                                в файловой системе 
    blkcnt_t      st_blocks;    количество выделенных блоков 
    time_t        st_atime;     время последнего доступа 
    time_t        st_mtime;     время последней модификации 
    time_t        st_ctime;     время последнего изменения 
};
*/

int tree_traversal(const char *filename)
{
    // инициализация пустого стека
    stack_t stack = NULL; 
    int stack_size = 0;

    struct stat statbuf;       // структура с информацией о файле bk 
    struct dirent * dirp;      // тут номер индексного узла и строка имени файла
                               // номер узла - уникальный идентификатор файла, 
                               // имя - для удобства работы пользователя 
    DIR* dp;                   // указатель на каталог (на поток каталога)
                               
    /* chdir изменяет текущий каталог каталог на filename.
    CHDIR здесь добавлен - каждый раз когда мы переходим 
    в новый катлог, устанавливаем его рабочим, чтобы в lstat передатвать
    короткое имя файла, а не полный путь 
    */
    // chdir следует по символической ссылке
    // для каждого процесса- текущий рабочий каталог 
    // относительно этого каталога вычисляют относи-е пути 

    // gередавать функции lstat не полный путь к файлу а только 
    // его короткое имя (chdir дб после обработки всех файлов в каталоге)
    // это влияет на время работы - c chdir быстрее
    if (chdir(filename) == -1){
		printf("Chdir error occured.\n");
		exit(-1);
	}

	// lstat - возвращает структуру (statbuf) 
    // с информацией о символической ссылке 
    // filename, которая передается в параметры
    // LSTAT не следует по символической ссылке -> 
    // не может изменить сам файл                               
    if (lstat(".", &statbuf) < 0)
    {
        printf("Error - stat\n");
        exit(-1);
    }

	// проверка на то, является ли каталогом
    if (S_ISDIR(statbuf.st_mode) == 0)
    {
        printf("passed filename is not a directory\n");
        printf("|----> %s\n", filename);
        exit(0);
    }

	//Функция opendir() открывает поток каталога и возвращает указатель на структуру типа DIR, 
	//которая содержит информацию о каталоге. 
    if ((dp = opendir(".")) == NULL)
    {
        printf("Error - opendir: %s\n", filename);
        exit(-1);
    }

    // выводим имя корневого каталога 
    printf("root curr_dir : %s\n", filename);
    printf("--------------------------\n");

    // создаем новый узел
    push_stack(&stack, dp, filename);
    stack_size++;

    // Пока стек не пуст
    while (stack_size > 0)
    {
        /* Функция readdir() возвращает указатель 
        на следующую запись каталога в структуре dirent, 
        прочитанную из потока каталога dp*/
        while (dirp = readdir(stack->data->curr_dir) != NULL)
        {
            /* каждый каталог содержит записи "." (указатель на сам каталог) 
            и ".." (указатель на родительский каталог) */

            // здесь каталоги "." и ".." пропускаются - это текущий и супер-каталог
            // -> не выводим о них информацию (она уже была выведена)
                     
            if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, ".."))
            {
                // lstat - возвращает структуру (statbuf) 
                // с информацией о символической ссылке 
                // LSTAT не следует по символической ссылке -> 
                // не может изменить сам файл -> не нужны права
                if (lstat(dirp->d_name, &statbuf) < 0)
                {
                    printf("Error occured - lstat.");
                    exit(-1);
                }

                // если обнаружен каталог - делаем его рабочей директорией 
                // делаем push
                if (S_ISDIR(statbuf.st_mode) == 1)
                {
                    if (chdir(dirp->d_name) < 0)
                    {
                        printf("Error occured - chdir.");
                        exit(-1);
                    }

                    // Открываем директорию
                    if ((dp = opendir(".")) == NULL)
                    {
                        printf("Error occured - opendir.");
                    }

                    for (int i = 0; i < stack_size; ++i)
                        printf("           ");
                    printf("|----> dir: %s\n", dirp->d_name);
                    stack_size++;

                    // ПУШ В СТЕК ЕСЛИ НАШЛИ ДИРЕКТОРИЮ
                    push_stack(&stack, dp, dirp->d_name);
                }    
                else
                {
                    for (int i = 0; i < stack_size; ++i)
                        printf("           ");
                    printf("|----> %s\n", dirp->d_name);
                }             
            }
        }

        // изымаем из стека элемент если 
        // readdir вернул NULL 
        // то есть все записи из текущего 
        // каталога были получены 
        stack_size--;
        
        // закрываем поток каталога
        if (closedir( pop_stack(&stack)->curr_dir ) < 0)
        {
            printf("Closedir error occured.");
            return -1;
        }

        /* т.к. мы спускались в каталог, 
        необходимо подняться на уровень выше */
        if (chdir("..") == -1)
        {
	    	printf("Chdir error occured.");
	    	return -1;
	    }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Use ./main <base_dir>\n");
        return -1;
    }

    return tree_traversal(argv[1]);
}

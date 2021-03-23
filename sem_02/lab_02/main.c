#include <stdio.h>
#include <string.h>
#include <dirent.h> // opendir
#include <unistd.h> // chdir
#include <sys/types.h>
#include <sys/stat.h> // lstat

#define MAX_DEPTH 5

#define OK 0
#define CHDIR_ERROR -1
#define LSTAT_ERROR -2
#define OPENDIR_ERROR -3
#define CLOSEDIR_ERROR -3
#define MAX_DEPTH_ACHIEVED -5
#define MARGIN "           "

/*
вывести на экран дерево каталогов (листинг 4.7)
убрать все goto, continue
добавить chdir (задание 4.11)
зачем: чтобы при переходе в новый каталог 
передавать функции lstat не полный путь к файлу а только 
его имя (chdir дб после обработки всех файлов в каталоге)
это влияет на время работы - c chdir быстрее
*/

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

int tree_traversal(const char *filename, int dpth)
{
    // условие выхода из рекурсии 
    if (dpth > MAX_DEPTH)
    {
		perror("Max depth of recursion achieved.");
		return MAX_DEPTH_ACHIEVED;
	}

    int ret = OK;              // return value 
    struct stat statbuf;       // структура с информацией о файле 
    struct dirent * dirp;      // тут номер индексного узла и строка имени файла
    DIR* dp;                   // указатель на каталог (на поток каталога)

    // lstat - возвращает структуру (statbuf) 
    // с информацией о символической ссылке 
    // filename, которая передается в параметры
    // LSTAT не следует по символической ссылке -> 
    // не может изменить сам файл
    if (lstat(filename, &statbuf) < 0) // ошибка вызова функции lstat
    {  
        printf("Lstat error occured.\n");
        return LSTAT_ERROR;
    }

    // глубина погружения == количество отсутпов
    for (int i = 0; i < dpth; ++i) 
        printf(MARGIN);

    // S_ISDIR - макроопределение - является ли файл каталогом 
    // st_mode - тип файла и режим (права доступа)
    if (S_ISDIR(statbuf.st_mode) == 0) // НЕ директория -> лист в дереве 
    {
        printf("|----> %s\n", filename);
        return OK;
        // вывели имя файла и выходим, т.к. погружаться 
        // больше никуда не надо 
    }

    /* filename - это каталог */
    /* файл каталога - содержит имена других файлов 
        и ссылки на информацию о них -> открываем директорию 
        и спускаемся в нее */
    printf("|----> dir : %s\n", filename);

    // спускаемся в каталог 
    // Функция opendir() открывает поток каталога <filename>, 
    // и возвращает указатель на этот поток (или NULL в случае ошибок).
    // Поток устанавливается на первой записи в каталоге.
    if ((dp = opendir(filename)) == NULL) 
    {
        printf("Opendir error occured.\n");
        return OPENDIR_ERROR; // каталог недоступен
    }

    /* chdir изменяет текущий каталог каталог на filename.
    CHDIR здесь добавлен - каждый раз когда мы переходим 
    в новый катлог, устанавливаем его рабочим, чтобы в lstat передатвать
    короткое имя файла, а не полный путь 
    */
    // chdir следует по символической ссылке
    // для каждого процесса- текущий рабочий каталог 
    // относительно этого каталога вычисляют относи-е пути 
    if (chdir(filename) == -1){
		printf("Chdir error occured.\n");
		return CHDIR_ERROR;
	}

    // tree_traversal возвращает код завершения уровня рекурсии
    // если tree_traversal вернул не OK -> либо ошибка, либо глубина рекурсии достингута 
    while ((ret == OK) && ((dirp = readdir(dp)) != NULL))
    {
        /* Функция readdir() возвращает указатель 
        на следующую запись каталога в структуре dirent, 
        прочитанную из потока каталога dp*/
        if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, ".."))
            ret = tree_traversal(dirp->d_name, dpth + 1);

        // здесь каталоги "." и ".." пропускаются - это текущий и супер-каталог
        // -> не выводим о них информацию (она уже была выведена)

        /* каждый каталог содержит записи "." (указатель на сам каталог) 
        и ".." (указатель на родительский каталог) */
    } 

    /* т.к. мы спускались в каталог, 
    необходимо подняться на уровень выше */
    if (chdir("..") == -1){
		perror("Chdir error occured.");
		return CHDIR_ERROR;
	}

    // закрываем поток каталога
    if (closedir(dp) < 0)
    {
        perror("Closedir error occured.");
        return CLOSEDIR_ERROR;
    }

    return ret;
}

int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        printf("Use ./main <base_dir>\n");
        return -1;
    }

    // вызов рекурсивной функции tree_traversal, которая выводит дерево 
    // каталогов из каталога argv[1]
    return tree_traversal(argv[1], 0);
}
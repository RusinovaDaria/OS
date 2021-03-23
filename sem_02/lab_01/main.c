#include <syslog.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h> // char * getlogin ( void );
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/wait.h> //wait
#include <pthread.h>

/*
Демон в отдельном потоке выводит 
имя пользователя и время обращения 
в logfile по сигналу sighup.
*/

// ps выводит информацию о процессах в системе 
// -a - процессы других пользователей
// -j - доп сведения 
// -x - вывести процессы, не имеющие упр териминала 

// 0 - процесс, запускаемый ядром 
// 1 - процесс, открывший терминал 

// большинство демонов обладают привелегиями суперпользователя 
// демоны не имеют упр терминала 
// демоны являются лидерами групп и лидерами сессий

// ПРАВИЛА: 
//--------------
// * вызвать unmask, чтобы сбросить маску режима создания файлов 
//   маска режима создания файлов (насл. от запускающего процесса) 
//   уст-ет права доступа на создание файлов - может воспрепятствовать 
//   созданию файла демоном 
//   а у демона должны быть привелегии суперпользователя 

// * вызвать fork и завершить родительский процесс - гарантирует, что дочерний 
//   процесс не является лидером группы для setsid + завершаем команду в командной оболочке

// * setsid - создаем новую сессию, в результате процесс: 
//   * становится лидером сессии
//   * становится лидером группы
//   * лишается упр-го терминала 

// * chdir("/") - сделать корневой каталог текущим рабочим каталогом 

// * закрыть все ненужные файловые дескрипторы - предотвращает удержание 
//   унаследованных (от род процесса) дескрипторов в открытом состоянии
//   getrlimit - определить максимально возможный номер дескриптора и 
//   закрыть все дескрипторы вплоть до него 

// * открыть файловые дескрипторы с номерами 0, 1 и 2 на устройстве /dev/null
//   -> любые стандарные функции ввода/вывода не будут оказывать никакого влияния
//   -> демон не может выводить никакую информацию в терминал


// СОГЛАШЕНИЕ: если демон использует файл блокировки, то этот файл помещается в 
// каталог /var/run/ - чтобы создать файл в этом каталоге, демон должен обладать
// правами суперпользователя. Имя файла - <имя демона>.pid
#define LOCKFILE "/var/run/my_daemon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
sigset_t mask;

// обеспечивает запуск единственного экземпляра демона  
int already_running(void) 
{
    int fd;
    char buf[16];

    fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0) {
        syslog(LOG_ERR, "Failed to open %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    // как работает: каждая копия демона будет пытаться установить 
    // блокировку на файл fd
    // если файл уже заблокирован (т.е. демон уже запущен), 
    // то завершаем процесс
    // int flock(int fd, int operation);  
    // LOCK_EX - установить эксклюзивную блокировку
    // LOCK_UN - Удалить существующую блокировку
    // LOCK_NB - non-blocking 
    // 0 - если успешно 
    // -1 - если неуспешно, в errno - ошибка 
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) 
    {
        if (errno == EWOULDBLOCK)
        {
            syslog(LOG_ERR, "ALREADY RUNNING: Failed to flock %s: %s", LOCKFILE, strerror(errno));
            close(fd);
            return -1;
        }

        syslog(LOG_ERR, "incorrect file descriptor");
        exit(1);
    }

    // усекаем размер файла fd до 0 
    // (чтобы перезапись 12345 на 9999 не привела к 99995)
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long) getpid());
    write(fd, buf, strlen(buf) + 1);
    syslog(LOG_WARNING, "new PID is written to lockfile");

    return 0;
}

// инициализация процесса-демона
// cmd - command - команда командной строки 
// это имя демона
void daemonize(const char *cmd) 
{
    int fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rlim;
    struct sigaction sa;

    // сбросить маску режима создания файла
    // зачем: маска устанавливает права на создание файлов
    // это может воспрепятствовать созданию файла демоном 
    // (демон должен иметь привелегии суперпользователя)
    umask(0);

    // получить максимальный номер дескриптора
    // зачем: чтобы закрыть все дескрипторы вплоть до этого номера
    // чтобы не удерживать унаследованные дескрипторы в открытом состоянии 
    if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) 
    {
        printf("Failed to get max descriptor number\n");
        exit(1);
    }

    // вызвать функцию fork и завершить родительский процесс 
    // зачем: 
    //    1 - чтобы завершить команду в терминале 
    //    2 - гарантируем, что доч. процесс не является лидеров группы 
    //        для вызова setsid
    if ((pid = fork()) < 0) 
    {
        printf("Error: fork\n");
        exit(1);
    } 
    else if (pid != 0) 
    {
        // завершаем род. процесс 
        printf("Parent done\n");
        exit(0);
    }

    // создаем новую сессию вызовом setsid, в результате:
    // 1 - процесс становится лидером новой сессии 
    // 2 - -//- группы
    // 3 - лишается упр терминала 
    if (setsid() == -1) 
    {
        printf("Error: setsid\n");
        exit(1);
    }

    // игнорирование сигнала SIGHUP
    // обеспечивает невозможность обрения управ-го терминала 
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        printf("Failed to set ignorance: SIGHUP\n");
        exit(1);
    }

    // назначаем корневой диалог текущим рабочим 
    // чтобы можно было отмонтировать файловую систему 
    if (chdir("/") < 0) 
    {
        printf("chdir error\n");
        exit(1);
    }

    // закрываем все дескрипторы, чтобы не 
    // удерживать унаследованные д-ры в открытом состоянии
    if (rlim.rlim_max == RLIM_INFINITY) 
        rlim.rlim_max = 1024;
    for (int i = 0; i < rlim.rlim_max; i++) 
        close(i);

    //-------------------------------------------------------------
    // ВОТ С ЭТОГО МОМЕНТА СТД-ЫЕ ФУНКЦИИ ВВОДА-ВЫВОДА НЕ РАБОТАЮТ
    //-------------------------------------------------------------
    // теперь демон может выводит информацию только в файл 

    // присоединить файловые дескрипторы 0,1 и 2 к /dev/null
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    // инициализировать файл журнала 
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "incorrect file descriptors %d %d %d\n", fd0, fd1, fd2);
        exit(1);
    }

    syslog(LOG_WARNING, "DAEMONIZE: daemon %s created \n", cmd);
}

// вывод времени и текущего пользователя в лог (syslog)
void output_user_time()
{
    time_t raw_time;
    struct tm *time_info;
    char buf[70];

    time(&raw_time);
    time_info = localtime(&raw_time);
    
    sprintf(buf, "Daemon is working, user: %s; time: %s", getlogin(), asctime(time_info));
    syslog(LOG_INFO, buf);
}

void *thr_fn(void *arg)
{
    int err, signo;

    // бесконечный цикл 
    for(;;)
    {
        //syslog(LOG_ERR, "THREAD\n"); 

        // асинхронно слушает сигналы
        err = sigwait(&mask, &signo);
        if (err != 0)
        {
            syslog(LOG_ERR, "EXITING: Error: sigwait failed\n");
            exit(1);
        }

        switch (signo)
        {
            // вывод юзера и времени
            case SIGHUP:
                syslog(LOG_INFO, "SIGHUP CAUGHT - OUTPUT USER AND TIME\n");
                output_user_time();
                break;
            
            // sudo kill -15 <pid демона> - посылка сигнала sigterm 
            case SIGTERM:
                syslog(LOG_INFO, "EXITING: Sigterm caught - exit\n");
                exit(0);
            
            default:
                syslog(LOG_INFO, "unexpected signal caught - %d\n", signo);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) 
{
    int err;
    pthread_t tid;    
    char *cmd;
    struct sigaction sa;

    // strrchr(argv[0], '/') ищет последнее вхождение символа / в строку
    if ((cmd = strrchr(argv[0], '/')) == NULL)
        cmd = argv[0];
    else
        cmd++;

    // cmd теперь == main - это команда и это имя демона

    // переход в режим демона 
    daemonize(cmd);

    // убеждаемся в том, что ранее не был запущен др экземпляр демона 
    if (already_running() != 0) {
        syslog(LOG_ERR, "EXITING: Daemon is already running\n");
        syslog(LOG_ERR, "EXITING: New Daemon exitted\n");
        exit(1);
    }

    // устанавливаем обработчик по умолчанию для sighup
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask); // сбрасывает в 0-ли
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) < 0){
        syslog(LOG_ERR, "EXITING: Failed to set SIG_DFL for SIGHUP\n");
        exit(1);
    }

    sigfillset(&mask); // инициализирует 1-цами 
    // применяет операцию ко всем переданным сигналам - блокируем все сигналы
    
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
    {
        syslog(LOG_ERR, "EXITING: Failed to run SIG_BLOCK\n");
        exit(1);
    }

    // создание потока, который будет заниматься обработкой SIGHUP и SIGTERM
    err = pthread_create(&tid, NULL, thr_fn, 0);
    if (err != 0){
        syslog(LOG_ERR, "EXITING: Failed to create thread\n");
        exit(1); 
    }

    //thr_fn(NULL);

    // если процесс завершится, завершится и поток -> 
    // указываем ожидание процессом завершения потока 
    // а в потоке бесконечный цикл 

    time_t raw_time;
    struct tm *time_info;
    char buf[70];
    time(&raw_time);
    time_info = localtime(&raw_time);

    while (1)
    {
        sprintf(buf, "Main thread time: %s", asctime(time_info));
        syslog(LOG_INFO, buf);
        sleep(2);
    }

    pthread_join(tid, NULL);
    exit(0);
}
/*
sudo insmod ./md.ko - загружает модуль в ядро
lsmod - список модулей загруженных в ядро 
modinfo ./md.ko - информация о модулях ядра
sudo rmmod ./md.ko - выгрузить модуль из ядра 
dmesg - вывести буфер сообщений ядра (системный журнал)

системный журнал используется большим числом процессов
для того чтобы было легче обнаружить сообщения конкретного модуля, 
рекомендуется в их начало помещать некоторый идентификатор 
(к примеру знак + или имя модуля)
dmesg | tail -n60 | grep MD 
*/

/*
daria@daria-VirtualBox:/media/sf_OS_bmstu/sem_02/lab_03/task_01$ modinfo ./md.ko
filename:       /media/sf_OS_bmstu/sem_02/lab_03/task_01/./md.ko
author:         Rusinova Daria
license:        GPL
srcversion:     61D3251DE8C94E8D9189BAB
depends:        
retpoline:      Y
name:           md
vermagic:       5.8.0-43-generic SMP mod_unload 
*/

/*
Задание 1
Реализовать загружаемый модуль ядра, 
который при загрузке записывает в системный журнал 
информацию о процессах. О каждом процессе записать: 
* название, идентификатор 
* название предка, идентификатор предка 
                * * * 
Модуль должен собираться при пом6ощи Make-файла. 
Загружаемый модуль должен содержать:
* Указание лицензии GPL
* Указание автора
*/

#include <linux/module.h> // макросы MODULE_<...>
#include <linux/kernel.h> // функции ядра
#include <linux/init.h> // __init и __exit 
#include <linux/sched.h> // task_struct - планировщик 
#include <linux/init_task.h> // next_task()

// чтобы сообщить ядру, под какой лицензией распространяется исходный код модуля. 
// Если лицензия не соотв. GPL то загрузка модуля приведет 
// к установке в ядре флага tainted (испорчено).
MODULE_LICENSE("GPL"); 

// позволяет сообщить ядру автора модуля, только для информационных целей
MODULE_AUTHOR("Rusinova Daria"); 

// __init - действие, на которое в системе надо обратить внимание
// __init используются для обозначения функций, встраиваемых в ядро, 
// указание о том, что функция используется только на этапе инициализации
// и освобождает использованные ресурсы после. 
// Это позволяет экономить ресурсы системы 
// static - т.к. функцию не нужно экспортировать 
static int __init md_init(void) 
{
    // printk - функция ядра (определена в ядре Linux) - выводит переданную строку в var/log/message
    // записывает сообщ в буфер ядра (в системный журнал) 
	printk("! Module is loaded.\n");

    // &init_task - точка входа в кольцевой список task_struct
    // task_struct - дескриптор процесса
	struct task_struct *task = &init_task;
 	do 
	{
        // KERN_INFO - уровень протоколирования - информационные сообщения 
        // ЗАПЯТАЯ НЕ СТАВИТСЯ тк уровень протоколирования - часть 
        // строки форматирования (для экономии памяти стека при вызове функции)
        printk(KERN_INFO "! process: %s - %d, parent: %s - %d", 
            task->comm, task->pid,  // task->comm - символическое имя исполняемого файла в файловой подсистеме 
            task->parent->comm, task->parent->pid);
        // в таск_стракт есть ссылка на файловую подсистему, которой принадлежить процесс

 	} while ((task = next_task(task)) != &init_task); // поскольку список кольцевой 

    // current - указатель на task_struct текущего процесса
 	printk(KERN_INFO "! process: %s - %d, parent: %s - %d", 
        current->comm, current->pid, 
        current->parent->comm, current->parent->pid);

 	return 0;
}

// пропуск функции когда модуль встроен (built) в ядро - built-in drivers
// has no effect for loadable modules
static void __exit md_exit(void) 
{
	printk("! Module is unloaded.\n");
}

// загружаемые модули ядра должны содержать два макроса 
module_init(md_init); // для регистрации функции инициализации модуля - функция md_init будет вызываться при загрузке модуля в ядро
                      // фцнкция иниц для запроса рексурсов и выделения памяти под структуры 

module_exit(md_exit); // для регистрации функции которая вызывается при удалении модуля из ядра
                      // как правило, выполняет освобождение ресурсов 
                      // после завершения этой функции модуль выгружается из ядра 
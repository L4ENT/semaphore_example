#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>



#define MAXSETSIZE 25
#define PROCESES_LIMIT 10
#define N_SEMS 1


void print_time_str(){
    char            fmt[64], buf[64];
    struct timeval  tv;
    struct tm       *tm;

    gettimeofday(&tv, NULL);
    if((tm = localtime(&tv.tv_sec)) != NULL)
    {
        strftime(fmt, sizeof fmt, "%Y-%m-%d %H:%M:%S.%%06u", tm);
        snprintf(buf, sizeof buf, fmt, tv.tv_usec);
        printf("'%s'", buf);
    }
}


int semaphore_open(){
    // Функция создает множесто семафоров
    // В множестве будет только один семафор (нам больше и не нужно)
    return semget(IPC_PRIVATE, N_SEMS, 0666 | IPC_CREAT);
}

void semaphore_remove(int sem_id){
    // Функция удаляет множество семафоров
    int result = semctl (sem_id, 0, IPC_RMID, 0);
    if (result == -1) {
        printf ("\nОшибка удаления семафоров\n");
    } else {
        printf ("\nСемаформы успешно удалены\n");
    }
    printf ("SEM_ID = %d\n", sem_id);
}

void worker_process(int sem_id){
    int self_pid = getpid();
    int sleep_time_seconds = self_pid % 10;

    // Структура для операции
    struct sembuf semaphore_operation;
    semaphore_operation.sem_num = 0;
    semaphore_operation.sem_op = -1;
    semaphore_operation.sem_flg = 0;

    int result = 0;
    do {
        // Пытаемся отнять у семафора единицу
        // Процесс будет ждать
        result = semop(sem_id, &semaphore_operation, N_SEMS);
        print_time_str();
        printf(" Получили какие-то данные и ждем!\n");
        sleep(sleep_time_seconds);
    } while(result != -1);

    exit(EXIT_SUCCESS);
}


int main()
{

    struct sembuf sem_zero_operation;
    sem_zero_operation.sem_num = 0;
    sem_zero_operation.sem_op = 0;
    sem_zero_operation.sem_flg = 0;

    struct sembuf sem_incr_operation;
    sem_incr_operation.sem_num = 0;
    sem_incr_operation.sem_op = 1;
    sem_incr_operation.sem_flg = 0;

    // Создаем семафор
    int semid = semaphore_open();

    // Запускаем дочерние процессы (воркеры)
    for (int i = 0;  i < PROCESES_LIMIT; i++){
        if(fork() == 0){
            worker_process(semid);
        }
    }

    // Раздаем данные
    for (int i = 0;  i < PROCESES_LIMIT; i++){
        // Ждем пока семафор не станет нулевым
        // Это будет значить что кто то прочитал данные
        int result = 0;
        do {
            result = semop(semid, &sem_zero_operation, N_SEMS);
        } while(result == -1);
        print_time_str();
        printf(" Отправили данные\n");
        semop(semid, &sem_incr_operation, N_SEMS);
    }

    // Удаляем семофорр
    semaphore_remove(semid);

    while (wait(NULL) > 0){}

    return EXIT_SUCCESS;
}

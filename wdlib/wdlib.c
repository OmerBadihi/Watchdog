#define _GNU_SOURCE_
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <signal.h>    /*SIG__, sigaction*/
#include <unistd.h>    /*fork, pid_t*/
#include <sys/types.h> /*fork, pid_t*/
#include <stdlib.h>    /*setenv*/
#include <pthread.h>   /*pthread*/
#include <semaphore.h> /*semaphore*/
#include <fcntl.h>     /*O_CREAT*/
#include "wdlib.h"
#include "SHeap.h" /*scheduler based on heap*/

#define UNUSED_PARAM(P) (void)P
#define SEM_NAME "ready"

enum SA_IDX
{
    CHECK_SIGNAL = 0,
    STOP_SIGNAL
};

enum STATUS
{
    FAIL = -1,
    SUCCESS = 0
};

static scheduler_t *scheduler = NULL;
static pid_t next_pid;
static unsigned int is_alive;
static sem_t *sem_ready = NULL;

static void CheckALiveSignalHandler(int signo, siginfo_t *info, void *other);
static void DontRevivalSignalHandler(int signo, siginfo_t *info, void *other);
static void *UpdateScheduler(void *arg);
static int TaskSendSignal(const void *param);
static int TaskCheckIsAlive(const void *param);
static void CleanUp(int choice);

int KeepMeAlive(int argc, char *argv[]) /*MMI*/
{
    struct sigaction action[2] = {0};
    char *argv_path[2] = {"../wd/watchdog.Debug.out", NULL};
    pthread_t id;
    int choice = 0, ch_thread = 0; /*for CleanUp*/
    unique_id_t uid_t1, uid_t2;
    size_t interval_in_sec1 = 1, interval_in_sec5 = 5;

    UNUSED_PARAM(argc);

    do
    {
        /*1. register signal handler*/
        /*update struct sigacion*/
        action[CHECK_SIGNAL].sa_sigaction = CheckALiveSignalHandler;
        sigemptyset(&action[CHECK_SIGNAL].sa_mask);
        action[CHECK_SIGNAL].sa_flags = SA_SIGINFO;

        action[STOP_SIGNAL].sa_sigaction = DontRevivalSignalHandler;
        sigemptyset(&action[STOP_SIGNAL].sa_mask);
        action[STOP_SIGNAL].sa_flags = SA_SIGINFO;

        /*int sigaction - first arg, which signal I want to catch
                            second arg, which function I want to call to handle it */
        if (FAIL == sigaction(SIGUSR1, &action[CHECK_SIGNAL], NULL) ||
            FAIL == sigaction(SIGUSR2, &action[STOP_SIGNAL], NULL))
        {
            break;
        }

        /*2. fork(), exec*/
        if (NULL == getenv("recovery"))
        {
            /*first time*/
            if (FAIL == setenv("recovery", argv_path[0], 1))
            {
                break;
            }

            next_pid = fork();
            if (FAIL == next_pid)
            {
                break;
            }

            if (0 == next_pid) /*child process*/
            {
                if (FAIL == setenv("recovery", argv[0], 1)) /*argv[0] == "./247.Debug.out"*/
                {
                    choice = 1;
                    break;
                }

                if (FAIL == execv(argv_path[0], argv_path))
                {
                    choice = 1;
                    break;
                }
            }
        }

        /*3. Scheduler create*/
        scheduler = SchedulerCreate();
        if (NULL == scheduler)
        {
            break;
        }

        /*4. Add tasks*/
        uid_t1 = SchedulerTaskAdd(scheduler, TaskSendSignal, interval_in_sec1, NULL);
        uid_t2 = SchedulerTaskAdd(scheduler, TaskCheckIsAlive, interval_in_sec5, argv[0]);
        if (UIDIsEqual(uid_null_uid, uid_t1) || UIDIsEqual(uid_null_uid, uid_t2))
        {
            choice = 3;
            break;
        }

        /*5. parent / child sync*/
        sem_ready = sem_open("ready", O_CREAT, 0666, 0);
        if (SEM_FAILED == sem_ready)
        {
            choice = 3;
            break;
        }

        if (next_pid > 0)
        {
            /*waiting for the child to complete init*/
            sem_wait(sem_ready);
        }

        sem_post(sem_ready);
        /* create thread - for the tasks (scheduler) */
        ch_thread = pthread_create(&id, NULL, UpdateScheduler, NULL);
        if (SUCCESS != ch_thread)
        {
            choice = 4;
            break;
        }

        return SUCCESS;
    } while (0);

    CleanUp(choice);

    return FAIL;
}

static void CheckALiveSignalHandler(int signo, siginfo_t *info, void *other)
{
    UNUSED_PARAM(signo);
    UNUSED_PARAM(other);

    next_pid = info->si_pid;
    /* printf("get signal from process # %d\n", next_pid); */
    is_alive = 1;
}

static void DontRevivalSignalHandler(int signo, siginfo_t *info, void *other)
{
    UNUSED_PARAM(signo);
    UNUSED_PARAM(info);
    UNUSED_PARAM(other);

    if (scheduler)
    {
        SchedulerStop(scheduler);
    }
}

static void *UpdateScheduler(void *arg)
{
    UNUSED_PARAM(arg);

    printf("Scheduler is start to run for process # %d\n", getpid());
    SchedulerRun(scheduler);

    CleanUp(4);

    return NULL;
}

static int TaskSendSignal(const void *param)
{
    UNUSED_PARAM(param);

    /* printf("send signal to process # %d\n", next_pid); */
    kill(next_pid, SIGUSR1);

    return 1;
}
static int TaskCheckIsAlive(const void *param)
{
    char *recover_proc = getenv("recovery");
    char *argv_path[2] = {0};

    if (is_alive)
    {
        is_alive = 0;
        /* printf("Pid %d is a live!!\n", next_pid); */
        return 1;
    }
    else
    {
        /*revival the sec process*/
        printf("Pid %d is NOT a live!!\n", next_pid);

        next_pid = fork();
        if (FAIL == next_pid)
        {
            return 1; /*try again in the next check*/
        }
        if (0 == next_pid)
        {
            setenv("recovery", param, 1);
            argv_path[0] = recover_proc;
            execv(argv_path[0], argv_path);
        }
    }

    return 1;
}
void LetMeDie(int argc, char *argv[]) /*DNR*/
{
    UNUSED_PARAM(argc);
    UNUSED_PARAM(argv);

    kill(getpid(), SIGUSR2);
    kill(next_pid, SIGUSR2);
}

static void CleanUp(int choice) /*1.kill(getpid(), SIGTERM);2.kill(next_pid, SIGUSR2);3.SchedulerDestroy(scheduler);*/
{
    switch (choice)
    {
    case 4:
        SchedulerDestroy(scheduler);
        sem_close(sem_ready);
        sem_unlink(SEM_NAME);
        break;
    case 3:
        SchedulerDestroy(scheduler);
        break;
    case 2:
        kill(next_pid, SIGUSR2);
        break;
    case 1:
        kill(getpid(), SIGTERM);
        break;
    }
}
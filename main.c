#include "header.h"
#include "functions.h"
#include "constants.h"
#include "simulationfuncs.h"
#include "msg_queue.h"
#include "plane.h"
#include "comittees.h"

#ifdef __LINUX__
union semun
{
    int val;
    struct semid_ds *buf;
    ushort *array;
};

#endif
// communction with opengl
int n, privatefifo, publicfifo;
struct message msg;

int main()
{
    // get all values
    read_settings_from_a_file("settings.txt");

    /* Open the public FIFO for writing */
    if ((publicfifo = open(PUBLIC, O_WRONLY)) == -1)
    {
        printf("Public error");
        perror(PUBLIC);
        exit(2);
    }

    // required values for planes
    pid_t pid;
    pid_t planes[NUMBER_OF_CARGO_PLANES];

    // creating msg queue for plan-gnd process communication
    key_t key;
    if ((key = ftok(".", PLANE_GND_MSG_KEY)) == -1)
    {
        perror("Key creation error\n");
        exit(70);
    }
    char plane_to_gnd_msgq_id[20];
    sprintf(plane_to_gnd_msgq_id, "%d", create_MQ(key));

    // creating planes
    for (int i = 0; i < NUMBER_OF_CARGO_PLANES; i++)
    {
        if ((pid = fork()) == -1)
        {
            perror("Fork Error\n");
            exit(1);
        }
        else if (pid == 0)
        {
            // child - plane
            char plane_id[20], number_of_containers[20], drop_time[20], refill_period_time[20];
            // fill values to be passed
            sprintf(number_of_containers, "%d", get_random_number_in_range(getpid(), MIN_FLOUR_CONTAINERS, MAX_FLOUR_CONTAINERS));

            sprintf(drop_time, "%d", get_random_number_in_range(getpid(), MIN_DROP_TIME, MAX_DROP_TIME));

            sprintf(refill_period_time, "%d", get_random_number_in_range(getpid(), REFILL_PERIOD_MIN, REFILL_PERIOD_MAX));

            sprintf(plane_id, "%d", (i + 1));

            // message for opengl ( plane created (p))

            sprintf(msg.cmd_line, "p,%d,%d,%d,%d", (i), atoi(number_of_containers), atoi(refill_period_time), atoi(drop_time));
            // send infomations to opengl about hte plane
            write(publicfifo, (char *)&msg, sizeof(msg));
            printf("Message send:%s\n", msg.cmd_line);

            // exec plane file and passing arguments
            execlp("./plane", "plane", plane_id, number_of_containers,
                   drop_time, refill_period_time, plane_to_gnd_msgq_id, NULL);
            // send to opengl to show plane

            // will not reach here if success exec
            perror("EXEC ERROR\n");
            exit(5);
        }
        else
        {
            // parent

            // save planes id
            planes[i] = pid;
        }
    }

    // create msg queue for gnd_to_collecting commu
    key_t key_gnd_collect;
    if ((key_gnd_collect = ftok(".", GND_TO_COLLECTER_KEY)) == -1)
    {
        perror("Key creation error\n");
        exit(70);
    }
    char gnd_to_collect_msgq_id[20];
    sprintf(gnd_to_collect_msgq_id, "%d", create_MQ(key_gnd_collect));
    pid_t gnd_pid;

    // create ground process
    if ((pid = fork()) == -1)
    {
        perror("Fork of Ground Error\n");
        exit(5);
    }
    else if (pid == 0)
    {
        // child - ground process
        execlp("./ground", "ground", plane_to_gnd_msgq_id, gnd_to_collect_msgq_id, NULL);
        // will not reach here if success exec
        perror("EXEC ERROR 1\n");
        exit(5);
    }
    else
    {
        // parent
        gnd_pid = pid;
    }

    // creating save storage shared memory between collecting and splitting
    // required data fields
    struct SAVE_STORAGE_SHMEM memory;
    static ushort start_val[2] = {1, 0};
    int semid, shmid;
    char *shmptr;
    union semun arg;
    key_t key_sv = ftok(".", SAVE_STORAGE_KEY);
    char key_passed[20];
    sprintf(key_passed, "%d", key_sv);

    struct SAVE_STORAGE_SHMEM *memptr;
    /*
     * Create, attach and initialize the memory segment
     */
    if ((shmid = shmget(key_sv, sizeof(memory),
                        IPC_CREAT | 0777)) != -1)
    {

        if ((shmptr = (char *)shmat(shmid, 0, 0)) == (char *)-1)
        {
            perror("shmptr -- parent -- attach");
            exit(1);
        }
        memcpy(shmptr, (char *)&memory, sizeof(memory));
        memptr = (struct SAVE_STORAGE_SHMEM *)shmptr;
        memptr->amount = 0; // initialize
    }
    else
    {
        perror("shmid -- parent -- creation");
        exit(2);
    }

    /*
     * Create and initialize the semaphores
     */
    if ((semid = semget(key_sv, 2,
                        IPC_CREAT | 0777)) != -1)
    {
        arg.array = start_val;

        if (semctl(semid, 0, SETALL, arg) == -1)
        {
            perror("semctl -- parent -- initialization");
            exit(3);
        }
    }
    else
    {
        perror("semget -- parent -- creation");
        exit(4);
    }

    // required values for creating committees
    pid_t collecting_committees[COLLECTING_RELIEF_COMMITTEES],
        splitting_committees, distributing_committees;

    for (int i = 0; i < COLLECTING_RELIEF_COMMITTEES; i++)
    {
        if ((pid = fork()) == -1)
        {
            perror("Fork Error\n");
            exit(1);
        }
        else if (pid == 0)
        {
            // child - collecting committee
            // required values
            char number_of_workers[20], collecting_com_id[20];
            sprintf(number_of_workers, "%d", COLLECTING_RELIEF_WORKERS);
            sprintf(collecting_com_id, "%d", (i + 1));

            // send informations to opengl about collecting commieties
            //  (o)-- > creating collecting commities

            sprintf(msg.cmd_line, "o,%d,%d", i, atoi(number_of_workers));
            write(publicfifo, (char *)&msg, sizeof(msg));
            execlp("./collecting", "collecting", collecting_com_id, number_of_workers,
                   gnd_to_collect_msgq_id, key_passed,
                   NULL);

            // will not reach here if success exec
            perror("EXEC ERROR\n");
            exit(6);
        }
        else
        {
            // parent

            // save planes id
            collecting_committees[i] = pid;
        }
    }

    // creating save storage shared memory between collecting and splitting
    // required data fields
    struct BAGS bags_memory;
    int bags_semid, bags_shmid;
    char *bags_shmptr;
    union semun bags_arg;
    key_t key_bags = ftok(".", BAGS_SHMEM_KEY);
    char bags_key_passed[20];
    sprintf(bags_key_passed, "%d", key_bags);

    struct BAGS *bags_memptr;
    /*
     * Create, attach and initialize the memory segment
     */
    if ((bags_shmid = shmget(key_bags, sizeof(bags_memory),
                             IPC_CREAT | 0777)) != -1)
    {

        if ((bags_shmptr = (char *)shmat(bags_shmid, 0, 0)) == (char *)-1)
        {
            perror("shmptr -- parent -- attach");
            exit(1);
        }
        memcpy(bags_shmptr, (char *)&bags_memory, sizeof(bags_memory));
        bags_memptr = (struct BAGS *)bags_shmptr;
        bags_memptr->number_of_bags = 0; // initialize
    }
    else
    {
        perror("shmid -- parent -- creation");
        exit(2);
    }

    /*
     * Create and initialize the semaphores
     */
    if ((bags_semid = semget(key_bags, 2,
                             IPC_CREAT | 0777)) != -1)
    {
        bags_arg.array = start_val;

        if (semctl(bags_semid, 0, SETALL, bags_arg) == -1)
        {
            perror("semctl -- parent -- initialization");
            exit(3);
        }
    }
    else
    {
        perror("semget -- parent -- creation");
        exit(4);
    }

    // creating splitting relief committee
    if ((pid = fork()) == -1)
    {
        perror("Fork Error\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child - splitting
        // required values
        char number_of_workers[20], splitting_com_id[20];
        sprintf(number_of_workers, "%d", SPLITTING_RELIEF_WORKERS);
        sprintf(splitting_com_id, "%d", (1));

        // send number to of workers to opengl
        // send (T) to opengl to increase number of splitting workers
        sprintf(msg.cmd_line, "T,%d,%d", atoi(number_of_workers), atoi(splitting_com_id));
        write(publicfifo, (char *)&msg, sizeof(msg));

        execlp("./splitting", "splitting", splitting_com_id, number_of_workers,
               key_passed, bags_key_passed, NULL);
        // will not reach here if success exec
        perror("EXEC ERROR\n");
        exit(5);
    }
    else
    {
        // parent
        splitting_committees = pid;
    }

    if ((pid = fork()) == -1)
    {
        perror("Fork Error\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child - dist com
        // required values
        char number_of_workers[20], distributing_com_id[20];
        sprintf(number_of_workers, "%d", DISTRIBUTING_RELIEF_WORKERS);
        sprintf(distributing_com_id, "%d", (1));

        // send distruting commitie information to opengl
        // send (D) to opengl to send distribution commitie information
        sprintf(msg.cmd_line, "D,%d,%d", atoi(number_of_workers), distributing_com_id);
        write(publicfifo, (char *)&msg, sizeof(msg));

        execlp("./distributing", "distributing", distributing_com_id, number_of_workers,
               bags_key_passed, NULL);

        // will not reach here if success exec
        perror("EXEC ERROR\n");
        exit(5);
    }
    else
    {
        // parent

        // save planes id
        distributing_committees = pid;
    }
    /*

    pid_t family;
    // pid_t pid;   //////////////////////////////////////////////// big note remove the line
    //  fork families
    if ((pid = fork()) == -1)
    {
        perror("Family Process Fork Error\n");
        exit(25);
    }
    else if (pid == 0)
    {
        // child - family
        execlp("./family", "family", NULL);

        // will not reach here
        perror("EXEC ERROR\n");
        exit(5);
    }
    else
    {
        // parent
        family = pid;
    }
    */
    sleep(10);
    exit(0);
}

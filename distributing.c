#include "header.h"
#include "functions.h" 
#include "constants.h"
#include "simulationfuncs.h"
#include "comittees.h"


struct DISTRIBUTING our_comm;

int main(int argc, char **argv){
    if(argc != 4){
        perror("our_comm Arguments Error\n");
        exit(10);
    }

    our_comm.committee_id = atoi(argv[1]);
    our_comm.number_of_workers = atoi(argv[2]);
    // creating array of workers
    our_comm.workers = (struct WORKER *) malloc(our_comm.number_of_workers * sizeof(struct WORKER));

      // for to add energy to workers
    for (int i=0;i<our_comm.number_of_workers;i++){
        our_comm.workers[i].energy=get_random_number_in_range(getpid(),50,100);
    }

    // required values for bags shmem
    int            bags_semid, bags_shmid;
    char          *bags_shmptr;
    struct BAGS   *bags_memptr;
    key_t bags_key = atoi(argv[3]);
    int bags_to_get = our_comm.number_of_workers * 20;
    int distribution_amount=0;

    while (1)
    {   
        /*
        * Access, attach and reference the shared memory
        */
        if ( (bags_shmid = shmget(bags_key, 0, 0)) != -1 ) {
            if ( (bags_shmptr = (char *) shmat(bags_shmid, (char *)0, 0)) == (char *) -1 ) {
            perror("shmat -- producer -- attach (Producer)");
            exit(1);
            }
            bags_memptr = (struct BAGS *) bags_shmptr;
        }
        else {
            perror("shmget -- producer -- access (producer get)");
            exit(2);
        }
        
        /*
        * Access the semaphore set
        */
        if ( (bags_semid = semget(bags_key, 2, 0)) == -1 ) {
            perror("semget -- producer -- access");
            exit(3);
        }

        // Acquire
        acquire.sem_num =DIST_CONS;
        if ( semop(bags_semid, &acquire, 1) == -1 ) {
            perror("semop -- producer -- acquire");
            exit(4);
        }
    //critical section
        if(bags_memptr->number_of_bags> bags_to_get){
            bags_memptr->number_of_bags -= bags_to_get;
            distribution_amount = bags_to_get;   
        }else{
            distribution_amount = bags_memptr->number_of_bags;
            bags_memptr->number_of_bags = 0;
        }
        printf("[Bags]: Total Amount = %d\n", bags_memptr->number_of_bags);
        fflush(stdout);
        printf("[Distribution]: Total Amount to dist= %d\n", distribution_amount);
        fflush(stdout);

        //release
        release.sem_num = SPLITTING_PROD;
        if ( semop(bags_semid, &release, 1) == -1 ) {
            perror("semop -- producer -- release");
            exit(5);
        }

        




    }





    exit(0);

    // read from dist shmem --> families


}
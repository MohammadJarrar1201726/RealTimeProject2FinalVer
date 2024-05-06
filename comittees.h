#ifndef __COLLECT_H_
#define __COLLECT_H_

#define GND_TO_COLLECTER_KEY 'J'
#define SAVE_STORAGE_KEY 'S'
#define BAGS_SHMEM_KEY 'B'

#include "plane.h"
#include <sys/sem.h>
struct sembuf acquire = {0, -1, SEM_UNDO},
              release = {0, 1, SEM_UNDO};

enum
{
    COLLECTING_PROD,
    SPLITTING_CONS
};
enum
{
    SPLITTING_PROD,
    DIST_CONS
};

struct SAVE_STORAGE_SHMEM
{
    int amount;
} save_storage_shmem;

struct GND_TO_COLLECT_MSG
{
    long msg_type;
    struct CONTAINER container;
} gnd_to_collect_msg;

struct WORKER
{
    int energy;
    char *type;
    float color[3];
} worker;

struct COLLECTING
{
    int committee_id;
    int number_of_workers;
    struct WORKER *workers;
    float color[3];
} collecting;

struct SPLITTING
{
    int committee_id;
    int number_of_workers;
    struct WORKER *workers;

} splitting;

struct DISTRIBUTING
{
    int committee_id;
    int number_of_workers;
    struct WORKER *workers;

} distributing;

struct BAGS
{
    int number_of_bags;
} bags;

#endif

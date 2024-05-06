#include "family.h"
#include "header.h"
#include "functions.h" 
#include "constants.h"
#include "simulationfuncs.h"
#include "comittees.h"


// note add id to family struct  ////////////////////////////////////////////////
int main(int argc, char** argv){
    // families need wheat flour
    struct FAMILY families[NUMBER_OF_FAMILIES];

    // initialize values
    initialize_families(families);

    if(argc != 1){
        perror("Family Argument Error\n");
        exit(66);
    }

    int amount_to_dist ;

    while (1)
    { amount_to_dist = 50;
        // get amount of bags from shmem 

        // sorting families on starvation rate
        sort_families_on_starvation_rate_desc(families, NUMBER_OF_FAMILIES);
        for(int i=0; i<NUMBER_OF_FAMILIES; i++){  
            if (amount_to_dist ){
                if(!families[i].is_dead){
                    printf("family %d with staravtion %d  take bags new amount %d \n",i+1,families[i].starvation_rate,amount_to_dist);
                    families[i].starvation_rate -=10;
                    amount_to_dist -= 10;

                }
            }else{
                break;
            }
        }
        sleep(3);
        increase_starvation_and_check_dead(families);

    }
    

}

void initialize_families(struct FAMILY* families){
    for(int i=0; i<NUMBER_OF_FAMILIES; i++){
        families[i].is_dead = 0;
        families[i].starvation_rate = get_random_number_in_range(getpid(), 50, 81);
    }
}

// Comparison function for qsort
int compare_families(const void *a, const void *b) {
    struct FAMILY *family1 = (struct FAMILY *)a;
    struct FAMILY *family2 = (struct FAMILY *)b;
    return (family2->starvation_rate - family1->starvation_rate);
}

// Function to sort families based on starvation rate in descending order
void sort_families_on_starvation_rate_desc(struct FAMILY *families, int num_families) {
    qsort(families, num_families, sizeof(struct FAMILY), compare_families);
}

void increase_starvation_and_check_dead(struct FAMILY* families){
    for(int i=0;i<NUMBER_OF_FAMILIES;i++){
        if(!families[i].is_dead){
            families[i].starvation_rate +=10;
            if (families[i].starvation_rate >= 90 )
            {
                families[i].is_dead = 1;
                printf("family %d is dead \n",i+1);
            }
        }
    }
}
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <map>
#include <unistd.h>
#include <string.h>
#include <sstream>

using namespace std;

//semaphore
sem_t sem;

//mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//Buffer for items
map<int,int> item;

int count = 0;//consumed count
int prod_count = 0;//produced count

//struct for consumer thread
struct cons_data {
	int b_cap;//buffer max size
	int n_items;//number of items
	int n_cons;//number of consumer threads
};

//Produce--------------------------------------------------------------
void* produce(void *arg){
	int buf_cap;//holds semaphore value
	int thrd_wrk = (int) arg;//number of items thread needs to proudce
		
		//producing logic
		while(thrd_wrk != 0){
			//lock critical section of code		
			if(pthread_mutex_lock(&lock) != 0){
					fprintf(stderr, "error: Mutex lock fail");
			}
			//get value of semaphore to see if buffer has room
			if(sem_getvalue(&sem, &buf_cap) != 0){
					fprintf(stderr, "error: sem_getValue fail");
			}
			if(buf_cap != 0){
				//buffer has room start producing
				if(usleep(rand() % 400 + 300) != 0){
						fprintf(stderr, "error: usleep fail");
				}
				
				//items key value is its item id number
				item[prod_count] = rand() % 700 + 200;
				
				//item consumed increment produced count
				prod_count++;
				
				//decrement semaphore/buffer 
				if(sem_wait(&sem) != 0){
					fprintf(stderr, "error: sem_wait fail");
				}
				
				//reevaluate buffer
				if(sem_getvalue(&sem, &buf_cap) != 0){
					fprintf(stderr, "error: sem_getvalue fail");
				}
				
				//thread has produced one product
				thrd_wrk--;
				}
			//unlock mutex
			if(pthread_mutex_unlock(&lock) != 0){
				fprintf(stderr, "error: mutex unlock fail" );
			}
		}
}
//Consume -------------------------------------------------------------
void* consume(void *arg){
	//Pass in struct. Which is data for consumer threads, num of items and buff size.
	struct cons_data* consD = (struct cons_data*) arg;
	int num_items = consD->n_items;//num of items to consume
	int buff_cap = consD->b_cap;//buffer capacity
	int num_con = consD->n_cons;//number of comsumer threads

	int thrd_id;//thread id to determine what thread is consuming
	int semValue;//used for getting value of semaphore
	
	//logic for cosuming.
	while(1){	
		//lock critical section of code
		if(pthread_mutex_lock(&lock) != 0){
			fprintf(stderr, "error: mutex lock fail");
		}
		
		//if all items have been consumed, allow threads to exit.
		if(count >= num_items){
			if(pthread_mutex_unlock(&lock) != 0){
				fprintf(stderr, "error: mutex unlock fail");
			}
			break;
		}
		
		//if semValue is less than buff size, consume
		if(sem_getvalue(&sem, &semValue) != 0){
			fprintf(stderr, "error: sem_getvalue fail");
		}
		if(semValue < buff_cap){
			//give thread an id to identify thread work on output
			if(count < num_con){
				thrd_id = count;
				
			}
			//consume-----
			cout << thrd_id << ":" << " consuming - " << count << endl;
			//consuming time based on info from buffer
			if(usleep(item[count]) != 0){
				fprintf(stderr, "error: usleep fail");
			}
			//erase item from buffer
			item.erase(item[count]);
			
			//new semaphore value now that product has been consumed
			sem_getvalue(&sem, &semValue);
			
			//consumed an item, increment buffer.
			if(sem_post(&sem) != 0){
				fprintf(stderr, "error: sem_post fail");
			}		
			//total items consumed, increment by 1
			count++;	
		}
		//unlock mutex
		if(pthread_mutex_unlock(&lock) != 0){
			fprintf(stderr, "error: mutex unlock fail");
		}
	}
}
//---------------------------------------------------------------------
int main(int argc, char **argv) {
	//command line vars
	int num_prod;//number to be produced
	int num_cons;//number to be comsumed
	int buf_size;//buffer max size
	int num_items;//number of items
	
	//vars for dividing up thread work
	int min1;//minus 1
	int rmd;//remainder
	int div;//divide

	num_prod = strtol(argv[1], NULL, 10);//number of producer threads
	num_cons = strtol(argv[2], NULL, 10);//number of consumer threads
	buf_size = strtol(argv[3], NULL, 10);//max size of buffer
	num_items = strtol(argv[4], NULL, 10);//number of items to be produced and consumed
	
	//struct to send data to consumer threads
	struct cons_data *consD = (struct cons_data*) malloc(sizeof(struct cons_data));
	if(consD == NULL){
		fprintf(stderr, "error: malloc fail");
	}
	
	//fill struct with command line data
	consD->b_cap = buf_size;
	consD->n_items = num_items;
	consD->n_cons = num_cons;

	//check num args from command line
	if(argc != 5){
		fprintf(stderr, "usage: ./hw1 num_prod num_cons buf_size num_items\n");
		exit(1);
	}
	
	//initialize semaphore
	sem_init(&sem,0,buf_size);

	//arrays to hold producer and consumer threads
	pthread_t prodThreads[num_prod];
	pthread_t consThreads[num_cons];
		
	//Create producer threads
	for(int i = 0; i < num_prod; i++){
		min1 = num_prod - 1;//minus 1 to check when on last thread create
		rmd = num_items % num_prod;//remainder will be added to last thread created
		div = num_items/num_prod;//divide by total producers to assign work to threads
		if(min1 == i){
			//final thread, add the remainder to thread work
			div = div + rmd;
			if(pthread_create(&prodThreads[i], NULL, produce, (void *)div) != 0){
				fprintf(stderr, "error: producer pthread_create fail%d\n ",i);
			}
		}else{
			//Work has been divided up and value for work is sent to thread function.
			if(pthread_create(&prodThreads[i], NULL, produce, (void *)div) != 0){
				fprintf(stderr, "error: producer pthread_create fail%d\n ",i);
			}
		}
	}

	//create consumer threads with consumer struct passed as argument
	for(int i = 0; i < num_cons; i++){
		if(pthread_create(&consThreads[i], NULL, consume, (void *)consD) != 0){
			fprintf(stderr, "error: consumer pthread_create fail%d\n",i);
		}
	}
	
	//join producer threads
	for(int i = 0; i < num_prod; i++){
		if(pthread_join(prodThreads[i],NULL) != 0){
			fprintf(stderr, "error: producer pthread_join fail");
		}
	}
	
	
	
	//join consumer threads
	for(int i = 0; i < num_cons; i++){
		if(pthread_join(consThreads[i],NULL) != 0){
			fprintf(stderr, "error: consumer pthread_join fail");
		}
	}
	fprintf(stderr,"Done producing\n");
	
	//free memory
	free(consD);
	
	return 0;
}

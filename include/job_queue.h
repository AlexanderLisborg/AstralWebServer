#ifndef __job_queue__
#define __job_queue__

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>

// Michael Scott concurrent queue with separate 
// mutexes for head and tail. Modified as a job queue.

// The scheduling algorithm determines which jobs get 
// prioritized when many are queued.
enum SCHEDALG {
  FIFO, // First in first out
  SFF, // Largest file size first (may starve small files with current implementation)
};

typedef struct job{
    int fd;
    char* line;
    int size;
    struct job* next;
}job_t;

typedef struct job_queue_t{
    sem_t dequeueAccess;
    sem_t enqueueAccess;
    job_t *head;
    job_t *tail;
    void(*jobFun) (int,char*);
    enum SCHEDALG schedalg;
    pthread_mutex_t headMutex;
    pthread_mutex_t tailMutex;
    pthread_cond_t waitUntilEmpty;
    char* logFileName;
}job_queue_t;

void job_queue_block_enqueues_and_wait_until_empty(job_queue_t *q){
    pthread_mutex_lock(&(q->tailMutex));
    int temp;
    while(!(q->head== NULL && q->tail==NULL)){
        pthread_cond_wait(&(q->waitUntilEmpty),&(q->tailMutex));
    }
    pthread_mutex_unlock(&(q->tailMutex));
}

int job_queue_init(job_queue_t *const q, void (*jobFun) (int,char*) ,int capacity,enum SCHEDALG schedalg,char* logFileName){
    int log_fd = -1;
    if (logFileName) {
        close(open(logFileName, O_WRONLY | O_CREAT | O_APPEND, 0644));
        q->logFileName = logFileName;
    } else q->logFileName = NULL;
    switch(schedalg){
        case FIFO: q->schedalg = FIFO; break;
        case SFF: q->schedalg = SFF; break;
        default: q-> schedalg = FIFO;
    }
    q->jobFun = jobFun;
    assert(!sem_init(&(q->dequeueAccess), 0, 0)); // TODO: replace assert with ass 
    assert(!sem_init(&(q->enqueueAccess), 0, capacity)); // TODO: replace assert with ass
    pthread_mutex_init(&(q->headMutex),NULL); 
    pthread_mutex_init(&(q->tailMutex),NULL);
    pthread_cond_init(&(q->waitUntilEmpty),NULL);
}

int job_queue_enqueue(job_queue_t * const q, int fd, int jobSize,char* line){
    if (q->logFileName) {
        int logFd =open(q->logFileName, O_WRONLY | O_CREAT | O_APPEND, 0644);
    }
    sem_wait(&(q->enqueueAccess));
    job_t *tmp = (job_t*) malloc(sizeof(job_t));
    assert(tmp!=NULL);// TODO: replace assert with ass 
    tmp -> next = NULL;
    tmp -> size = jobSize;
    tmp -> line = line;
    tmp -> fd = fd;
    // CRITICAL SECTION START
    pthread_mutex_lock(&(q->tailMutex));
    if(q->tail==NULL && q->head == NULL){
        pthread_mutex_lock(&(q->headMutex));
        q->head = q -> tail = tmp;
        pthread_mutex_unlock(&(q->headMutex));
    } else {
        if(q->schedalg == SFF){ // Sorted insert
            job_t *j = (q->head);
            pthread_mutex_lock(&(q->headMutex));
            if(jobSize<=j->size){ // Insert head
                tmp->next = q->head;
                q->head = tmp;
            } else{
                int index=0;
                // Iterate over queue
                while(1){
                    if(j==q->tail){ // Insert tail
                        j->next = tmp;
                        q->tail = tmp;
                        break;
                    }
                    if(jobSize <= j->next->size){
                        // stop and insert
                        tmp->next = j->next;
                        j->next = tmp;
                        break;
                    }
                    index++;
                    j = j->next; // next
                }
            }
            pthread_mutex_unlock(&(q->headMutex));
            
        } else { // FIFO insert
            q->tail->next = tmp;
            q->tail=tmp;
        }
    }
    
    sem_post(&(q->dequeueAccess));
    pthread_mutex_unlock(&(q->tailMutex));
    // CRITICAL SECTION END
};

int job_queue_execute(job_queue_t * const q){
    sem_wait(&(q->dequeueAccess));
    // CRITICAL SECTION START
    pthread_mutex_lock(&(q->headMutex));
    job_t *tmp = q->head;
    job_t *newHead = tmp->next;
    if(q->head==q->tail) q->tail = newHead;
    q->head = newHead; 
    if(q->head==NULL && q->tail==NULL)
        pthread_cond_signal(&(q->waitUntilEmpty));
    sem_post(&(q->enqueueAccess));
    // CRITICAL SECTION END
    pthread_mutex_unlock(&(q->headMutex));
    // Execute popped job...
    
    q->jobFun(tmp->fd,tmp->line);
    free(tmp);
    return 0;
};
/*
void append_log(char *buf, int stage, char *reqLine, int id, double now, double arrival) {
    size_t len = strlen(buf);
    if (len >= 1024 - 1) return;

    char *p = buf + len;
    size_t room = 1024 - len;

    switch (stage) {
    case 0:
        snprintf(p, room, "%.4fs [Thread Main] Arrived - request: %s\n", now, reqLine);
        break;
    case 1:
        snprintf(p, room, "%.4fs [Thread %d] Started - request: %s (%.4fs waiting)\n", now, id, reqLine, now - arrival);
        break;
    case 2:
        snprintf(p, room, "%.4fs [Thread %d] Completed - request: %s (%.4fs total)\n", now, id, reqLine, now - arrival);
        break;
    }
};*/

#endif // guard
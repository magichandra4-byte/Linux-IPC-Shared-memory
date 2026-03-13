#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TEXT_SZ 2048

struct shared_use_st {
    int written;
    char some_text[TEXT_SZ];
};

int main() {
    int shmid;
    void *shared_memory = NULL;
    struct shared_use_st *shared_stuff;

    shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    printf("Shared memory id = %d\n", shmid);

    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    printf("Memory attached at %p\n", shared_memory);

    shared_stuff = (struct shared_use_st *)shared_memory;
    shared_stuff->written = 0;

    pid_t pid = fork();

    if (pid == 0) {
        while (1) {
            while (shared_stuff->written == 0)
                sleep(1);

            printf("Consumer received: %s", shared_stuff->some_text);

            if (strncmp(shared_stuff->some_text, "end", 3) == 0)
                break;

            shared_stuff->written = 0;
        }

        shmdt(shared_memory);
        exit(0);
    } 
    else {
        char buffer[TEXT_SZ];

        while (1) {
            printf("Enter Some Text: ");
            fgets(buffer, TEXT_SZ, stdin);

            strcpy(shared_stuff->some_text, buffer);
            shared_stuff->written = 1;

            if (strncmp(buffer, "end", 3) == 0)
                break;

            while (shared_stuff->written == 1)
                sleep(1);
        }

        wait(NULL);

        shmdt(shared_memory);
        shmctl(shmid, IPC_RMID, 0);

        printf("Shared memory removed\n");
    }

    return 0;
}

/* KallistiOS ##version##

   rlock_test.c
   Copyright (C) 2008 Lawrence Sebald

*/

/* This program is a test for the recursive locks added in KOS 1.3.0. This
   synchronization primitive works essentially the same as a mutex, but allows
   the thread that owns the lock to acquire it as many times as it wants. */

#include <stdio.h>

#include <kos/thread.h>
#include <kos/recursive_lock.h>

#include <arch/arch.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#define UNUSED __attribute__((unused))

recursive_lock_t *l = NULL;

void thd0(void *param UNUSED) {
    int i;

    printf("Thd 0: About to obtain lock 10 times\n");

    for(i = 0; i < 10; ++i) {
        rlock_lock(l);
    }

    printf("Thd 0: Lock acquired %d times\n", l->count);
    printf("Thd 0: About to sleep\n");
    thd_sleep(100);

    printf("Thd 0: Awake, about to release lock 9 times\n");

    for(i = 0; i < 9; ++i) {
        rlock_unlock(l);
    }

    printf("Thd 0: About to sleep again\n");
    thd_sleep(10);

    printf("Thd 0: Awake, about to release lock\n");
    rlock_unlock(l);
    printf("Thd 0: done\n");
}

void thd1(void *param UNUSED) {
    printf("Thd 1: About to obtain lock 2 times\n");
    rlock_lock(l);
    rlock_lock(l);

    printf("Thd 1: About to pass timeslice\n");
    thd_pass();

    printf("Thd 1: Awake, going to release lock 2 times\n");
    rlock_unlock(l);
    rlock_unlock(l);

    printf("Thd 1: About to obtain lock 1 time\n");
    rlock_lock(l);

    printf("Thd 1: About to release lock\n");
    rlock_unlock(l);
    printf("Thd 1: done\n");
}

void thd2(void *param UNUSED) {
    int i;

    printf("Thd 2: About to obtain lock 200 times\n");

    for(i = 0; i < 200; ++i) {
        rlock_lock(l);
    }

    printf("Thd 2: About to release lock 200 times\n");

    for(i = 0; i < 200; ++i) {
        rlock_unlock(l);
    }

    printf("Thd 2: done\n");
}

KOS_INIT_FLAGS(INIT_DEFAULT);

int main(int argc, char *argv[]) {
    kthread_t *t0, *t1, *t2;

    /* Exit if the user presses all buttons at once. */
    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      arch_exit);

    printf("KallistiOS Recursive Lock test program\n");

    /* Create the recursive lock */
    l = rlock_create();

    if(!l) {
        printf("Could not create recursive lock, bailing out!\n");
        arch_exit();
    }

    printf("About to create threads\n");
    t0 = thd_create(thd0, NULL);
    t1 = thd_create(thd1, NULL);
    t2 = thd_create(thd2, NULL);

    printf("About to sleep\n");
    thd_wait(t0);
    thd_wait(t1);
    thd_wait(t2);

    if(rlock_is_locked(l)) {
        printf("Lock is still locked!\n");
        arch_exit();
    }

    rlock_destroy(l);

    printf("Recursive lock tests completed successfully!\n");
    return 0;
}

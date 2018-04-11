
#include "kernel_ken.h"

#define PRINT_HEADER(header)   if(getpid() == 1){print("\n\n======= "); print(header); print(" =======\n\n");}
#define PRINT_SUB_HEADER(header)   if(getpid() == 1){print("\n\n------- "); print(header); print(" --------\n");}
#define PRINT_SUB_SUB_HEADER(header)   if(getpid() == 1){print("\n-> "); print(header); print("\n");}

#define BUFFER_SIZE 4095

void barrier()
{
    if(getpid() != 1)
        exit();
}

void test_paging()
{
    PRINT_HEADER(__func__);
     int *value = alloc(sizeof(int),0);
     *value = 0xDEADBEEF;

     print_hex(*value);
     free(value);
     print("\n value after free: ");
     print_hex(*value);

     int *new_val = alloc(sizeof(int),0);
        print("\n realloced value: ");
     print_hex(*new_val);

     print("\n");
     if(new_val != value)
        print("ERROR");

    else
        print("SUCCESS");

    sleep(2);

}

void test_tasking()
{
    PRINT_HEADER(__func__);
    // Create a new process in a new address space which is a clone of this
    PRINT_SUB_HEADER("Forking");
    PRINT_SUB_SUB_HEADER("fork once");
    int ret = fork();
    print("fork returned ");
    print_hex(ret);
    print(", and pid returned ");
    print_hex(getpid());
    print("\n");

    PRINT_SUB_SUB_HEADER("fork twice");
    int ret1 = fork();
    print("fork returned ");
    print_hex(ret1);
    print(", and pid returned ");
    print_hex(getpid());
    print("\n");

    PRINT_SUB_HEADER("Other");

    PRINT_SUB_SUB_HEADER("exit all forks and printing all forks left running");
    barrier();

    print_hex(getpid());

    PRINT_SUB_SUB_HEADER("sleep 5 seconds with count");
    for(int i = 5; i >0; i--)
    {
        print_dec(i);
        print(" ");
        sleep(1);
    }

    PRINT_SUB_HEADER("test yield");

    PRINT_SUB_SUB_HEADER("before yield order");
    ret = fork();
    print_hex(getpid());
    print(" ");
    if(getpid()!=1)
        yield();

    PRINT_SUB_SUB_HEADER(" after yield order");
    print_hex(getpid());
    print(" ");

    if(getpid()==1)
        yield();

    barrier();

}

void basic_test_semaphores()
{
    PRINT_HEADER(__func__);
    int sem_id = open_sem(1);

    fork();

    wait(sem_id);

    print("proc ");
    print_dec(getpid());
    print(" has the lock ... yielding\n");
    yield();

    print("proc ");
    print_dec(getpid());
    print(" release and exit :");
    print_dec(signal(sem_id));
    print("\n");
    barrier();

    close_sem(sem_id);

}

void strong_semaphore_test()
{
    PRINT_HEADER(__func__);
    int semid;
    /* grab the semaphore created by seminit() */
    if((semid = open_sem(1)) == -1)
    {
        print(" *** initsem error\n");
        return;
    }

    print("Successfully created a new semaphore set with id: ");
    print_dec(semid);
    print("\n\n");
    int *data = alloc(sizeof(int),0);
    *data =0;
    wait(semid);
    for(int i=0; i < 4; i++)
    {
        if(getpid() == 1)
            fork();

        if(getpid() != 1)
        {
            wait(semid);
            break;
        }
    }


    if(getpid() == 1)
    {
        signal(semid);
        yield();
    }

    if(getpid() != 1)
        ++(*data);

    while(getpid() == 1 && (*data) < 4)
        yield();

    /* wait for all of the child processes */
    print_dec(getpid());
    print(" is unlocked and terminated \n");
    signal(semid);
    barrier();

    close_sem(semid);
}

void basic_test_pipe()
{
    PRINT_HEADER(__func__);
    char test_text[] = "this is a pipe dream";
    int pipe_id = open_pipe();
    write(pipe_id,test_text,21);
    print(test_text);
    print(" --> ");

    char input[128];
    read(pipe_id,input,21);
    print(input);
}


void strong_test_pipe()
{
    PRINT_HEADER(__func__);
    char text_1[] =     ">**** this is the first line *<";
    char text_2[] =     ">*** override the first line *<";

    int pipe_id = open_pipe();
    char input[40];

    write(pipe_id,text_1,32);
    read(pipe_id,input,32);
    PRINT_SUB_SUB_HEADER("initial buffer head");
    print(input);

    //fill in the pipe
    char dumb;
    for(int i = 33 ; i < BUFFER_SIZE ; i++)
    {
        write(pipe_id, (void*)'$', 1);
        read(pipe_id, &dumb, 1);
    }

    //read the head again to see if me can wrap around
    read(pipe_id,input,32);
    PRINT_SUB_SUB_HEADER("fill buffer and then reread the head");
    print(input);

    //overwrite the head
    write(pipe_id,text_2,32);

    //go back to the begining of the pipe
    for(int i = 33 ; i < BUFFER_SIZE ; i++)
    {
        write(pipe_id, (void*)'$', 1);
        read(pipe_id, &dumb, 1);
    }

    read(pipe_id,input,32);
    PRINT_SUB_SUB_HEADER("overwrite the head");
    print(input);


}

#define run_test(test)  barrier(); \
                        test(); \
                        barrier() ;

void my_app()
{
    print("ENTERING KERNEL TEST PHASE");
    print("\n============================================================================\n");

    run_test( test_paging );

    run_test( test_tasking );

    run_test( basic_test_pipe );

    run_test( strong_test_pipe );

    run_test( basic_test_semaphores );

    run_test( strong_semaphore_test );

    print("\n============================================================================\n");
    print("ALL TEST COMPLETE, \n\n ***starting user app\n");
}

#include "kernel_ken.h"

#include <stddef.h>
#include <stdint.h>

#define true 1
#define false 0

typedef unsigned int size_t;

void stall(int factor){
  int i;
  int j=0;
  for(i=0; i<(1<<factor); i++){
    j += i;
  }
  print("\n");
}

void test_mem(){
    print("***********************\n\0");
    print("*Begining Memory Tests*\n\0");
    print("***********************\n\0");

    void* ptr1 = alloc(0x00000100, false);
    print("ptr1: \0");
    print_hex((uint32_t)ptr1);
    stall(20);

    void* ptr2 = alloc(0x00000100, false);
    print("ptr2: \0");
    print_hex((uint32_t)ptr2);
    stall(20);

    void* ptr3 = alloc(0x00000100, false);
    print("ptr3: \0");
    print_hex((uint32_t)ptr3);
    stall(20);

    //free(ptr2);

    void* ptr4 = alloc(0x00000100, false);
    print("ptr4: \0");
    print_hex((uint32_t)ptr4);
    stall(20);

    void* ptr5 = alloc(0x00000100, false);
    print("ptr5: \0");
    print_hex((uint32_t)ptr5);
    stall(20);

    //free(ptr1);
    //free(ptr3);
    //free(ptr4);
    //free(ptr5);

    print("The following pointers should all be page aligned.\n");

    void* ptr6 = alloc(0x00000100, true);
    print("ptr6: \0");
    print_hex((uint32_t)ptr6);
    stall(20);

    void* ptr7 = alloc(0x00000100, true);
    print("ptr7: \0");
    print_hex((uint32_t)ptr7);
    stall(20);

    void* ptr8 = alloc(0x00000100, true);
    print("ptr8: \0");
    print_hex((uint32_t)ptr8);
    stall(20);

    void* ptr9 = alloc(0x00000100, true);
    print("ptr9: \0");
    print_hex((uint32_t)ptr9);
    stall(20);

    void* ptr10 = alloc(0x00000100, true);
    print("ptr10: \0");
    print_hex((uint32_t)ptr10);
    stall(20);

    print("The following pointers should all be NULL (0).\n");

    void* ptr11 = alloc(0x00000000, true);
    print("ptr11: \0");
    print_hex((uint32_t)ptr11);
    stall(20);

    void* ptr12 = alloc(0xFFFFFFFF, true);
    print("ptr12: \0");
    print_hex((uint32_t)ptr12);
    stall(20);

    print("***********************\n\0");
    print("*Memory Tests Complete*\n\0");
    print("***********************\n\0");


}

void test_proc(){

    print("************************\n\0");
    print("*Begining Process Tests*\n\0");
    print("************************\n\0");
    print("Forking\n\0");
    stall(20);
    int pid = fork();
    stall(20);
    print("Forked\n\0");
    if(!pid){
        print("Child process was here\n\0");
        exit();
    } else if(pid == (0-1)) {
        print("Parent process failed to create child\n\0");
    } else {
        print("Parent process created child\n\0");
        yield();
    }

    pid = fork();
    setpriority(1, 1);
    yield();
    if(!pid){
        print("I printed second\n\0");
        exit();
    } else if(pid == -1) {
        print("Parent process failed to create child\n\0");
    } else {
        print("I print first.\n\0");
        print("Sleeping for 4 seconds\n\0");
        sleep(4);
        print("The highest priority process is woke\n\0");
    }

    print("************************\n\0");
    print("*Process Tests Complete*\n\0");
    print("************************\n\0");
    stall(26);
}

void test_sem(){
    print("**************************\n\0");
    print("*Begining Semaphore Tests*\n\0");
    print("**************************\n\0");

    int sid;
    if(getpid() == 1)
        sid = open_sem(1);

    int pid = fork();
    if(!pid){
        wait(sid);
        print("Child Aquired semaphore\n\0");
        stall(26);
        print("Child released semaphore\n\0");
        signal(sid);
        exit();
    } else if(pid == (0-1)) {
        print("Parent process failed to create child\n\0");
    } else {
        yield();
        wait(sid);
        print("Parent Aquired semaphore\n\0");
        stall(26);
        print("Parent released semaphore\n\0");
        signal(sid);
    }

    print("**************************\n\0");
    print("*Semaphore Tests Complete*\n\0");
    print("**************************\n\0");
}

void test_pipes(){
      print("**************************\n\0");
      print("*Begining IPC Pipes Tests*\n\0");
      print("**************************\n\0");

      int pipe_fd = open_pipe();
      int pid = fork();
      char buff[16];
      if(!pid){
        char i;
        for(i=0; i<16; i++)
          buff[i] = i;
        size_t written = write(pipe_fd, (const void*)buff, 16);
        print("Expect written is 16. written is \0");
        print_dec(written);
        print("\n");
        exit();
      } else if(pid == (0-1)) {
        print("Parent process failed to create child\n\0");
      } else {
				size_t bytes_read;
        do{
          print("Attempting to read bytes in 2 seconds\n\0");
          sleep(2);
          bytes_read = read(pipe_fd, (void*)buff, 16);
        } while(!bytes_read);
        print("Expect bytes_read is 16. bytes_read is \0");
        print_dec(bytes_read);
        print("\n");
      }
      close_pipe(pipe_fd);
      size_t written = write(pipe_fd, (const void*)buff, 16);
      print("Expect written is (uint32_t)-1. written is ");
      print_dec(written); // Fails because print_dec doesn't work for 2^32 - 1
      print("\n");
      print("**************************\n\0");
      print("*IPC Pipes Tests Complete*\n\0");
      print("**************************\n\0");
}

void user_app(){
  test_mem();
  test_proc();
  test_sem();
  test_pipes();
}

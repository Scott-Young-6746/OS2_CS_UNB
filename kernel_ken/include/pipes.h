#ifndef PIPES_H
#define PIPES_H

#ifndef INVALID_PIPE
#define INVALID_PIPE -1
#endif

typedef struct pipe {
  int fildes;
  uint8_t written_to;
  uint8_t first_unread;
  uint8_t first_unwritten;
  char buff[256];
} pipe_t;

void initialise_pipes();
int internal_open_pipe();
int internal_close_pipe(int fildes);
unsigned int internal_write(int fildes, const void *buf, unsigned int nbyte);
unsigned int internal_read(int fildes, void *buf, unsigned int nbyte);

#endif

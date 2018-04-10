#include "common.h"
#include "pipes.h"

#define VALID_SEM(sid) (sid >= 1 && sid <= 32)
#define sid_index(sid) (sid-1)

pipe_t pipes[16][16];
uint16_t pipes_used[16] = { 0 };

#define row_index(fildes) (fildes/16)
#define col_index(fildes) (fildes%16)
#define VALID_ROW(fildes) (row_index(fildes) >= 0 && row_index(fildes) < 16)
#define VALID_COL(fildes) (col_index(fildes) >= 0 && col_index(fildes) < 16)
#define VALID_FILDES(fildes) (VALID_ROW(fildes) && VALID_COL(fildes))

#define is_set(r, c) (pipes_used[r] & (uint16_t)(1<<c))
#define set_pipe(r, c) (pipes_used[r] |= (uint16_t)(1<<c))
#define unset_pipe(r, c) (pipes_used[r] &= ((uint16_t)0xFFFF-(uint16_t)(1<<c)))

int get_first_unset_pipe_in_row(int row){
  int i;
  for(i=0; i<16; i++){
    if(!is_set(row, i)){
      return i;
    }
  }
  return INVALID_PIPE;
}

int get_first_unset_pipe(){
  int i;
  for(i=0; i<16; i++){
    int j = get_first_unset_pipe_in_row(i);
    if(j != INVALID_PIPE)
      return (i*16)+j;
  }
  return INVALID_PIPE;
}

void initialise_pipes(){

}

int internal_open_pipe(){
  pipe_t pipe;
  pipe.fildes = get_first_unset_pipe();
  if(VALID_FILDES(pipe.fildes)){
    pipe.written_to = 0;
    pipe.first_unread = 0;
    pipe.first_unwritten = 0;
    uint16_t row = (uint16_t)row_index(pipe.fildes);
    uint16_t col = (uint16_t)col_index(pipe.fildes);
    pipes[row][col] = pipe;
    set_pipe(row, col);
  }
  return pipe.fildes;
}

int internal_close_pipe(int fildes){
  if(!VALID_FILDES(fildes)) return INVALID_PIPE;
  uint16_t row = row_index(fildes);
  uint16_t col = col_index(fildes);
  unset_pipe(row, col);
  return 0;
}

unsigned int internal_write(int fildes, const void *buf, unsigned int nbyte){
  const char* buff = buf;
  if(!VALID_FILDES(fildes)) return INVALID_PIPE;
  int row = row_index(fildes);
  int col = col_index(fildes);
  if(!is_set(row, col)) return INVALID_PIPE;
  pipe_t* pipe = &pipes[row][col];
  int i = pipe->first_unwritten;
  int used = 0;
  uint32_t available = ( pipe->first_unread > pipe->first_unwritten) ?
                        (pipe->first_unread - pipe->first_unwritten) :
                        ((255 - pipe->first_unwritten) + (pipe->first_unread));
  if(available < nbyte) return 0;
  pipe->written_to = 1;
  while(used < nbyte){
    pipe->buff[i] = buff[used];
    i = (i==255) ? 0 : i+1;
    used++;
  }
  return used;
}

unsigned int internal_read(int fildes, void *buf, unsigned int nbyte){
  char* buff = buf;
  if(!VALID_FILDES(fildes)) return INVALID_PIPE;
  int row = row_index(fildes);
  int col = col_index(fildes);
  if(!is_set(row, col)) return INVALID_PIPE;
  pipe_t* pipe = &pipes[row][col];
  int i = pipe->first_unread;
  int used = 0;
  uint32_t available = ( pipe->first_unwritten > pipe->first_unread) ?
                        (pipe->first_unwritten - pipe->first_unread) :
                        ((255 - pipe->first_unread) + (pipe->first_unwritten));
  if(available < nbyte || !pipe->written_to) return 0;
  while(used < nbyte){
    buff[used] = pipe->buff[i];
    i = (i==255) ? 0 : i+1;
    used++;
  }
  return used;
}

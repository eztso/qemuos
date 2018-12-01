#ifndef _SYS_H_
#define _SYS_H_

#include "stdint.h"

/****************/
/* System calls */
/****************/

typedef int ssize_t;
typedef unsigned int size_t;

/* all system calls return negative value on failure except when noted */

/* exit */
/* never returns, rc is the exit code */
extern void exit(int rc);

/* open */
/* opens a file, returns file descriptor */
extern int open(const char* fn, int flags);

/* len */
/* returns number of bytes in file */
extern ssize_t len(int fd);

/* write */
/* writes up to 'nbytes' to file, returns number of files written */
extern ssize_t write(int fd, void* buf, size_t nbyte);

/* read */
/* reads up to nbytes from file, returns number of bytes read */
extern ssize_t read(int fd, void* buf, size_t nbyte);

/* create semaphore */
/* returns semaphore descriptor */
extern int sem(uint32_t initial);

/* up */
/* semaphore up */
extern int up(int id);

/* down */
/* semaphore down */
extern int down(int id);

/* close */
/* closes either a file or a semaphore or disowns a child process */
extern int close(int id);

/* shutdown */
extern int shutdown(void);

/* wait */
/* wait for a child, status filled with exit value from child */
extern int wait(int id, uint32_t *status);

/* seek */
/* seek to given offset in file */
extern off_t seek(int fd, off_t offset);

/* fork */
extern int fork();

/* execl */
extern int execl(const char* path, const char* arg0, ...);

#endif

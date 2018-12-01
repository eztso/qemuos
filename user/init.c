#include "libc.h"

void one(int fd) {
    printf("*** fd = %d\n",fd);
    printf("*** len = %d\n",len(fd));

    cp(fd,2);
}


int fib1(int f)
{
	int32_t res = -1;

    int32_t pid = fork();
    if (pid == 0)
    {
       printf("*** Child is producing the %dth fib number...\n", f);
       int32_t a = 0;
       int32_t b = 1;
       int32_t n = a + b;
       printf("*** %li\n",a);
       printf("*** %li\n",b);
       for (int32_t i=0;i<f - 1;i++)
       {
          n=a+b;
          printf("*** %li\n", n);
          a=b;
          b=n;
       }
       printf("*** Exiting child with %li as exit code\n", n); 
       exit(n);
    }
    else if(pid > 0)
    {
       wait(pid, (uint32_t*)&res);
       printf("*** Leaving parent, recieved %li as calculation for %dth fib number\n", res, f);
    }
    else
    {
    	printf("*** Fork failed\n");
    }
   return res;
}

int main(int argc, char** argv) {
	const int n = 15;
	const int nthFib = 610;

	int n_1 = -1;
	int n_2 = -1;

	int pid = fork();

	if(pid == 0)
	{
		printf("*** in child\n");
		int tmp = fib1(n-1);
		exit(tmp);
	}
	else if (pid > 0)
	{
		wait(pid, (uint32_t*) &n_1);
		printf("*** in parent\n");
	}
	else
	{
		printf("*** Fork failed\n");
		exit(-1);
	}
	printf("*** %d %d\n",n_1, n_2 );

	int pid2 = fork();
	if(pid2 == 0)
	{
		printf("*** in child2\n");
		int tmp = fib1(n - 2);
		exit(tmp);
	}
	else if (pid2 > 0)
	{
		wait(pid2, (uint32_t*) &n_2);
		printf("*** in parent2\n");
	}
	else
	{
		printf("*** Fork2 failed\n");
		exit(-1);
	}    
	printf("*** Calculated %d as the %dth fib number\n",n_1 + n_2, n);
	if(n_1 + n_2 == nthFib)
		printf("*** That is correct!\n");

	// oof better take care of this special case
	const int32_t stdin = 0;
	close(stdin);
	int32_t nextFd = open("/etc/data.txt",0);
	if(nextFd != 0)
		printf("*** But we just closed this!\n");
	one(nextFd);


	const int32_t stdout = 1;
	close(stdout);
	puts("*** This should not be here\n");

	shutdown();
    return 0;
}

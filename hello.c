#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char** argv) {
	pid_t me = getpid();
	size_t i = 0;
	for (i=0; i<50; i++)
		printf("pid %d count:%d\n", me, i);

	return 0;
}

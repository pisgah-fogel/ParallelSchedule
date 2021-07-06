#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

// gcc -shared -o libfoo.so foo.o
// LD_PRELOAD=/path/to/check_mem.so
#define FILEBUF 512

#define MEASURE_ELAPSED // Set it to mesure elapsed time
#define DUMP_MAPS // Set if to dump mapped memories
#define DEBUG

#ifdef MEASURE_ELAPSED
static struct timeval t_start;
#endif

void copy_file(const char* sourcePath,const char* destinationPath)
{
  FILE *filew;
  FILE *filer;
  char *buffer;
  size_t count;  /* Using size_t makes robust C code that is portable and avoids implicit type conversions. */
  
  /*Open files for copying*/
  if( ! ( buffer = (char *)malloc( sizeof( char ) * FILEBUF ) )  ) {
    fprintf(stderr,"Can not allocate memory for memory mapping\n");
  }
  else if((filew = fopen(destinationPath,"wb")) == NULL){
    free( buffer ); /* Cleanup! */
    fprintf(stderr,"Can not copy memory mapping\n");
  }
  else if((filer = fopen(sourcePath,"rb")) == NULL){
    fclose( filew ); /* Cleanup! */
    free( buffer ); /* Cleanup! */
    fprintf(stderr,"Can not copy memory mapping\n");
  }
  else
  {  
    /* Read file by FILEBUF sized chunks and write to workspace location. */
    /* If less than FILEBUF characters read, then no problem, write the partial out.  */
    /* If 0 bytes read, then consider the copy operation done.  If this is a device, then this should be changed. */
    while( ( count = fread( buffer, sizeof( char ), FILEBUF, filer ) ) > 0 )
      if( fwrite( buffer, sizeof( char ), count, filew ) != count ) 
        break; /* Error, could not write the full length. */
  
    /*close file connections*/
    fclose(filer);
    fclose(filew);

    free( buffer ); /* Cleanup! */
  }
}

void at_exit(void) {
#ifdef MEASURE_ELAPSED
	struct timeval t_end;
	gettimeofday(&t_end, NULL);
	int elapsed = ((t_end.tv_sec - t_start.tv_sec) * 1000000) + (t_end.tv_usec - t_start.tv_usec);
	printf("Elapsed %d\n", elapsed);
#endif

#ifdef DUMP_MAPS
	char buffer[20];
	pid_t my_pid = getpid();
	snprintf(buffer, 20, "/tmp/map-%d", my_pid);
	copy_file("/proc/self/maps", buffer);
	#ifdef DEBUG
	puts("Mapping dumped");
	#endif
#endif
}

__attribute__((constructor)) void runs_first(void) {
  atexit(&at_exit);

#ifdef MEASURE_ELAPSED
  gettimeofday(&t_start, NULL);
#endif
};

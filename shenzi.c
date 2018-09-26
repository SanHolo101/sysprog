#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <string.h>

int
main(int argc, char **argv) 
{
	char str_in[100];
	char str_out[100];

	fprintf(stderr, "shenzi 1\n");
	if (read(STDIN_FILENO, str_in, 100) == -1) {
		err(1, "Error reading from stdin");
	}

	fprintf(stderr, "shenzi 2\n");
	fprintf(stderr, "%d - GOT THIS: \n %s \n", getpid(), str_in);

	sprintf(str_out, "REPLYING TO PARENT FOR [%s]\n", str_in);
	if (write(STDOUT_FILENO, str_out, 100) == -1) {
		err(1, "Error writing to stdoout");
	}


	return 0;
}


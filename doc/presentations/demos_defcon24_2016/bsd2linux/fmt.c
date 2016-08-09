#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *foobar="static";

int main(int argc, char **argv){
	char tata[]="static";

	printf(foobar);
	printf("toto");
	printf(argv[3]);
	printf(tata);
	return 0;
}

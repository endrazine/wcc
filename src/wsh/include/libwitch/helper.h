int read_maps(int pid);
int is_mapped(unsigned long int addr);

extern struct section *zfirst;
extern int nsections;

/*
* Data structures
*/
// section
struct section {
	unsigned long long int init;	// start address
	unsigned long long int end;	// end address
	int size;	// size
	int perms;	// permissions
	char name[255];	// name
	char hperms[10];// permissions in human readable form
	void *next;	// ptr to next section

	int num;	// section number in memory mapping
	int proba;	// aslr stuff (highest probability of a given mapping)
	int probableval;// aslr stuff (address of most probable mapping)
};


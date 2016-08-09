#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

int main(int argc, char **argv){
  void *handle = dlopen(argv[1], RTLD_LAZY);
  if(!handle){ printf("%s\n",dlerror()); exit(-1);}
  printf("[%u] : success loading %s\n", getpid(), argv[1]);
  getchar();
  return 0;
}


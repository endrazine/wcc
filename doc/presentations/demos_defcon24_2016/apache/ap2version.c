/**
* Calling ap_get_server_banner() from /usr/sbin/apache2
*
* endrazine for Defcon 24 // August 2016
*/
#include <stdio.h>

void *ap_get_server_banner();

int main (void){
	printf("Server banner: \e[31m%s\e[0m\n", ap_get_server_banner());
	return 0;
}


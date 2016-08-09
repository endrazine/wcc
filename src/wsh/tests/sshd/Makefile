CC := gcc 
CFLAGS := -W -Wall -Wextra 
LDFLAGS := /usr/sbin/sshd

all::
	$(CC) $(CFLAGS) $(LDFLAGS) ssh.c -o ssh

clean:
	rm -rf ssh

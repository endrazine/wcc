CC := gcc 
CFLAGS := -W -Wall -Wextra -Wl,-e__main
LDFLAGS := /usr/sbin/sshd

all::
	$(CC) $(CFLAGS) $(LDFLAGS) sshd2.c -o sshd2

clean:
	rm -rf sshd2

#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2022 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

all::
#	cd apache2 && make
#	cd  sshd && make
#	cd  sshd2 && make
	cd  globals && make

# Self-checking regression tests
test:
	cd globals && make test
	cd syscall && make test
	cd prototypes && make test

clean:
	cd apache2 && make clean
	cd  sshd && make clean
	cd  sshd2 && make clean
	cd  globals && make clean
	cd  syscall && make clean
	cd  prototypes && make clean



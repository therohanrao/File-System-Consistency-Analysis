#NAME: Rohan Rao,Ethan Ngo
#EMAIL: raokrohan@gmail.com,ethan.ngo2019@gmail.com
#ID: 305339928,205416130

lab3a: lab3a.c ext2_fs.h
	@gcc -g lab3a.c -Wall -Wextra -o lab3a
	@chmod 777 lab3a
dist: lab3a.c README Makefile ext2_fs.h
	@tar -cvzf lab3a-305339928.tar.gz README Makefile lab3a.c *.h
	@chmod 777 lab3a-305339928.tar.gz
sanity: lab3a-305339928.tar.gz
	@./sanity.sh 305339928
.PHONY: clean
clean: 
	@rm -f lab3a-305339928.tar.gz lab3a
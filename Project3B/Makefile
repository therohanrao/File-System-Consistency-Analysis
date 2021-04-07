#NAME: Rohan Rao,Ethan Ngo
#EMAIL: raokrohan@gmail.com,ethan.ngo2019@gmail.com
#ID: 305339928,205416130

lab3b: lab3b.py
	@cp template.sh lab3b
	@chmod 777 lab3b
dist: lab3b.py README Makefile argparser template.sh
	@tar -cvzf lab3b-305339928.tar.gz README Makefile lab3b.py argparser template.sh
	@chmod 777 lab3b-305339928.tar.gz
sanity: lab3b-305339928.tar.gz
	@./sanity.sh 305339928
.PHONY: clean
clean: 
	@rm -f lab3b-305339928.tar.gz lab3b
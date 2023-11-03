all:
	gcc  -m32 -no-pie -nostdlib -o sum sum.c
	gcc  -m32 -no-pie -nostdlib -o fib fib.c
	gcc -m32 -o Smart_loader Smart_loader.c
	./Smart_loader sum 
	./Smart_loader fib

clean:
	-@rm -f sum Smart_loader fib
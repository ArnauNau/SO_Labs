ARGS = $(filter-out $@,$(MAKECMDGOALS))

# make main
main: src/S1.c
	gcc -Wall -Werror -Wpedantic -g -O0 -fsanitize=undefined src/S1.c -o main
	#gcc -Wall -Werror -Wpedantic -g -O0 -fsanitize=address,undefined src/main.c -o main
# make clean
clean:
	rm -f main
# make val
val:
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./main $(ARGS)
# make
all: main
# make run
run:
	./main

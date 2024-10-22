ARGS = $(filter-out $@,$(MAKECMDGOALS))

# make main
main: src/S4.c
	gcc -Wall -Wextra -Werror -Wpedantic -g -O0 -fsanitize=undefined -lpthread src/S4.c -o main
	#gcc -Wall -Werror -Wpedantic -g -O0 -fsanitize=address,undefined src/main.c -o main
# make clean
clean:
	rm -f main
# make val
val:
	valgrind -s --read-var-info=yes --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes --track-fds=yes ./main $(ARGS)
# make
all: main
# make run
run:
	./main

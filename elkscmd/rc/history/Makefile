CC=gcc -g -O

all: history

history: history.c
	$(CC) -o history history.c
	@echo 'Now install "history" as -, --, -p and --p in your bin directory'
	@echo '(for example, by making them links to the same file)'

clean:
	rm -rf core history

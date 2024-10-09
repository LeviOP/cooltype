CC=gcc
CFLAGS=-Wall -Wextra
LDFLAGS=

SRCS=main.c
OBJS=$(SRCS:.c=.o)
EXEC=cooltype

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)

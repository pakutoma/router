SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)
CC=gcc
CFLAGS=-g -Wall -Werror
TARGET=router
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

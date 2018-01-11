SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)
CFLAGS=-Wall -Werror -O0 -g3
TARGET=router
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

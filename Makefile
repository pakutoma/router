SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)
CFLAGS=-Wall -O3
TARGET=router
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

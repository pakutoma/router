SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)
CFLAGS=-Wall -Werror -O0 -g3 -lxml2 -I/usr/include/libxml2
TARGET=router
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

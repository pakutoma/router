OBJS=main.o device_init.o router.o log.o ether_frame.o
SRCS=$(OBJS:%.o=%.c)
CFLAGS=-g -Wall
TARGET=router
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

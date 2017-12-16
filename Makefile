OBJS=main.o device_init.o route.o log.o ether_frame.o arp_table.o arp_waiting_list.o create_arp.o create_icmp.o send_queue.o ip_packet.o checksum.o device.o arp_packet.o
SRCS=$(OBJS:%.o=%.c)
CFLAGS=-g -Wall
TARGET=router
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

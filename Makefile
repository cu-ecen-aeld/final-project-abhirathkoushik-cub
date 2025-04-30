TARGET = apds_can_control

all:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) apds_can_control.c

clean:
	rm -f $(TARGET)


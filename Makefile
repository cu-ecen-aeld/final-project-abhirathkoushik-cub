TARGET 1= can_tx_from_file
TARGET 2= apds_can_control

all:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET 1) can_tx_from_file.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET 2) apds_can_control.c
clean:
	rm -f $(TARGET 1) $(TARGET 2)


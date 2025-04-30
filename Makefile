TARGET1 = can_tx_from_file
TARGET2 = apds_can_control
TARGET3 = socket_can_client
TARGET4 = soclet_can_server

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

$(TARGET1): $(TARGET1).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(TARGET2): $(TARGET2).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(TARGET3): $(TARGET3).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(TARGET4): $(TARGET4).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)


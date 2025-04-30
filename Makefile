TARGET1 = can_tx_from_file
TARGET2 = apds_can_control
TARGET3 = socket_can_client
TARGET4 = socket_can_server
TARGET5 = rx_client
TARGET6 = tx_server

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET6)

$(TARGET1): $(TARGET1).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(TARGET2): $(TARGET2).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(TARGET3): $(TARGET3).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(TARGET4): $(TARGET4).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<


$(TARGET5): $(TARGET5).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<


$(TARGET6): $(TARGET6).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET6)

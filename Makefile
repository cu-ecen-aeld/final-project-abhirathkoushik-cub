TARGET1 = can_tx_from_file
TARGET2 = apds_can_control

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(TARGET1).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(TARGET2): $(TARGET2).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET1) $(TARGET2)


TARGET1 = apds_read
TARGET2 = apds_read_file

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(TARGET1).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(TARGET2): $(TARGET2).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET1) $(TARGET2)



COMM_SRC = log_debug.c sock.c
COMM_FILE := $(patsubst %.c,%.o,$(COMM_SRC))

CC1=/opt/buildroot-gcc342/bin/mipsel-linux-gcc

all: nc_cli nc_ser

nc_cli: nc_cli.c $(COMM_SRC)
	$(CC) -o $@ $^

nc_ser:  nc_ser.c $(COMM_SRC)
	$(CC1) -o $@ $^

clean:
	rm -f *.o nc_cli  nc_ser

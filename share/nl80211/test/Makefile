CROSS_COMPILE:=arm-hisiv300-linux-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

CFLAGS = -I../../../libs/include/netlink -I../
LDFLAGS = -lnl -L../../../libs/arm-hisiv300-linux- -lc -lpthread -lm

OBJ := iw_80211_scan.o event.o genl.o util.o status.o reg.o reason.o

TARGET = iw_scan

all: $(OBJ) scan_test.o
	$(CC) $^ -o ${TARGET} $(LDFLAGS)
	#cp ${TARGET} ~/jovision/progs-hi3518EV200/ -a
	@echo OK!

%.o: ../%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) scan_test.o ${TARGET}
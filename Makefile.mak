obj-m += custom_tcp_cc.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

// CLI settings
CXX = g++
CXXFLAGS = -Wall -O2 -std=c++17
CLI_TARGET = ccctl
CLI_SRC = ccctl.cpp

all: module cli

module:
	make -C $(KDIR) M=$(PWD) modules

cli:
	$(CXX) $(CXXFLAGS) -o $(CLI_TARGET) $(CLI_SRC)

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f $(CLI_TARGET)

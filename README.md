TCP congestion control module along with a command line tool to switch and test congestion control algorithms dynamically.

The module works directly inside the Linux networking stack and can handle real TCP traffic.

FEATURES

Custom TCP congestion control algorithm
Kernel module implementation
User space CLI controller
Works with real network traffic such as iperf, curl, ssh
Supports production style deployment
Portable Linux build system

PROJECT STRUCTURE

tcp-cc-project folder contains:

custom_tcp_cc.c -> Kernel congestion control module
ccctl.cpp -> User space CLI controller
Makefile -> Build automation
README -> Documentation

SYSTEM REQUIREMENTS

Linux Kernel version 5 or newer
Ubuntu or Debian recommended
Root privileges required

INSTALLATION STEPS

STEP 1 - Install Dependencies

Run the following command:

sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)


STEP 2 - Clone Repository

git clone YOUR_REPOSITORY_URL
cd tcp-cc-project

STEP 3 - Build Project

Run:

make

This will generate:

custom_tcp_cc.ko
ccctl

DEPLOYMENT

STEP 4 - Load Kernel Module

sudo insmod custom_tcp_cc.ko

STEP 5 - Verify Module Loaded

lsmod | grep custom

STEP 7 - View Available Congestion Algorithms

sysctl net.ipv4.tcp_available_congestion_control

STEP 8 - Activate Custom Algorithm

Using CLI tool:

sudo ./ccctl set custom_cubic

OR using sysctl command:

sudo sysctl -w net.ipv4.tcp_congestion_control=custom_cubic
STEP 9 - Verify Active Algorithm

sysctl net.ipv4.tcp_congestion_control

TESTING NETWORK TRAFFIC

STEP 10 - Install iperf

sudo apt install iperf3

STEP 11 - Start Test Server

iperf3 -s

STEP 12 - Run Client

iperf3 -c 127.0.0.1

STEP 13 - Monitor TCP State

ss -ti

UNLOADING MODULE

sudo rmmod custom_tcp_cc

PERMANENT DEPLOYMENT

STEP 14 - Copy Module To Kernel Directory

sudo cp custom_tcp_cc.ko /lib/modules/$(uname -r)/

STEP 15 - Update Module Dependencies

sudo depmod

STEP 16 - Load Using modprobe

sudo modprobe custom_tcp_cc

STEP 17 - Auto Load Module On Boot

Edit the file:

/etc/modules

Add the line:

custom_tcp_cc

STEP 18 - Set Default Congestion Algorithm

Edit the file:

/etc/sysctl.conf

Add the line:

net.ipv4.tcp_congestion_control=custom_cubic

Apply changes:

sudo sysctl -p

CLEAN BUILD FILES

make clean

TROUBLESHOOTING

If module fails to load, reinstall kernel headers:

sudo apt install --reinstall linux-headers-$(uname -r)

If permission errors occur, run commands using sudo.

If algorithm does not appear, check kernel logs using:

dmesg

SAFETY NOTICE

Do not deploy experimental congestion algorithms on critical production servers without proper testing. Incorrect implementations may reduce network performance or cause instability.


LICENSE

GPL License required for Linux kernel modules
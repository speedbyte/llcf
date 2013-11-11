insmod can
insmod vcan
insmod can-raw
insmod can-bcm
ifconfig can0 up
ifconfig vcan2 up
echo "i 0x4914 s" > /dev/pcan32
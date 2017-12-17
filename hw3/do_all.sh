rmmod message_slot; rm message_slot.ko; rm /dev/my_cool_device; make; insmod message_slot.ko; mknod /dev/my_cool_driver c 244 1;

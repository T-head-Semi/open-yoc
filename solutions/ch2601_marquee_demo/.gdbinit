target remote 192.168.125.225:1025
#target remote localhost:1025

#si
#set *0x50008000=0x1
#set *0x50008000=0
#shell sleep 2

#target remote 30.225.208.112:1025

si
set $mhcr=0
set $mstatus&=~(0x00000008)

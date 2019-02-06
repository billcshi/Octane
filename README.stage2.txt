a) Reused Code: Yes
	* the icmp_checksum.c entire code (no change to the origin code)
	* the sample_tunnel.c entire code (no change to the origin code)
b) Complete: 
	* The output file is the same(the number is different) as Sample output
	* WireShark can capure the ICMP reply packet from primary router
	* Still uncomplete: the ping process in other window had not receive the reply
c) Portable:
	* A problem might be ICMP packet in single computer doesn't contain MAC address which is 84 bytes, and remote ping packet is 96 bytes(2 MAC addresses added) 
	* my code use some fixed number=84 bytes, so it will be wrong if running on multi-computer network


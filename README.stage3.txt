1. Reused Code:
	* Since Stage 3 still need to reply ECHO to 10.5.51/24, so the part of init a router and reply ECHO is reused from Stage 2
2. Complete:
	* Stage 3 is completed, using wireshark to track, packet can be transmit to outside internet using eth1 via second router
3. Addressing on the way out of your router:
	* If I didn't rewrite the source IP, the reply ICMP will send directly to server rather than the second router
4. Addressing on the way in to the VM:
	* I bind the second router's raw socket to eth1 interface rather than the IP address. So if there is not a separate network device, all VM's traffic will go through the raw socket.
5. Addressing from the VM to the host:
	* Since I use NAT in configuring VM, the traffic through NAT will change its ip to host ip and change its port to identify where it is from. The host machine will deal with it.

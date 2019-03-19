1. Reused Code:
	* Represent TCP header(struct tcphdr) and TCP psd-IPheader(struct psdtcphdr) according to RFC 793
	* Rebuild the primary router code to fit more than 1 secondary routers
2. Complete:
	* Stage 6 is completed
3. Multiplexing:
	* Since I use different NICs(eth1, eth2) for different (router1,router2), router 1 only used eth1 and router 2 only used eth2.
	* The NAT gateway will seperate the traffic to different NIC according to the port number.
	* If the traffic is going through eth1, this packet should be used in router1.


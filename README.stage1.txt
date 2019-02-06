a) Reused Code:
	* part of UDP socket code from Steven's book
	* include socket setting(UDP_start in Router.cpp) and msg send/recv
b) Complete:
	* This stage is completed
c) Portable:
	* This stage use <sys/sockets.h> for UDP socket and <unistd.h> for fork new process which is provide by Linux system library which means the code can run on any arthitectures that can run linux

1. Reused Code:
	* In Stage4, I rebuild the primary router code to send the reliable transmit.
	* I use the timer code(timers.cc/.hh & tools.cc/.hh) from TA web and make no change in the origin files.
	* The reliable timer setup code is inspired by (test-app.cc/.hh) but use no code from these files.
	* I use the (struct octane_control) define from the projb_current.pdf and make no change in this struct.
2. Complete:
	* Stage4 is completed
3. Reliability:
	* To implement reliable transmit, I use the class timer library.
	* Using 2 kind of timer:
		1. 15s timer to check if any traffic through the router. Stop the router after this timer is timeout.
		2. 2s timer, for each Octane Control Message. Start a timer when first sending the message and register the (segno,handle) to a std::map. When receive an ACK, RemoveTimer by handle from map. When timer expired, which means packet lost, resend the control message and restart the timer.
4. Chance of failure:
	* The only failure I met is: Due to different timer to control the primary and secondary router to close. So if the secondary router is close and the primary router still need to send the control_msg, it will resend.


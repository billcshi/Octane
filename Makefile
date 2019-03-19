all: projb
projb: proja.o task_config.o ./Stage1/stage1main.o ./Stage2/stage2main.o ./Stage3/stage3main.o ./Stage4/stage4main.o ./Stage5/stage5main.o ./Stage6/stage6main.o ./Stage7/stage7main.o ./timers/tools.o ./timers/timers.o ./Router/OctaneManager.o ./Router/router.o ./Router/PrimaryRouter.o ./Router/SecondaryRouter.o ./GlobalVar.o ./Router/sample_tunnel.o ./Datagram/IPDatagram.o ./Datagram/icmp_checksum.o
	g++ -g ./task_config.o ./proja.o ./Stage1/stage1main.o ./Stage2/stage2main.o ./Stage3/stage3main.o ./Stage4/stage4main.o ./Stage5/stage5main.o ./Stage6/stage6main.o ./Stage7/stage7main.o ./timers/tools.o ./timers/timers.o ./Router/OctaneManager.o ./Router/router.o ./Router/PrimaryRouter.o ./Router/SecondaryRouter.o ./Router/sample_tunnel.o ./GlobalVar.o ./Datagram/IPDatagram.o ./Datagram/icmp_checksum.o -o ./projb
proja.o: proja.cpp
	g++ -g -c ./proja.cpp -o ./proja.o
task_config.o: task_config.cpp
	g++ -g -c ./task_config.cpp -o ./task_config.o
./Stage1/stage1main.o: ./Stage1/stage1main.cpp
	g++ -g -c ./Stage1/stage1main.cpp -o ./Stage1/stage1main.o
./Stage2/stage2main.o: ./Stage2/stage2main.cpp
	g++ -g -c ./Stage2/stage2main.cpp -o ./Stage2/stage2main.o
./Stage3/stage3main.o: ./Stage3/stage3main.cpp
	g++ -g -c ./Stage3/stage3main.cpp -o ./Stage3/stage3main.o
./Stage4/stage4main.o: ./Stage4/stage4main.cpp
	g++ -g -c ./Stage4/stage4main.cpp -o ./Stage4/stage4main.o
./Stage5/stage5main.o: ./Stage5/stage5main.cpp
	g++ -g -c ./Stage5/stage5main.cpp -o ./Stage5/stage5main.o
./Stage6/stage6main.o: ./Stage6/stage6main.cpp
	g++ -g -c ./Stage6/stage6main.cpp -o ./Stage6/stage6main.o
./Stage7/stage7main.o: ./Stage7/stage7main.cpp
	g++ -g -c ./Stage7/stage7main.cpp -o ./Stage7/stage7main.o
./Router/router.o: ./Router/Router.cpp
	g++ -g -c ./Router/Router.cpp -o ./Router/router.o
./Router/OctaneManager.o: ./Router/OctaneManager.cpp
	g++ -g -c ./Router/OctaneManager.cpp -o ./Router/OctaneManager.o
./Router/PrimaryRouter.o: ./Router/PrimaryRouter.cpp
	g++ -g -c ./Router/PrimaryRouter.cpp -o ./Router/PrimaryRouter.o
./Router/SecondaryRouter.o: ./Router/SecondaryRouter.cpp
	g++ -g -c ./Router/SecondaryRouter.cpp -o ./Router/SecondaryRouter.o
./GlobalVar.o: ./GlobalVar.cpp
	g++ -g -c ./GlobalVar.cpp -o ./GlobalVar.o
./Router/sample_tunnel.o: ./Router/sample_tunnel.c
	g++ -g -c ./Router/sample_tunnel.c -o ./Router/sample_tunnel.o
./Datagram/icmp_checksum.o: ./Datagram/icmp_checksum.c
	g++ -g -c ./Datagram/icmp_checksum.c -o ./Datagram/icmp_checksum.o
./Datagram/IPDatagram.o: ./Datagram/IPDatagram.cpp
	g++ -g -c ./Datagram/IPDatagram.cpp -o ./Datagram/IPDatagram.o
./timers/tools.o: ./timers/tools.cc ./timers/tools.hh
	g++ -g -c ./timers/tools.cc -o ./timers/tools.o
./timers/timers.o: ./timers/timers.cc ./timers/timers.hh
	g++ -g -c -c ./timers/timers.cc -o ./timers/timers.o
clean:
	rm -r ./*.o
	rm -r ./Router/*.o
	rm -r ./Stage1/*.o
	rm -r ./Stage2/*.o
	rm -r ./Stage3/*.o
	rm -r ./Stage4/*.o
	rm -r ./Stage5/*.o
	rm -r ./Stage6/*.o
	rm -r ./Stage7/*.o
	rm -r ./Datagram/*.o
	rm -r ./timers/*.o
	rm -r ./projb
	rm -r ./*.out
stage1clean:
	rm -r ./Stage1/*.o
stage2clean:
	rm -r ./Stage2/*.o
stage3clean:
	rm -r ./Stage3/*.o

all: proja
proja: proja.o task_config.o ./Stage1/stage1main.o ./Stage2/stage2main.o ./Router/router.o ./GlobalVar.o ./Router/sample_tunnel.o
	g++ -g ./task_config.o ./proja.o ./Stage1/stage1main.o ./Stage2/stage2main.o ./Router/router.o ./Router/sample_tunnel.o ./GlobalVar.o -o ./proja
proja.o: proja.cpp
	g++ -g -c ./proja.cpp -o ./proja.o
task_config.o: task_config.cpp
	g++ -g -c ./task_config.cpp -o ./task_config.o
./Stage1/stage1main.o: ./Stage1/stage1main.cpp
	g++ -g -c ./Stage1/stage1main.cpp -o ./Stage1/stage1main.o
./Stage2/stage2main.o: ./Stage2/stage2main.cpp
	g++ -g -c ./Stage2/stage2main.cpp -o ./Stage2/stage2main.o
./Router/router.o: ./Router/Router.cpp
	g++ -g -c ./Router/Router.cpp -o ./Router/router.o
./GlobalVar.o: ./GlobalVar.cpp
	g++ -g -c ./GlobalVar.cpp -o ./GlobalVar.o
./Router/sample_tunnel.o: ./Router/sample_tunnel.c
	g++ -g -c ./Router/sample_tunnel.c -o ./Router/sample_tunnel.o
clean:
	rm -r ./*.o
	rm -r ./Router/*.o
	rm -r ./Stage1/*.o
	rm -r ./Stage2/*.o
	rm -r ./proja
	rm -r ./*.out
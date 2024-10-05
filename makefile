all:
	g++ -fpermissive -std=c++17 -march=native -w -O3 *.cpp -o perft
debug:
	g++ -fpermissive -std=c++17 -march=native -w -g *.cpp -o debug
clean:
	rm -f *~ perft


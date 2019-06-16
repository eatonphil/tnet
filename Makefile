bin/tnet: src/tnet/*.c include/tnet/*.h
	mkdir -p bin
	g++ -std=c++17 -I./include -Wall -Werror -o $@ src/tnet/*.c

dev:
	docker build -t tnet .
	docker run -it --cap-add=NET_ADMIN -v ${CURDIR}:/tnet tnet sh

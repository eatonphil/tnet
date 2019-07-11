bin/tnet: src/tnet/*.c include/tnet/*.h
	mkdir -p bin
	gcc -rdynamic -I./include -Wall -Werror -o $@ src/tnet/*.c

dev:
	docker build -t tnet .
	docker run -it --cap-add=NET_ADMIN -v ${CURDIR}:/tnet tnet sh

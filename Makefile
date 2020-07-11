CC_FLAGS = -I../include -Werror -ggdb -O2 -fPIC -fno-strict-aliasing -pthread
LD_FLAGS = -pthread -lrt

all:
	mkdir -p output
	cd output
	g++ $(CC_FLAGS) -c ../src/*.cpp
	ar libcoc_fiber.a *.o
	g++ $(CC_FLAGS) -o echo_svr ../test/echo_svr/*.cpp $(LD_FLAGS)

clean:
	rm -rf output

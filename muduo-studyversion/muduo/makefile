CXX = g++
CXXFLAGS := -Wall  -std=c++0x
SRCNET := $(wildcard ./net/*.cpp)
OBJNET := $(patsubst %.cpp,%.o,$(SRCNET))
SRCBASE := $(wildcrad ./base/*.cpp)
OBJBASE := $(patsubst %.cpp,%.o,$(SRCBASE))
SRCNET_POLLER := $(wildcard ./net/poller/*.cpp)
OBJNET_POLLER := $(patsubcard %.cpp,%.o,$(SRCNET_POLLER))
LIB = libmuduo.a
AR := ar

$(LIB): $(OBJNET) $(OBJBASE) $(OBJNET_POLLER)
	$(AR) -cr $@ -o $^

.PHONY : clean

clean:
	rm -f ./base/*.o ./net/*.o ./net/poller/*.o *.a


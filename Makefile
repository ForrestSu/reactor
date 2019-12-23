

CXXFLAGS = -std=c++17 -g -Wall -O2 -MD -MT $@ -MP -flto

tests = test-reactor

# all: reactor $(tests)
all: reactor


reactor: main.o reactor.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test-reactor: test-reactor.o reactor.o
	$(CXX) $(CXXFLAGS) -o $@ $^

-include *.d

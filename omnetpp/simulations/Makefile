TARGET = scm-simulations
SRCS = src/Main.cc src/SCMNode.cc src/SCMMessages.cc src/SCMFaultInjector.cc src/TopologyBuilders.cc src/TwitchNetworkInitializer.cc

# Derive paths from OMNETPP_ROOT (set by sourcing setenv)
OMNETPP_INCL_DIR ?= $(OMNETPP_ROOT)/include
OMNETPP_LIB_DIR  ?= $(OMNETPP_ROOT)/lib

CXXFLAGS += -std=c++17
LDFLAGS  += -loppenvir -loppsim -lssl -lcrypto

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -I. -Isrc -I$(OMNETPP_INCL_DIR) -o $@ $^ \
	-L$(OMNETPP_LIB_DIR) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o

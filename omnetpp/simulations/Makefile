TARGET = scm-simulations
SRCS = src/Main.cc src/SCMNode.cc src/SCMMessages.cc src/SCMFaultInjector.cc src/TopologyBuilders.cc src/TwitchNetworkInitializer.cc

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -I. -I$(OMNETPP_INCL_DIR) -o $@ $^ \
	-L$(OMNETPP_LIB_DIR) -loppenvir -loppsim

clean:
	rm -f $(TARGET) *.o
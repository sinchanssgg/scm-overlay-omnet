TARGET = scm-simulations
SRCS = src/main.cc src/SCMNode.cc src/SCMMessages.cc src/SCMFaultInjector.cc

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -I. -I$(OMNETPP_INCL_DIR) -o $@ $^ \
	-L$(OMNETPP_LIB_DIR) -loppenvir -loppsim

clean:
	rm -f $(TARGET) *.o
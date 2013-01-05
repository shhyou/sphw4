CXX       = g++
CXXFLAGS  = -std=c++11 -Wall -Wshadow -Wextra
DEBUG     =
HEADERS   = pipe.h subproc.h watcher.h task.h log.h
OBJS      = pipe.o subproc.o watcher.o task.o log.o
CPPS      = mrg.cpp srt.cpp pipe.cpp subproc.cpp watcher.cpp task.cpp log.cpp
TARGET    = mergesort sorting

.PHONY: all clean
.SUFFIXES:

all: $(TARGET) $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)

%.o: %.cpp $(HEADERS)
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(DEBUG)

sorting: srt.cpp
	$(CXX) -o $@ $< $(OBJS) $(CXXFLAGS) $(DEBUG)

mergesort: mrg.cpp $(OBJS) $(HEADERS)
	$(CXX) -o $@ $< $(OBJS) $(CXXFLAGS) $(DEBUG)

%: %.cpp $(OBJS) $(HEADERS)
	$(CXX) -o $@ $< $(OBJS) $(CXXFLAGS) $(DEBUG)


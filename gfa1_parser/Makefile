BUILDDIR = bin
OBJDIR = build
SRCDIR = src
EXECNAME = gfa1_parser

CXX = g++
CXXFLAGS = -std=c++11 -march=native -O2 -pedantic -Wall -Iinclude/gfa1_parser -Iinclude/containers
LDFLAGS = 

SOURCES = $(shell find $(SRCDIR) -type f -name *.cpp)
OBJECTS = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.cpp=.o))
DEPENDS = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.cpp=.d))

.PHONY: all clean

all: $(EXECNAME)

clean:
	\rm $(OBJDIR)/*.o $(OBJDIR)/*.d $(BUILDDIR)/$(EXECNAME)

-include $(DEPENDS)

$(EXECNAME): $(OBJECTS)
	mkdir -p $(BUILDDIR); $(CXX) $(LDFLAGS) $(OBJECTS) -o $(BUILDDIR)/$(EXECNAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(OBJDIR); $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

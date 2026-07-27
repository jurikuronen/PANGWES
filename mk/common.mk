COMMONDIR         := $(ROOTDIR)/common
COMMONINCLUDEDIR  ?= $(COMMONDIR)/include
COMMONSRCDIR      ?= $(COMMONDIR)/src
COMMONTESTDIR     ?= $(COMMONDIR)/tests

ifeq ($(PROJECT), common)
TESTEXTRASOURCES  :=
TESTEXTRAOBJECTS  :=
else
# Test-only source that must be linked into every test executable.
TESTEXTRASOURCES  := $(COMMONSRCDIR)/test_harness/Test.cpp
TESTEXTRAOBJECTS  := $(OBJDIR)/tests/common/test_harness/Test_test.o
endif

COMMONSOURCES     := $(filter-out $(TESTEXTRASOURCES), $(shell find $(COMMONSRCDIR) -type f -name "*.cpp"))
# Keep common objects separate from project objects.
COMMONOBJECTS     := $(patsubst $(COMMONSRCDIR)/%, $(OBJDIR)/src/common/%, $(COMMONSOURCES:.cpp=.o))
COMMONDEPENDS     := $(COMMONOBJECTS:.o=.d)

RUNTESTS          := $(COMMONDIR)/scripts/run_unit_and_integration_tests.sh

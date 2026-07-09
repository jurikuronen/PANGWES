PROJECTSOURCES    := $(filter-out $(TESTEXTRASOURCES), $(shell find $(SRCDIR) -type f -name "*.cpp"))
PROJECTOBJECTS    := $(patsubst $(SRCDIR)/%, $(OBJDIR)/src/%, $(PROJECTSOURCES:.cpp=.o))
PROJECTDEPENDS    := $(PROJECTOBJECTS:.o=.d)

ifeq ($(PROJECT), common)
SOURCES           := $(PROJECTSOURCES)
OBJECTS           := $(PROJECTOBJECTS)
DEPENDS           := $(PROJECTDEPENDS)
else
SOURCES           := $(PROJECTSOURCES) $(COMMONSOURCES)
OBJECTS           := $(PROJECTOBJECTS) $(COMMONOBJECTS)
DEPENDS           := $(PROJECTDEPENDS) $(COMMONDEPENDS)
endif

TESTSOURCES       := $(shell find $(TESTDIR) -type f -name "*.cpp")
TESTOBJECTS       := $(filter-out $(OBJDIR)/tests/main_test.o, \
                                  $(patsubst $(OBJDIR)/src/%, \
                                             $(OBJDIR)/tests/%, \
                                             $(OBJECTS:.o=_test.o)))
# Defined for TESTDEPENDS so test-source header dependencies trigger rebuilds.
TESTSOURCEOBJECTS := $(patsubst $(TESTDIR)/%.cpp, $(OBJDIR)/tests/%_test.o, $(TESTSOURCES))
TESTDEPENDS       := $(TESTSOURCEOBJECTS:.o=.d) $(TESTOBJECTS:.o=.d) $(TESTEXTRAOBJECTS:.o=.d)

TESTEXECS         := $(patsubst $(TESTDIR)/%.cpp, $(TESTBINDIR)/%, $(TESTSOURCES))
UNITTESTS         := $(filter $(TESTBINDIR)/unit/%, $(TESTEXECS))
INTEGRATIONTESTS  := $(filter $(TESTBINDIR)/integration/%, $(TESTEXECS))

# Use `make check TESTS="test1 ... testN"` to run a subset of tests.
ifneq ($(origin TESTS), undefined)
ALLTESTS          := $(foreach test, $(TESTS), $(filter %/$(test), $(TESTEXECS)))
else
# Ensure unit tests are run before integration tests.
ALLTESTS          := $(UNITTESTS) $(INTEGRATIONTESTS)
endif

# Prevent make from deleting intermediate files.
.SECONDARY:

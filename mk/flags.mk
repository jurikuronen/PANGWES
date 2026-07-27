# Define CXXFLAGS contexts.
CXXCOMMONFLAGS    := -std=c++11 -pthread -pedantic -Wall -I$(INCLUDEDIR)
CXXDEBUGFLAGS     := -fno-omit-frame-pointer -g -O0
CXXTESTMACROS     := -DTEST_DATA_DIR=\"$(TESTDATADIR)\"
CXXDEVEXTRAFLAGS  := -Werror -pedantic-errors -Wmissing-declarations -Wswitch-default -Wcast-align -Wcast-qual \
                    -Wold-style-cast -Woverloaded-virtual -Wdisabled-optimization -Wstrict-overflow=2 -Wlogical-op \
                    -Winit-self -Wnoexcept -Wmissing-include-dirs -Wsign-promo -Wzero-as-null-pointer-constant \
                    -Wshadow -Wredundant-decls

ifneq ($(PROJECT), common)
CXXCOMMONFLAGS    +=-I$(COMMONINCLUDEDIR)
endif

# Build flags and information for the main executable.
CXX               ?= g++
CXXFLAGS          ?= $(CXXCOMMONFLAGS) -march=native -O2 -DNDEBUG
LDFLAGS           ?= -pthread

# Build flags for test executables.
CXXTESTFLAGS      ?= $(CXXCOMMONFLAGS) $(CXXDEBUGFLAGS) $(CXXTESTMACROS) -I$(TESTDIR)
LDTESTFLAGS       ?= $(LDFLAGS)

ifneq ($(PROJECT), common)
CXXTESTFLAGS      += -I$(COMMONTESTDIR)
endif

# Extra flags and options for sanitizer builds.
CXXASANFLAGS      := -fsanitize=address
ASANOPTIONS       := halt_on_error=1:exitcode=1:symbolize=1:detect_leaks=1:leak_check_at_exit=1
LDASANFLAGS       := $(LDFLAGS) -fsanitize=address

CXXTSANFLAGS      := -fsanitize=thread
TSANOPTIONS       := halt_on_error=1:exitcode=1:symbolize=1:detect_deadlocks=1
LDTSANFLAGS       := $(LDFLAGS) -fsanitize=thread

CXXUBSANFLAGS     := -fsanitize=undefined
UBSANOPTIONS      := halt_on_error=1:exitcode=1:symbolize=1
LDUBSANFLAGS      := $(LDFLAGS) -fsanitize=undefined

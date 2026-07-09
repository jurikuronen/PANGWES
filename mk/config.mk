.DEFAULT_GOAL     := all

TARGET            ?= release

# Relative paths (make is invoked in the project directory).
ROOTDIR           := ..
PROJECTDIR        := .

BINDIR            := $(ROOTDIR)/bin
BUILDDIR          := $(ROOTDIR)/build

# Fixed directory names inside the project directory.
INCLUDEDIR        := $(PROJECTDIR)/include
SRCDIR            := $(PROJECTDIR)/src
TESTDIR           := $(PROJECTDIR)/tests
TESTDATADIR       := $(ROOTDIR)/test_data

# Create separate test binaries for non-release targets (`PROJECT` provided by the project's Makefile).
TESTBINDIR        := $(BINDIR)/tests/$(PROJECT)$(if $(filter release, $(TARGET)),,_$(TARGET))

# Separate object directories by project and target.
OBJDIR            := $(BUILDDIR)/$(PROJECT)/$(TARGET)

# Use a separate executable name for non-release targets.
EXEC               = $(PROJECT)$(if $(filter release, $(TARGET)),,_$(TARGET))

# `check-e2e-%` is omitted because `.PHONY` treats `%` literally; files named like that are not really expected.
.PHONY: all check dev check-dev check-unit check-integration check-e2e clean debug \
        asan check-asan tsan check-tsan ubsan check-ubsan lint

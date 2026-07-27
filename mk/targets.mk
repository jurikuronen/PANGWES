ifeq ($(PROJECT), common)
# `common` project doesn't have an executable to build.
all: ; @:
else
# Build target for the main executable; called automatically with `make`.
all: $(BINDIR)/$(EXEC)
endif

# Clean target that removes the build and bin directories.
clean:
	@if [ -d "$(BUILDDIR)/" ]; then \
		\find "$(BUILDDIR)/" -type f -delete; \
		\find "$(BUILDDIR)/" -type d -empty -delete; \
	fi
	@if [ -d "$(BINDIR)" ]; then \
		\find "$(BINDIR)/" -type f -delete; \
		\find "$(BINDIR)/" -type d -empty -delete; \
	fi

# Combined unit and integration tests target.
check: $(ALLTESTS)
	@$(RUNTESTS) "$(EXEC) unit and integration tests" $(ALLTESTS)

# Unit tests target.
check-unit: $(UNITTESTS)
	@$(RUNTESTS) "$(EXEC) unit tests" $(UNITTESTS)

# Integration tests target.
check-integration: $(INTEGRATIONTESTS)
	@$(RUNTESTS) "$(EXEC) integration tests" $(INTEGRATIONTESTS)

# Main end-to-end tests target; uses the debug build for the main executable.
check-e2e: check-e2e-debug

ifeq ($(PROJECT),common)
# `common` project has no end-to-end tests.
check-e2e-%: ; @:
else
# Generic end-to-end tests target; supports: check-e2e-debug, check-e2e-asan, check-e2e-tsan, check-e2e-ubsan.
check-e2e-%: TARGET=$*
check-e2e-%:
	@$(MAKE) $(TARGET)
	@echo "Running $(EXEC) end-to-end tests."
	@env \
		$(if $(filter asan,$(TARGET)),ASAN_OPTIONS='$(ASANOPTIONS)') \
		$(if $(filter tsan,$(TARGET)),TSAN_OPTIONS='$(TSANOPTIONS)') \
		$(if $(filter ubsan,$(TARGET)),UBSAN_OPTIONS='$(UBSANOPTIONS)') \
		$(RUNE2ETESTS) "$(BINDIR)/$(EXEC)" "$(TESTRUNNERSCRIPT)" "$(TESTDATADIR)"
endif

# Debug build target for the main executable; compiles with asserts and extra debugging information.
debug:
	@$(MAKE) TARGET=debug all CXXFLAGS='$(CXXCOMMONFLAGS) $(CXXDEBUGFLAGS)'

# Development build target for the main executable; compiles with extra flags.
dev:
	@$(MAKE) TARGET=dev all CXXFLAGS='$(CXXFLAGS) $(CXXDEVEXTRAFLAGS)'

# Development build target for unit and integration tests; compiles with extra flags.
check-dev:
	@$(MAKE) TARGET=dev check CXXTESTFLAGS='$(CXXTESTFLAGS) $(CXXDEVEXTRAFLAGS)'

# AddressSanitizer build target for the main executable.
asan:
	@$(MAKE) TARGET=asan all CXXFLAGS='$(CXXCOMMONFLAGS) $(CXXDEBUGFLAGS) $(CXXASANFLAGS)' LDFLAGS='$(LDASANFLAGS)'

# AddressSanitizer build target for unit and integration tests.
check-asan:
	@ASAN_OPTIONS='$(ASANOPTIONS)' \
	$(MAKE) TARGET=asan check CXXTESTFLAGS='$(CXXTESTFLAGS) $(CXXASANFLAGS)' LDTESTFLAGS='$(LDASANFLAGS)'

# ThreadSanitizer build target for the main executable.
tsan:
	@$(MAKE) TARGET=tsan all CXXFLAGS='$(CXXCOMMONFLAGS) $(CXXDEBUGFLAGS) $(CXXTSANFLAGS)' LDFLAGS='$(LDTSANFLAGS)'

# ThreadSanitizer build target for unit and integration tests.
check-tsan:
	@TSAN_OPTIONS='$(TSANOPTIONS)' \
	$(MAKE) TARGET=tsan check CXXTESTFLAGS='$(CXXTESTFLAGS) $(CXXTSANFLAGS)' LDTESTFLAGS='$(LDTSANFLAGS)'

# UndefinedBehaviorSanitizer build target for the main executable.
ubsan:
	@$(MAKE) TARGET=ubsan all CXXFLAGS='$(CXXCOMMONFLAGS) $(CXXDEBUGFLAGS) $(CXXUBSANFLAGS)' LDFLAGS='$(LDUBSANFLAGS)'

# UndefinedBehaviorSanitizer build target for unit and integration tests.
check-ubsan:
	@UBSAN_OPTIONS='$(UBSANOPTIONS)' \
	$(MAKE) TARGET=ubsan check CXXTESTFLAGS='$(CXXTESTFLAGS) $(CXXUBSANFLAGS)' LDTESTFLAGS='$(LDUBSANFLAGS)'

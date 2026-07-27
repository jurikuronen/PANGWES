-include $(wildcard $(DEPENDS) $(TESTDEPENDS))

# Main executable objects (project sources).
$(OBJDIR)/src/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

ifneq ($(PROJECT), common)
# Main executable objects (common sources).
$(OBJDIR)/src/common/%.o: $(COMMONSRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
endif

ifneq ($(PROJECT), common)
# Link main executable.
$(BINDIR)/$(EXEC): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@
endif

# Test executable objects (project sources).
$(OBJDIR)/tests/%_test.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXTESTFLAGS) -MMD -MP -c $< -o $@

ifneq ($(PROJECT), common)
# Test executable objects (common sources).
$(OBJDIR)/tests/common/%_test.o: $(COMMONSRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXTESTFLAGS) -MMD -MP -c $< -o $@
endif

# Test executable objects (test sources).
$(OBJDIR)/tests/%_test.o: $(TESTDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXTESTFLAGS) -MMD -MP -c $< -o $@

# Test extra objects.
$(TESTEXTRAOBJECTS): $(TESTEXTRASOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXTESTFLAGS) -MMD -MP -c $< -o $@

# Link test executables.
$(TESTBINDIR)/%: $(OBJDIR)/tests/%_test.o $(TESTOBJECTS) $(TESTEXTRAOBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDTESTFLAGS) $^ -o $@

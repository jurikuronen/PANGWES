# Programs and flags for the `make lint` target; requires `clang-tidy`, `shellcheck`, `perlcritic` and `lintr`.
SHELLCHECK        := shellcheck
SHELLSCRIPTS       = $(RUNTESTS) $(RUNE2ETESTS)
CLANGTIDY         := clang-tidy
CLANGTIDYFLAGS    := $(CXXCOMMONFLAGS) $(CXXTESTMACROS) -I$(TESTDIR)
CLANGTIDYCONFIG   := $(ROOTDIR)/.clang-tidy
PERLCRITIC        := perlcritic
PERLCRITICCONFIG  := $(ROOTDIR)/.perlcriticrc
PERLSCRIPTS        = $(ROOTDIR)/plot_scripts/map_unitigs_to_reference.pl

ifneq ($(PROJECT), common)
CLANGTIDYFLAGS    += -I$(COMMONTESTDIR)
PERLSCRIPTS		  += $(TESTRUNNERSCRIPT)
endif

# Target for code quality checks; requires `clang-tidy`, `shellcheck`, `perlcritic` and `lintr`.
lint:
	@$(SHELLCHECK) $(SHELLSCRIPTS)
	@$(CLANGTIDY) $(SOURCES) $(TESTSOURCES) --config-file=$(CLANGTIDYCONFIG) -- $(CLANGTIDYFLAGS)
	@$(PERLCRITIC) --profile $(PERLCRITICCONFIG) $(PERLSCRIPTS)
	@cd $(ROOTDIR) && Rscript -e 'lintr::lint_dir(path = "plot_scripts")'

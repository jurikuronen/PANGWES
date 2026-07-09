# Read version information from the VERSION file.
VERSIONFILE       := $(ROOTDIR)/VERSION

# Stamp file used to track when version information was last updated.
VERSIONSTAMP      := version.stamp

# List of Git files to determine the commit (handles new commits, detached HEAD and ref packing with Git housekeeping).
GITDIR            := $(strip $(shell git rev-parse --git-dir 2>/dev/null))
GITFILES          := $(wildcard $(GITDIR)/refs/heads/* $(GITDIR)/HEAD $(GITDIR)/packed-refs)

PANGWESVERSION    := $(strip $(shell cat $(VERSIONFILE) 2>/dev/null || echo "(unknown version)"))
GITREVSHORT       := $(strip $(shell git rev-parse --short=7 HEAD 2>/dev/null || echo "n/a"))

# Create a version stamp file.
$(BUILDDIR)/$(VERSIONSTAMP): $(VERSIONFILE) $(GITFILES)
	@mkdir -p $(dir $@)
	@tmpfile=$@.tmp; \
    printf '%s\n' '$(PANGWESVERSION)' '$(GITREVSHORT)' > $$tmpfile; \
    if [ ! -f "$@" ] || ! cmp -s "$@" "$$tmpfile"; then mv "$$tmpfile" "$@"; else rm "$$tmpfile"; fi

# Make the main object depend on the version stamp to force recompilation when version information changes.
$(OBJDIR)/src/main.o: $(BUILDDIR)/$(VERSIONSTAMP)
$(OBJDIR)/src/main.o: override CXXFLAGS += -DPANGWES_VERSION=\"$(PANGWESVERSION)\" -DGIT_REV_SHORT=\"$(GITREVSHORT)\"

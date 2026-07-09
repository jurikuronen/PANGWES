PROJECTS          := gfa_parser unitig_distance

.PHONY: all check check-unit check-integration check-e2e clean $(PROJECTS)

# Build all main binaries.
all:
	@set -e; \
	for project in $(PROJECTS); do \
		$(MAKE) -C $$project all; \
	done

# Run unit and integration tests for all projects.
check:
	@set -e; \
	for project in common $(PROJECTS); do \
		$(MAKE) -C $$project check; \
	done

# Run unit tests for all projects.
check-unit:
	@set -e; \
	for project in common $(PROJECTS); do \
		$(MAKE) -C $$project check-unit; \
	done

# Run integration tests for all projects.
check-integration:
	@set -e; \
	for project in common $(PROJECTS); do \
		$(MAKE) -C $$project check-integration; \
	done

# Run end-to-end tests for all projects.
check-e2e:
	@set -e; \
	for project in $(PROJECTS); do \
		$(MAKE) -C $$project check-e2e; \
	done

# It suffices to run the clean target for any project.
clean:
	$(MAKE) -C unitig_distance clean;

# Build a specific project (e.g. `make unitig_distance`).
$(PROJECTS):
	@$(MAKE) -C $@ all

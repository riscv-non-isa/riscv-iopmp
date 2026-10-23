# Directories
BIN_DIR  := bin
SRC_DIR  := src
VERIF    := verif
TEST_DIR := $(VERIF)/tests
LIB_DIR  := lib
LIBIOPMP_DIR := ../libiopmp
LIBIOPMP     := $(LIBIOPMP_DIR)/build/lib/libiopmp.a

cov =
COVFLAGS =
ifeq ($(cov), 1)
COVFLAGS += -fprofile-arcs -ftest-coverage -lgcov --coverage
endif

asan =
ASAN_CFLAGS =
ASAN_LDFLAGS =
ifeq ($(asan), 1)
ASAN_CFLAGS += -fsanitize=address -fno-omit-frame-pointer -O1 -g3
ASAN_LDFLAGS += -lasan
endif

libiopmp =
LIBIOPMP_CFLAGS =
LIBIOPMP_LDFLAGS =
LIBIOPMP_PREREQ =
ifeq ($(libiopmp), 1)
# Enable coverage analysis
cov = 1
LIBIOPMP_CFLAGS += -I$(LIBIOPMP_DIR)/include -I$(LIBIOPMP_DIR)/src
LIBIOPMP_LDFLAGS += $(LIBIOPMP) -lgcov
# Prerequisite
LIBIOPMP_PREREQ += $(LIBIOPMP)
endif

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Werror $(COVFLAGS) $(ASAN_FLAGS) $(LIBIOPMP_CFLAGS) -I./include -Iverif/ -fPIC
LDFLAGS := -lm $(ASAN_LDFLAGS) $(LIBIOPMP_LDFLAGS)

# Common source files
COMMON_SOURCES := $(SRC_DIR)/iopmp_reg.c \
                  $(SRC_DIR)/iopmp_interrupt.c \
                  $(SRC_DIR)/iopmp_rule_analyzer.c \
                  $(SRC_DIR)/iopmp_validate.c \
                  $(SRC_DIR)/iopmp_error_report.c \
                  $(VERIF)/test_utils.c

# Models and configurations
ifneq ($(libiopmp),1)
MODELS := full_model:fullmodel.c \
          rapid_k_model:rapidmodel.c \
          dynamic_k_model:dynamicmodel.c \
          isolation_model:isolationmodel.c \
          compact_k_model:compactmodel.c \
          unnamed_model_1:unnamed_model_1.c \
          unnamed_model_2:unnamed_model_2.c \
          unnamed_model_3:unnamed_model_3.c \
          unnamed_model_4:unnamed_model_4.c
else
MODELS := libiopmp_INFO:libiopmp_INFO.c \
          libiopmp_full_model:libiopmp_fullmodel.c \
          libiopmp_isolation:libiopmp_isolation.c \
          libiopmp_compact_k:libiopmp_compact_k.c \
          libiopmp_srcmd_fmt_1_mdcfg_fmt_2:libiopmp_srcmd_fmt_1_mdcfg_fmt_2.c \
          libiopmp_rapid_k:libiopmp_rapid_k.c \
          libiopmp_dynamic_k:libiopmp_dynamic_k.c \
          libiopmp_srcmd_fmt_2_mdcfg_fmt_0:libiopmp_srcmd_fmt_2_mdcfg_fmt_0.c \
          libiopmp_srcmd_fmt_2_mdcfg_fmt_1:libiopmp_srcmd_fmt_2_mdcfg_fmt_1.c \
          libiopmp_srcmd_fmt_2_mdcfg_fmt_2:libiopmp_srcmd_fmt_2_mdcfg_fmt_2.c
endif

# Targets
.PHONY: all build run clean

all: build run

# Ensure the directories exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# Build all models into libraries and executables
build: $(BIN_DIR) $(LIB_DIR) $(LIBIOPMP_PREREQ)
	@echo "Building library..."; \
	obj_files=""; \
	for src in $(COMMON_SOURCES); do \
		obj_file=$$(basename $$src .c).o; \
		$(CC) $(CFLAGS) -c $$src -o $$obj_file; \
		obj_files="$$obj_files $$obj_file"; \
	done; \
	if [ -n "$(sharedLib)" ]; then \
		$(CC) $$obj_files -shared -o $(LIB_DIR)/iopmp_model.so; \
	else \
		ar rcs $(LIB_DIR)/iopmp_model.a $$obj_files > /dev/null 2>&1; \
		ranlib $(LIB_DIR)/iopmp_model.a; \
	fi; \
	echo "Library built successfully."; \
	rm -f $$obj_files;
	@if [ -z "$(model)" ]; then \
		echo "Building all models..."; \
		for entry in $(MODELS); do \
			model=$$(echo $$entry | cut -d':' -f1); \
			source=$$(echo $$entry | cut -d':' -f2); \
			echo "Building executable for $$model..."; \
			if [ -n "$(sharedLib)" ]; then \
				$(CC) $(CFLAGS) $(TEST_DIR)/$$source -L$(LIB_DIR) -l:iopmp_model.so -o $(BIN_DIR)/$$model $(LDFLAGS); \
			else \
				$(CC) $(CFLAGS) $(TEST_DIR)/$$source -L$(LIB_DIR) -l:iopmp_model.a -o $(BIN_DIR)/$$model $(LDFLAGS); \
			fi; \
			echo "Executable for $$model built successfully."; \
		done; \
	else \
		entry=$$(echo "$(MODELS)" | tr ' ' '\n' | grep -E "^$(model):"); \
		if [ -z "$$entry" ]; then \
			echo "Error: Model $(model) not found."; \
			exit 1; \
		fi; \
		source=$$(echo $$entry | cut -d':' -f2); \
		echo "Building executable for $(model)..."; \
		if [ -n "$(sharedLib)" ]; then \
			$(CC) $(CFLAGS) $(TEST_DIR)/$$source -L$(LIB_DIR) -l:iopmp_model.so -o $(BIN_DIR)/$(model) $(LDFLAGS); \
		else \
			$(CC) $(CFLAGS) $(TEST_DIR)/$$source -L$(LIB_DIR) -l:iopmp_model.a -o $(BIN_DIR)/$(model) $(LDFLAGS); \
		fi; \
		echo "Executable for $(model) built successfully."; \
	fi

# Run all compiled binaries. Exits non-zero if any of them fails or is missing,
# so that this target can be used as a regression gate.
run:
	@failed=""; \
	for model in $(MODELS); do \
		target=$${model%%:*}; \
		binary=$(BIN_DIR)/$$target; \
		if [ -f $$binary ]; then \
			echo "Running $$binary..."; \
			if ! $$binary; then failed="$$failed $$target"; fi; \
		else \
			echo "Binary $$binary not found. Did you build it?"; \
			failed="$$failed $$target"; \
		fi; \
	done; \
	if [ -n "$$failed" ]; then \
		echo "FAILED:$$failed"; \
		exit 1; \
	fi; \
	echo "All binaries passed."

# Display usage information
.PHONY: help

help:
	@echo "Usage: make [TARGET] [VARIABLE=value]"
	@echo ""
	@echo "Targets:"
	@echo "  all           Build all models and run their binaries"
	@echo "  build         Compile all models or a specific model"
	@echo "                Use 'model=<model_name>' to specify a model"
	@echo "  run           Run all compiled binaries"
	@echo "                Ensure the models are built before running"
	@echo "  clean         Remove all compiled binaries, libraries, and coverage files"
	@echo "  help          Display this usage information"
	@echo ""
	@echo "Variables:"
	@echo "  model=<model_name>    Build a specific model instead of all models"
	@echo "                        Example: make build model=full_model"
	@echo "  sharedLib=1           Build shared libraries instead of static ones"
	@echo "  cov=1                 Enable code coverage during compilation"
	@echo "  asan=1                Enable AddressSanitizer"
	@echo "  libiopmp=1            Build libiopmp test program"
	@echo ""
	@echo "Available Models:"
	@echo "  full_model            SRCMD_FMT = 0, MDCFG_FMT = 0"
	@echo "  rapid_k_model         SRCMD_FMT = 0, MDCFG_FMT = 1"
	@echo "  dynamic_k_model       SRCMD_FMT = 0, MDCFG_FMT = 2"
	@echo "  isolation_model       SRCMD_FMT = 1, MDCFG_FMT = 0"
	@echo "  compact_k_model       SRCMD_FMT = 1, MDCFG_FMT = 1"
	@echo "  unnamed_model_1       SRCMD_FMT = 1, MDCFG_FMT = 2"
	@echo "  unnamed_model_2       SRCMD_FMT = 2, MDCFG_FMT = 0"
	@echo "  unnamed_model_3       SRCMD_FMT = 2, MDCFG_FMT = 1"
	@echo "  unnamed_model_4       SRCMD_FMT = 2, MDCFG_FMT = 2"
	@echo ""
	@echo "Available libiopmp test programs:"
	@echo "  libiopmp_INFO                     SRCMD_FMT = 0, MDCFG_FMT = 0"
	@echo "  libiopmp_full_model               SRCMD_FMT = 0, MDCFG_FMT = 0"
	@echo "  libiopmp_rapid_k                  SRCMD_FMT = 0, MDCFG_FMT = 1"
	@echo "  libiopmp_dynamic_k                SRCMD_FMT = 0, MDCFG_FMT = 2"
	@echo "  libiopmp_isolation                SRCMD_FMT = 1, MDCFG_FMT = 0"
	@echo "  libiopmp_compact_k                SRCMD_FMT = 1, MDCFG_FMT = 1"
	@echo "  libiopmp_srcmd_fmt_1_mdcfg_fmt_2  SRCMD_FMT = 1, MDCFG_FMT = 2"
	@echo "  libiopmp_srcmd_fmt_2_mdcfg_fmt_0  SRCMD_FMT = 2, MDCFG_FMT = 0"
	@echo "  libiopmp_srcmd_fmt_2_mdcfg_fmt_1  SRCMD_FMT = 2, MDCFG_FMT = 1"
	@echo "  libiopmp_srcmd_fmt_2_mdcfg_fmt_2  SRCMD_FMT = 2, MDCFG_FMT = 2"
	@echo ""

# Clean the binary and library directories
clean:
	rm -rf bin/ lib/ *.gcno *.gcda

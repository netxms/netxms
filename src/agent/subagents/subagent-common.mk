#
# subagent-common.mk - shared build rules for NetXMS subagents (Windows/MinGW)
# Part of NetXMS project
#
# A subagent Makefile.w32 defines the following before including this file:
#   SUBAGENT      - base name; produces $(BIN_DIR)/<SUBAGENT>.nsm
#   SOURCES       - list of .cpp / .c source files
#   SA_LIBS       - libraries in addition to -lnetxms -lnxagent (optional)
#   SA_CPPFLAGS   - extra compiler flags, e.g. SDK include paths (optional)
#   SA_RC         - subagent-specific resource script embedding runtime data
#                   such as parser XML (optional; compiled and linked in
#                   addition to the version-info resource)
#   GENERATED_SOURCES - files produced by an in-tree generator (flex/bison, etc.)
#                   that 'clean' should also remove (optional). The rules that
#                   produce them go in the subagent's own Makefile.w32.
# then:
#   include $(TOPDIR)/src/agent/subagents/subagent-common.mk
#
# Subagents are DLLs (renamed .nsm) loaded by the agent; nothing links against
# them, so no import library is produced. In-tree libs are found via
# -L$(LIB_DIR) in COMMON_LIBDIRS.

TARGET = $(BIN_DIR)/$(SUBAGENT).nsm

# Resource file for version info
RC_SOURCE = $(TOPDIR)/build/netxms-build-tag.rc
RC_OBJECT = $(OBJDIR)/netxms-build-tag.o

# Optional subagent-specific resource script (embedded parser XML etc.)
ifdef SA_RC
SA_RC_OBJECT = $(OBJDIR)/$(basename $(SA_RC)).res.o
endif

# Object files (.cpp and .c both map to .o)
# Objects go to a per-architecture directory so multiple ARCH builds coexist
OBJDIR = $(OBJ_DIR)/$(subst $(TOPDIR)/,,$(CURDIR))
OBJECTS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SOURCES))))

CFLAGS = $(COMMON_CFLAGS) $(SA_CPPFLAGS)
CXXFLAGS = $(COMMON_CXXFLAGS) $(SA_CPPFLAGS)

LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS) -shared

# -lnxagent provides the subagent helper API; -lnetxms the core utilities
LIBS = -lnxagent -lnetxms $(SA_LIBS) $(WIN_LIBS)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS) $(RC_OBJECT) $(SA_RC_OBJECT)
	@mkdir -p $(BIN_DIR)
	@echo "  LD      $@"
	$(Q)$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo "  Built: $(TARGET)"

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "  CXX     $<"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@echo "  CXX     $<"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(RC_OBJECT): $(RC_SOURCE)
	@mkdir -p $(dir $@)
	@echo "  RC      $<"
	$(Q)$(WINDRES) -I$(TOPDIR)/include -I$(TOPDIR)/build $< -o $@

ifdef SA_RC
# -I. so windres finds resource.h and the referenced data files (parser XML),
# which are named relative to the subagent directory.
$(SA_RC_OBJECT): $(SA_RC)
	@mkdir -p $(dir $@)
	@echo "  RC      $<"
	$(Q)$(WINDRES) -I. -I$(TOPDIR)/include -I$(TOPDIR)/build $< -o $@
endif

clean:
	rm -f $(OBJECTS) $(RC_OBJECT) $(SA_RC_OBJECT) $(TARGET) $(GENERATED_SOURCES) .depend

install: $(TARGET)
	@echo "  INSTALL $(TARGET)"
	mkdir -p "$(PREFIX)/lib/netxms"
	cp $(TARGET) "$(PREFIX)/lib/netxms/"

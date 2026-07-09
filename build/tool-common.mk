#
# tool-common.mk - shared build rules for NetXMS console-tool executables
# (Windows/MinGW). Used by both agent tools and client command-line tools.
# Part of NetXMS project
#
# A tool Makefile.w32 defines:
#   TOOL          - base name; produces $(BIN_DIR)/<TOOL>.exe
#   SOURCES       - list of .cpp / .c source files
#   TOOL_LIBS     - libraries to link (in addition to $(WIN_LIBS)), e.g. -lnetxms
#   TOOL_CPPFLAGS - extra compiler flags (optional)
#   TOOL_LDFLAGS  - extra linker flags, e.g. -municode for a _tmain entry (optional)
#   GENERATED_SOURCES - files produced by an in-tree generator (flex/bison, etc.)
#                   that 'clean' should also remove (optional). The rules that
#                   produce them go in the tool's own Makefile.w32.
# then:
#   include $(TOPDIR)/build/tool-common.mk
#
# In-tree libs are found via -L$(LIB_DIR) in COMMON_LIBDIRS.

TARGET = $(BIN_DIR)/$(TOOL).exe

RC_SOURCE = $(TOPDIR)/build/netxms-build-tag.rc
RC_OBJECT = $(OBJDIR)/netxms-build-tag.o

# Objects go to a per-architecture directory so multiple ARCH builds coexist
OBJDIR = $(OBJ_DIR)/$(subst $(TOPDIR)/,,$(CURDIR))
OBJECTS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SOURCES))))

CFLAGS = $(COMMON_CFLAGS) $(TOOL_CPPFLAGS)
CXXFLAGS = $(COMMON_CXXFLAGS) $(TOOL_CPPFLAGS)

# Console executable - no -shared. TOOL_LDFLAGS lets a tool add flags such as
# -municode (needed when its entry point is _tmain/wmain rather than main).
LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS) $(TOOL_LDFLAGS)
LIBS = $(TOOL_LIBS) $(WIN_LIBS)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS) $(RC_OBJECT)
	@mkdir -p $(BIN_DIR)
	@echo "  LD      $@"
	$(Q)$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo "  Built: $(TARGET)"

$(OBJDIR)/%.o: %.cpp
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

clean:
	rm -f $(OBJECTS) $(RC_OBJECT) $(TARGET) $(GENERATED_SOURCES) .depend

install: $(TARGET)
	@echo "  INSTALL $(TARGET)"
	mkdir -p "$(PREFIX)/bin"
	cp $(TARGET) "$(PREFIX)/bin/"

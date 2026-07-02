#
# tool-common.mk - shared build rules for NetXMS agent tools (Windows/MinGW)
# Part of NetXMS project
#
# A tool Makefile.w32 defines:
#   TOOL          - base name; produces $(BIN_DIR)/<TOOL>.exe
#   SOURCES       - list of .cpp / .c source files
#   TOOL_LIBS     - libraries to link (in addition to $(WIN_LIBS)), e.g. -lnetxms
#   TOOL_CPPFLAGS - extra compiler flags (optional)
# then:
#   include $(TOPDIR)/src/agent/tools/tool-common.mk
#
# In-tree libs are found via -L$(LIB_DIR) in COMMON_LIBDIRS.

TARGET = $(BIN_DIR)/$(TOOL).exe

RC_SOURCE = $(TOPDIR)/build/netxms-build-tag.rc
RC_OBJECT = netxms-build-tag.o

OBJECTS = $(addsuffix .o,$(basename $(SOURCES)))

CFLAGS = $(COMMON_CFLAGS) $(TOOL_CPPFLAGS)
CXXFLAGS = $(COMMON_CXXFLAGS) $(TOOL_CPPFLAGS)

# Console executable - no -shared
LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS)
LIBS = $(TOOL_LIBS) $(WIN_LIBS)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS) $(RC_OBJECT)
	@mkdir -p $(BIN_DIR)
	@echo "  LD      $@"
	$(Q)$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo "  Built: $(TARGET)"

%.o: %.cpp
	@echo "  CXX     $<"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(RC_OBJECT): $(RC_SOURCE)
	@echo "  RC      $<"
	$(Q)$(WINDRES) -I$(TOPDIR)/include -I$(TOPDIR)/build $< -o $@

clean:
	rm -f $(OBJECTS) $(RC_OBJECT) $(TARGET) .depend

install: $(TARGET)
	@echo "  INSTALL $(TARGET)"
	mkdir -p "$(PREFIX)/bin"
	cp $(TARGET) "$(PREFIX)/bin/"

#
# driver-common.mk - shared build rules for NetXMS database drivers (Windows/MinGW)
# Part of NetXMS project
#
# A driver Makefile.w32 defines the following before including this file:
#   DRIVER        - base name; produces $(BIN_DIR)/<DRIVER>.ddr
#   SOURCES       - list of .cpp source files
#   DRV_LIBS      - libraries in addition to -lnetxms (optional)
#   DRV_CPPFLAGS  - extra compiler flags, e.g. SDK include paths (optional)
# then:
#   include $(TOPDIR)/src/db/dbdrv/driver-common.mk
#
# Database drivers are DLLs renamed to .ddr, loaded at runtime by libnxdb via
# DBLoadDriver(). Nothing links against them, so no import library is produced.
# In-tree libs are found via -L$(LIB_DIR) in COMMON_LIBDIRS.

TARGET = $(BIN_DIR)/$(DRIVER).ddr

# Resource file for version info
RC_SOURCE = $(TOPDIR)/build/netxms-build-tag.rc
RC_OBJECT = netxms-build-tag.o

OBJECTS = $(addsuffix .o,$(basename $(SOURCES)))

CXXFLAGS = $(COMMON_CXXFLAGS) $(DRV_CPPFLAGS)

LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS) -shared

# -lnetxms provides the core utilities used by every driver
LIBS = -lnetxms $(DRV_LIBS) $(WIN_LIBS)

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

$(RC_OBJECT): $(RC_SOURCE)
	@echo "  RC      $<"
	$(Q)$(WINDRES) -I$(TOPDIR)/include -I$(TOPDIR)/build $< -o $@

clean:
	rm -f $(OBJECTS) $(RC_OBJECT) $(TARGET) .depend

install: $(TARGET)
	@echo "  INSTALL $(TARGET)"
	mkdir -p "$(PREFIX)/lib/netxms/dbdrv"
	cp $(TARGET) "$(PREFIX)/lib/netxms/dbdrv/"

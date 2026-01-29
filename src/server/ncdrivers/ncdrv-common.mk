#
# ncdrv-common.mk - shared build rules for NetXMS notification channel drivers
# (Windows/MinGW). Part of NetXMS project.
#
# A driver Makefile.w32 defines the following before including this file:
#   DRIVER        - base name; produces $(BIN_DIR)/<DRIVER>.ncd
#   SOURCES       - list of .cpp source files
#   DRV_LIBS      - libraries in addition to -lnetxms (optional)
#   DRV_CPPFLAGS  - extra compiler flags, e.g. SDK include paths (optional)
# then:
#   include $(TOPDIR)/src/server/ncdrivers/ncdrv-common.mk
#
# NC drivers are DLLs renamed to .ncd, loaded at runtime by the server. Unlike
# server modules they do NOT link against nxcore - they only use the driver API
# from ncdrv.h plus libnetxms. Nothing links against them, so no import library
# is produced. In-tree libs found via -L$(LIB_DIR) in COMMON_LIBDIRS.

TARGET = $(BIN_DIR)/$(DRIVER).ncd

RC_SOURCE = $(TOPDIR)/build/netxms-build-tag.rc
RC_OBJECT = $(OBJDIR)/netxms-build-tag.o

# Objects go to a per-architecture directory so multiple ARCH builds coexist
OBJDIR = $(OBJ_DIR)/$(subst $(TOPDIR)/,,$(CURDIR))
OBJECTS = $(addprefix $(OBJDIR)/,$(SOURCES:.cpp=.o))

CXXFLAGS = $(COMMON_CXXFLAGS) -I$(TOPDIR)/src/server/include $(DRV_CPPFLAGS)

LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS) -shared

LIBS = -lnetxms $(DRV_LIBS) $(WIN_LIBS)

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

$(RC_OBJECT): $(RC_SOURCE)
	@mkdir -p $(dir $@)
	@echo "  RC      $<"
	$(Q)$(WINDRES) -I$(TOPDIR)/include -I$(TOPDIR)/build $< -o $@

clean:
	rm -f $(OBJECTS) $(RC_OBJECT) $(TARGET) .depend

install: $(TARGET)
	@echo "  INSTALL $(TARGET)"
	mkdir -p "$(PREFIX)/lib/netxms/ncd"
	cp $(TARGET) "$(PREFIX)/lib/netxms/ncd/"

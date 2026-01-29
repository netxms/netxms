#
# pdsdrv-common.mk - shared build rules for NetXMS performance data storage
# drivers (Windows/MinGW). Part of NetXMS project.
#
# A driver Makefile.w32 defines the following before including this file:
#   DRIVER        - base name; produces $(BIN_DIR)/<DRIVER>.pdsd
#   SOURCES       - list of .cpp source files
#   DRV_LIBS      - libraries in addition to the base core set (optional)
#   DRV_CPPFLAGS  - extra compiler flags (optional)
# then:
#   include $(TOPDIR)/src/server/pdsdrv/pdsdrv-common.mk
#
# PDS drivers are DLLs renamed to .pdsd, loaded at runtime by the server. They
# import the server API from nxcore (-lnxcore); nothing links against them, so
# no import library is produced. In-tree libs found via -L$(LIB_DIR) in
# COMMON_LIBDIRS.

TARGET = $(BIN_DIR)/$(DRIVER).pdsd

RC_SOURCE = $(TOPDIR)/build/netxms-build-tag.rc
RC_OBJECT = $(OBJDIR)/netxms-build-tag.o

# Objects go to a per-architecture directory so multiple ARCH builds coexist
OBJDIR = $(OBJ_DIR)/$(subst $(TOPDIR)/,,$(CURDIR))
OBJECTS = $(addprefix $(OBJDIR)/,$(SOURCES:.cpp=.o))

CXXFLAGS = $(COMMON_CXXFLAGS) -I$(TOPDIR)/src/server/include $(DRV_CPPFLAGS)

LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS) -shared

LIBS = -lnxcore -lnxsrv -lnetxms $(DRV_LIBS) $(WIN_LIBS)

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
	mkdir -p "$(PREFIX)/lib/netxms/pdsdrv"
	cp $(TARGET) "$(PREFIX)/lib/netxms/pdsdrv/"

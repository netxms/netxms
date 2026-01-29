#
# module-common.mk - shared build rules for NetXMS server modules (Windows/MinGW)
# Part of NetXMS project
#
# A module Makefile.w32 defines the following before including this file:
#   MODULE        - base name; produces $(BIN_DIR)/<MODULE>.nxm
#   SOURCES       - list of .cpp source files (often $(wildcard *.cpp))
#   MOD_LIBS      - libraries in addition to the base core set (optional)
#   MOD_CPPFLAGS  - extra compiler flags (optional)
# then:
#   include $(TOPDIR)/src/server/module-common.mk
#
# Server modules are DLLs renamed to .nxm, loaded at runtime by netxmsd. They
# import the server API from nxcore (-lnxcore); nothing links against them, so
# no import library is produced. In-tree libs are found via -L$(LIB_DIR) in
# COMMON_LIBDIRS.

TARGET = $(BIN_DIR)/$(MODULE).nxm

RC_SOURCE = $(TOPDIR)/build/netxms-build-tag.rc
RC_OBJECT = $(OBJDIR)/netxms-build-tag.o

# Handles both .cpp and .cc (e.g. generated protobuf) sources
# Objects go to a per-architecture directory so multiple ARCH builds coexist
OBJDIR = $(OBJ_DIR)/$(subst $(TOPDIR)/,,$(CURDIR))
OBJECTS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SOURCES))))

# Modules include the heavy server headers (nms_core.h and friends), so they get
# the same include set as nxcore itself.
CXXFLAGS = $(COMMON_CXXFLAGS) \
	-I$(TOPDIR)/src/server/include \
	-I$(TOPDIR)/include/argon2 \
	-I$(TOPDIR)/private/common/src/licensing \
	-I$(MICROHTTPD_ROOT)/include \
	-I$(JNI_ROOT)/include -I$(JNI_ROOT)/include/win32 \
	$(MOD_CPPFLAGS)

LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS) -shared

# -lnxcore provides the exported server API; -lnxsrv/-lnetxms the support libs
LIBS = -lnxcore -lnxsrv -lnetxms $(MOD_LIBS) $(WIN_LIBS)

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

$(OBJDIR)/%.o: %.cc
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
	mkdir -p "$(PREFIX)/lib/netxms"
	cp $(TARGET) "$(PREFIX)/lib/netxms/"

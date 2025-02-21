lib_LTLIBRARIES = libnxsl.la
libnxsl_la_SOURCES = lex.parser.cpp parser.tab.cpp \
		     array.cpp bytestream.cpp class.cpp compiler.cpp env.cpp \
		     file.cpp functions.cpp geolocation.cpp hashmap.cpp \
		     inetaddr.cpp instruction.cpp io.cpp iterator.cpp \
		     json.cpp lexer.cpp library.cpp macaddr.cpp math.cpp \
		     program.cpp range.cpp selectors.cpp storage.cpp \
		     string.cpp table.cpp time.cpp tools.cpp value.cpp \
		     variable.cpp vm.cpp
libnxsl_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNXSL_EXPORTS -I@top_srcdir@/build
libnxsl_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsl_la_LIBADD = ../libnetxms/libnetxms.la

EXTRA_DIST = libnxsl.vcxproj libnxsl.vcxproj.filters libnxsl.h parser.l parser.y parser.tab.hpp windll.cpp

compiler.cpp: parser.tab.hpp

lexer.cpp: parser.tab.hpp lex.parser.cpp

lex.parser.cpp: parser.l parser.tab.hpp parser.tab.cpp
	$(LEX) -B -f -8 -olex.parser.cpp parser.l

parser.tab.hpp: parser.tab.cpp

parser.tab.cpp: parser.y
	$(YACC) -b parser -o parser.tab.cpp -d -t -v parser.y
	mv parser.tab.cpp parser.tab.xx
	sed -e "s/__attribute__ ((__unused__))//" parser.tab.xx > parser.tab.cpp
	rm parser.tab.xx

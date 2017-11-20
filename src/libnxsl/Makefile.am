lib_LTLIBRARIES = libnxsl.la
libnxsl_la_SOURCES = lex.parser.cpp parser.tab.cpp \
		     array.cpp class.cpp compiler.cpp env.cpp functions.cpp \
                     geolocation.cpp hashmap.cpp inetaddr.cpp instruction.cpp \
                     iterator.cpp lexer.cpp library.cpp main.cpp network.cpp \
                     program.cpp selectors.cpp stack.cpp storage.cpp \
                     table.cpp value.cpp variable.cpp vm.cpp
libnxsl_la_CPPFLAGS=-I@top_srcdir@/include
libnxsl_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsl_la_LIBADD = ../libnetxms/libnetxms.la
if USE_INTERNAL_LIBTRE
libnxsl_la_LIBADD += ../libtre/libnxtre.la
endif

EXTRA_DIST = Makefile.w32 libnxsl.vcxproj libnxsl.vcxproj.filters libnxsl.h parser.l parser.y parser.tab.hpp

lex.parser.cpp: parser.l parser.tab.cpp parser.tab.hpp
	$(LEX) -B -f -8 -olex.parser.cpp parser.l

parser.tab.hpp: parser.tab.cpp

parser.tab.cpp: parser.y
	$(YACC) -b parser -o parser.tab.cpp -d -t -v parser.y
	mv parser.tab.cpp parser.tab.xx
	sed -e "s/__attribute__ ((__unused__))//" parser.tab.xx > parser.tab.cpp
	rm parser.tab.xx

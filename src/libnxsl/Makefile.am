INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxsl.la
libnxsl_la_SOURCES = lex.yy.cpp parser.tab.cpp class.cpp compiler.cpp \
		     env.cpp functions.cpp instruction.cpp lexer.cpp \
		     library.cpp main.cpp program.cpp stack.cpp value.cpp \
		     variable.cpp
libnxsl_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsl_la_LIBADD = ../libnetxms/libnetxms.la

EXTRA_DIST = libnxsl.dsp libnxsl.dsw libnxsl.h parser.l parser.y parser.tab.hpp

lex.yy.cpp: parser.l parser.tab.cpp parser.tab.hpp
	$(LEX) -B -f -8 -olex.yy.temp parser.l
	echo "using namespace std;" > lex.yy.cpp
	sed -e "s/class istream;/#include <iostream>/" lex.yy.temp >> lex.yy.cpp
	rm lex.yy.temp

parser.tab.cpp: parser.y
	$(YACC) -b parser -o parser.tab.cpp -d -t -v parser.y
	mv parser.tab.cpp parser.tab.xx
	sed -e "s/__attribute__ ((__unused__))//" parser.tab.xx > parser.tab.cpp
	rm parser.tab.xx

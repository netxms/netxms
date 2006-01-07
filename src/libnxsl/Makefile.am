INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxsl.la
libnxsl_la_SOURCES = lex.yy.cpp parser.tab.cpp class.cpp compiler.cpp env.cpp \
                     functions.cpp instruction.cpp lexer.cpp library.cpp \
                     main.cpp program.cpp stack.cpp value.cpp variable.cpp 
libnxsl_la_LDFLAGS = -version-info $(LIBNXSL_LIBRARY_VERSION) $(PTHREAD_LIBS)
libnxsl_la_LIBADD = ../libnetxms/libnetxms.la

EXTRA_DIST = libnxsl.dsp libnxsl.dsw libnxsl.h parser.l parser.y parser.tab.h

lex.yy.cpp: parser.l parser.tab.cpp parser.tab.hpp
	$(LEX) -Bf8+ -olex.yy.cpp parser.l

parser.tab.cpp: parser.y
	$(YACC) -b parser -o parser.tab.cpp -d -t -v parser.y

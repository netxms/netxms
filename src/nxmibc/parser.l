%a 14000
%p 12000
%e 2000
%n 1000
%o 20000

%x COMMENT
%x KEYWORD
%x IMPORTS
%x MACRO
%x MACRO_BODY
%x EXPORTS
WHITESPC  [[:blank:]\t\n\r]

%{
/*
 * We now build a lexical analyzer to be used by a higher-level parser.
 */

/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *  
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *  
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define YY_NEVER_INTERACTIVE 1

#include "mibparse.h"
#include "parser.tab.h"/* token codes from the parser */
#include <nms_util.h>

int mperror(char *pszMsg);

LineNumber  mpLineNoG = 1;

static ubi_dlList _state;

typedef struct state_stack_struct {
    ubi_dlNode node;
	int state;
} StkVal;

static void _push_state(int x)
{
    StkVal *p = MT ( StkVal );
	p->state = YYSTATE;
	ubi_dlPush(&_state,p);
	BEGIN(x);
}

static void _pop_state()
{
    StkVal *p=(StkVal *)ubi_dlPop(&_state);
	if( p != NULL )
	{
	    BEGIN(p->state);
		free(p);
	}
	else
	{
	    BEGIN(INITIAL);
	}
}

static void _count_newlines( char *str, int len )
{
    /* 
	 * I hate, just hate, empty initializers in loops. Even if not needed
	 */
    for ( NULL ; len >= 0 ; len--)
    {
	    switch( str[len] )
		{
		    case '\n':
		    case '\r':
                mpLineNoG++;
				break;
			default:
			    break;
		}
    }
}

%}


%%

<INITIAL,COMMENT,KEYWORD,IMPORTS,EXPORTS,MACRO_BODY>{WHITESPC}+ {
     _count_newlines(mptext,mpleng);
     while( mpleng-- )
	 {
    	 if ( (*mptext == '\r') || (*mptext == '\n') )
    	 {
            switch( YYSTATE )
            {
                case INITIAL:
                    break;
                case COMMENT:
   					_pop_state();
                    break;
                case KEYWORD:
					/* Terminate the keyword state and revert to comment */
					_pop_state();
					/* Terminate the comment state and revert to previous */
					_pop_state();
                    break;
                case IMPORTS:
                case EXPORTS:
                    break;
                case MACRO:
                case MACRO_BODY:
                    break;
                default:
					_pop_state();
                    break;
            }
        }
		mptext++;
    }
}

<INITIAL,COMMENT,KEYWORD,IMPORTS,EXPORTS>"--" {
        switch( YYSTATE )
        {
            case INITIAL:
            case IMPORTS:
            case EXPORTS:
		        /* first one shifts us into comment mode */
     				_push_state(COMMENT);
                break;
            case COMMENT:
   				_pop_state();
                break;
            case KEYWORD:
                /* Ignore other states for now */
		       mperror("Cannot Embed Comments Within Keyword Definition");
            default:
                break;
        }
}

<COMMENT>KEYWORD { _push_state(KEYWORD); return KEYWORD_SYM; }
<KEYWORD>VALUE { return KEYWORD_VALUE_SYM; }
<KEYWORD>BINDING { return KEYWORD_BIND_SYM; }

<INITIAL,IMPORTS,EXPORTS,KEYWORD>"," return COMMA_SYM;

<INITIAL,IMPORTS,EXPORTS>";" { 
	if ((YYSTATE == IMPORTS) || (YYSTATE == EXPORTS))
	{
	    /* a semi colon terminates the import or export state */
		_pop_state();
	}
	return SEMI_COLON_SYM;
}

<IMPORTS>FROM return FROM_SYM;

<IMPORTS,EXPORTS,KEYWORD>[A-Z][a-zA-Z0-9]*[-a-zA-Z0-9]* {
          mplval.charPtr = (char*)malloc (mpleng+1);
		  sprintf(mplval.charPtr,"%*s",mpleng,mptext);
          mplval.charPtr[mpleng] = '\0';
          return UCASEFIRST_IDENT_SYM;}

<IMPORTS,EXPORTS,KEYWORD>[a-z][a-zA-Z0-9]*[-a-zA-Z0-9]* {
          mplval.charPtr = (char*)malloc (mpleng+1);
		  sprintf(mplval.charPtr,"%*s",mpleng,mptext);
          return LCASEFIRST_IDENT_SYM;}

<INITIAL,KEYWORD>"{"      return LEFT_BRACE_SYM;
<INITIAL,KEYWORD>"}"      return RIGHT_BRACE_SYM;
<INITIAL>"["      return LEFT_BRACKET_SYM;
<INITIAL>"]"      return RIGHT_BRACKET_SYM;
<INITIAL>"."      return DOT_SYM;
<INITIAL>"|"      return BAR_SYM;
<INITIAL>"("      return LEFT_PAREN_SYM;
<INITIAL>")"      return RIGHT_PAREN_SYM;
<INITIAL>DEFINITIONS return DEFINITIONS_SYM;
<INITIAL,KEYWORD,MACRO>"::=" return ASSIGNMENT_SYM;
<INITIAL>FROM return FROM_SYM;
<INITIAL>IMPORTS { BEGIN(IMPORTS); return IMPORTS_SYM; }
<INITIAL>EXPORTS { BEGIN(EXPORTS); return EXPORTS_SYM; }
<INITIAL>REVISION return REVISION_SYM;
<INITIAL>ORGANIZATION return ORGANIZATION_SYM;
<INITIAL>CONTACT-INFO return CONTACT_SYM;
<INITIAL>LAST-UPDATED return UPDATE_SYM;
<INITIAL>DESCRIPTION return DESCRIPTION_SYM;
<INITIAL>MODULE-IDENTITY return MODULE_IDENTITY_SYM;
<INITIAL>MODULE-COMPLIANCE return MODULE_COMPLIANCE_SYM;
<INITIAL>OBJECTS return OBJECTS_SYM;
<INITIAL>GROUP return GROUP_SYM;
<INITIAL>AGENT[ |\t]*CAPABILITIES return AGENT_CAPABILITIES_SYM;
<INITIAL>OBJECT-TYPE return OBJECT_TYPE_SYM;
<INITIAL>OBJECT-GROUP return OBJECT_GROUP_SYM;
<INITIAL>OBJECT-IDENTITY return OBJECT_IDENTITY_SYM;
<INITIAL>MANDATORY-GROUPS return MANDATORY_GROUPS_SYM;
<INITIAL>NOTIFICATION-GROUP return NOTIFICATION_GROUP_SYM;
<INITIAL>NOTIFICATION-TYPE return NOTIFICATION_TYPE_SYM;
<INITIAL>INTEGER return INTEGER_SYM;
<INITIAL>Integer32 return INTEGER32_SYM;
<INITIAL>Unsigned32 return UNSIGNED32_SYM;
<INITIAL>Counter return COUNTER_SYM;
<INITIAL>Counter32 return COUNTER32_SYM;
<INITIAL>Counter64 return COUNTER64_SYM;
<INITIAL>Gauge32 return GAUGE32_SYM;
<INITIAL>TimeTicks return TIMETICKS_SYM;
<INITIAL>IpAddress return IP_ADDRESS_SYM;
<INITIAL>BITS      return BITS_SYM;
<INITIAL>STRING   return STRING_SYM;
<INITIAL>OCTET    return OCTET_SYM;
<INITIAL>Opaque    return OPAQUE_SYM;
<INITIAL>SEQUENCE return SEQUENCE_SYM;
<INITIAL>OF       return OF_SYM;
<INITIAL>ACCESS    return  ACCESS_SYM;
<INITIAL>SYNTAX    return  SYNTAX_SYM;
<INITIAL>WRITE-SYNTAX    return  WRITE_SYNTAX_SYM;
<INITIAL>STATUS    return  STATUS_SYM;
<INITIAL>REFERENCE    return  REFERENCE_SYM;
<INITIAL>INDEX    return  INDEX_SYM;
<INITIAL>DEFVAL    return  DEFVAL_SYM;
<INITIAL>TEXTUAL-CONVENTION    return  TEXTUAL_CONVENTION_SYM;
<INITIAL>DISPLAY-HINT    return  DISPLAY_HINT_SYM;
<INITIAL>MAX-ACCESS    return  MAX_ACCESS_SYM;
<INITIAL>MIN-ACCESS    return  MIN_ACCESS_SYM;
<INITIAL>NOTIFICATIONS return NOTIFICATIONS_SYM;
<INITIAL>SIZE return SIZE_SYM;
<INITIAL>SUPPORTS return SUPPORTS_SYM;
<INITIAL>INCLUDES return INCLUDES_SYM;
<INITIAL>CHOICE return CHOICE_SYM;
<INITIAL>VARIATION return VARIATION_SYM;
<INITIAL>EXPLICIT return EXPLICIT_SYM;
<INITIAL>IMPLICIT return IMPLICIT_SYM;
<INITIAL>IMPLIED return IMPLIED_SYM;
<INITIAL>TAGS return TAGS_SYM;
<INITIAL>AUTOMATIC return AUTOMATIC_SYM;
<INITIAL>UNITS return UNITS_SYM;
<INITIAL>AUGMENTS return AUGMENTS_SYM;
<INITIAL>ENTERPRISE return ENTERPRISE_SYM;
<INITIAL>VARIABLES return VARIABLES_SYM;
<INITIAL>TRAP-TYPE return TRAP_TYPE_SYM;
<INITIAL>PRODUCT-RELEASE return PRODUCT_RELEASE_SYM;
<INITIAL>CREATION-REQUIRES return CREATION_REQUIRES_SYM;
<INITIAL>OBJECT[ ]*IDENTIFIER { return OBJECT_IDENTIFIER_SYM;}
<INITIAL>OBJECT { return OBJECT_SYM;}

<INITIAL>[A-Z][a-zA-Z0-9]*[-a-zA-Z0-9]*{WHITESPC}*MACRO {
/*
 * This code initiates the macro state. There are two states associated
 * with a macro definition. the MACRO state indicates that we have found
 * a MACRO keyword preceeded by an identifier. In this state the assignment
 * operator is processed normally (returning ASSIGNMENT_SYM) to allow the
 * parser to detect the macro header (BEGIN_SYM).
 * The BEGIN_SYM switches us into the MACRO_BODY state where all characters
 * are processed as tokens (TOKEN_SYM returns a char *).
 *
 * The macro footer (END_SYM) switches the state back to INITIAL. This may
 * be incorrect and it may be necessary to preserve and switch back to the
 * preceeding state.
 */
          int length=0; /* length of the output string */
          int count=0;  /* loop control variable */
		  int multiSpace=0; /* multiple space compression */
          mplval.charPtr = (char *)malloc(mpleng); /* this is over allocated */
		  mptext += 5; /* skip MACRO */
		  mpleng -= 5; /* skip MACRO */
          _count_newlines(mptext,mpleng);
		  while ( count < mpleng )
		  {
			  switch( *mptext)
			  {
				  case '\n':
				  case '\r':
				  /*
				   * I'm not sure if the return/newlines are strictly
				   * legal
				   */
				  case '\t':
				  case ' ':
				  /*
				   * In any event compress them, tabs and spaces 
				   */
				      if( multiSpace == 0 )
					  {
					      mplval.charPtr[length++] = *mptext;
						  multiSpace = 1;
					  }
					  break;
				  default:
                      mplval.charPtr[length++] = *mptext;
                      multiSpace = 0; /* reset multi space flag */
					  break;
			  }
			  count++;
			  mptext++;
		  }
		  _push_state(MACRO);
          mplval.charPtr[length] = '\0';
          return MACRO_SYM; }

<INITIAL,MACRO>BEGIN {
        switch( YYSTATE )
          {
              case MACRO:
			      _push_state(MACRO_BODY);
                  break;
              case INITIAL:
              case KEYWORD:
              case COMMENT:
              default:
                  break;
          }
          return BEGIN_SYM;}

<INITIAL,MACRO_BODY>END {
/*
 * If an END symbol is encountered while in the MACRO_BODY state
 * the state will be reverted to initial. To revert to the previous state
 * it must be preserved in the MACRO detection and reassigned here 
 */
        switch( YYSTATE )
          {
              case COMMENT:
                  break;
              case INITIAL:
              case KEYWORD:
                  break;
              case MACRO_BODY:
			      _pop_state(); /* should revert to MACRO and fall through */
              case MACRO:
			      _pop_state(); /* should clear the MACRO state */
                  break;
              default:
                  break;
          }
          return END_SYM;}

<MACRO_BODY>([[:alnum:][:punct:]]*) {
          mplval.charPtr = (char*)malloc (mpleng+1);
          strcpy (mplval.charPtr, mptext);
          mplval.charPtr[mpleng] = '\0';
          return TOKEN_SYM;}

<INITIAL>MODULE {
          mplval.charPtr = NULL;
          return MODULE_SYM;}

<INITIAL>MODULE[[:blank:]]+[A-Z][a-zA-Z0-9]*[-a-zA-Z0-9]* {
		  mptext += 6; /* skip past "MODULE" */
		  mpleng -= 6; /* skip past "MODULE" */
          _count_newlines(mptext,mpleng);
		  while( isspace(*mptext) )
		  {
		      mptext++;
			  mpleng--;
		  }
          mplval.charPtr = (char*)malloc(mpleng+1);
          strncpy (mplval.charPtr, mptext, mpleng);
          mplval.charPtr[mpleng] = '\0';
          return MODULE_SYM;}

<INITIAL,KEYWORD>\'[0-1]*\'B {
          mplval.charPtr = (char*)malloc (mpleng);
          strncpy (mplval.charPtr, mptext+1, mpleng -1);  /* strip "'"s */
          mplval.charPtr[mpleng-2] = '\0';
          return BSTRING_SYM;}

<INITIAL,KEYWORD>\'[0-9A-Fa-f]*\'H {
          mplval.charPtr = (char*)malloc (mpleng);
          strncpy (mplval.charPtr, mptext+1, mpleng -1);   /* strip "'"s */
          mplval.charPtr[mpleng-2] = '\0';
          return HSTRING_SYM;}

<INITIAL,KEYWORD>\"([^\"]|"\"\"")*\"  {
          _count_newlines( mptext, mpleng );
          mplval.charPtr = (char*)malloc (mpleng);
          strncpy (mplval.charPtr, mptext+1, mpleng -1);       /* strip '"'s */
          mplval.charPtr[mpleng-2] = '\0'; /* 2 quotes == quote in a quote */
          return CSTRING_SYM;}

<INITIAL>[A-Z][a-zA-Z0-9]*[-a-zA-Z0-9]* {
          mplval.charPtr = (char*)malloc (mpleng+1);
          strcpy (mplval.charPtr, mptext);
          mplval.charPtr[mpleng] = '\0';
          return UCASEFIRST_IDENT_SYM;}

<INITIAL>[a-z](-[a-zA-Z0-9]|[a-zA-Z0-9])* {
          mplval.charPtr = (char*)malloc (mpleng+1);
          strcpy (mplval.charPtr, mptext);
          mplval.charPtr[mpleng] = '\0';
          return LCASEFIRST_IDENT_SYM;}

<INITIAL>([-]|0x)*[1-9][0-9]* {
		  QWORD qwVal = strtoull(mptext, NULL, 0);
          long int intVal = strtol(mptext, NULL, 0);
/*		  if ((intVal == LONG_MAX) && (qwVal == 0xFFFFFFFFFFFFFFFF))
		  {
		  char tPtr[200];
		    sprintf(tPtr,"Numeric Overflow - Value %.*s", mpleng,mptext);
		    mperror(tPtr);
		  }*/
		  if( intVal == LONG_MAX )
		  {
		        mplval.numberVal.type = SNMP_INTEGER64;
		        mplval.numberVal.data.qwVal = qwVal;
		  }
		  else
		  {
		        mplval.numberVal.type = SNMP_INTEGER32;
		        mplval.numberVal.data.intVal = intVal;
		  }
  		  return NUMBER_SYM;}

<INITIAL>0 {  /*allow zero as first digit on single digit #'s*/
         mplval.numberVal.type = SNMP_INTEGER32;
         mplval.numberVal.data.intVal = 0;
         return NUMBER_SYM;}


<INITIAL,COMMENT,KEYWORD,EXPORTS>. { 
    _count_newlines( mptext, mpleng );
}
%%

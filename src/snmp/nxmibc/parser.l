%option never-interactive
%option 8bit

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
%x END
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
#define YY_NO_UNISTD_H

#include "mibparse.h"
#include "parser.tab.hpp"/* token codes from the parser */
#include <nms_util.h>

#ifdef _WIN32 
#pragma warning(disable: 4018)
#if defined(__64BIT__)
#pragma warning(disable: 4244 4267)
#endif
#define read	_read
#endif

int mperror(const char *pszMsg);

int g_nCurrLine;


//
// State stack
//

static int m_nStateStack[32];
static int m_nCurrStackPos;

void InitStateStack(void)
{
   m_nCurrStackPos = 0;
}

static void PushState(int nState)
{
   m_nStateStack[m_nCurrStackPos++] = YYSTATE;
   BEGIN(nState);
}

static void PopState(void);

/**
 * Count newlines in a text block
 */
static void CountNewlines(char *pszStr, int len)
{
   int i;

   for(i = 0; i < len; i++)
      if (pszStr[i] == '\n')
         g_nCurrLine++;
}

%}


%%

<INITIAL,COMMENT,KEYWORD,IMPORTS,EXPORTS,MACRO,MACRO_BODY>{WHITESPC}+ {
   int i;
   char *ptr;

   CountNewlines(mptext, mpleng);
   for(i = 0, ptr = mptext; i < mpleng; i++, ptr++)
	{
      if (*ptr == '\n')
    	{
         switch( YYSTATE )
         {
            case INITIAL:
            case IMPORTS:
            case EXPORTS:
            case MACRO:
            case MACRO_BODY:
               break;
            case COMMENT:
               PopState();
               break;
            case KEYWORD:
               /* Terminate the keyword state and revert to comment */
               PopState();
               /* Terminate the comment state and revert to previous */
               PopState();
               break;
            default:
               PopState();
               break;
         }
      }
   }
}

<INITIAL,COMMENT,KEYWORD,IMPORTS,EXPORTS>"--" {
   switch(YYSTATE)
   {
      case INITIAL:
      case IMPORTS:
      case EXPORTS:
         /* first one shifts us into comment mode */
         PushState(COMMENT);
         break;
      case KEYWORD:
         /* Ignore other states for now */
         mperror("Cannot embed comments within keyword definition");
         break;
      default:
         break;
   }
}

<COMMENT>KEYWORD {
   PushState(KEYWORD);
   return KEYWORD_SYM; 
}

<KEYWORD>VALUE    return KEYWORD_VALUE_SYM;
<KEYWORD>BINDING  return KEYWORD_BIND_SYM;

<INITIAL,IMPORTS,EXPORTS,KEYWORD>"," return COMMA_SYM;

<INITIAL,IMPORTS,EXPORTS>";" { 
	if ((YYSTATE == IMPORTS) || (YYSTATE == EXPORTS))
	{
	    /* a semi colon terminates the import or export state */
		PopState();
	}
	return SEMI_COLON_SYM;
}

<IMPORTS>FROM return FROM_SYM;

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
<INITIAL>IMPORTS {
   BEGIN(IMPORTS);
   return IMPORTS_SYM; 
}
<INITIAL>EXPORTS { 
   BEGIN(EXPORTS);
   return EXPORTS_SYM;
}
<INITIAL>REVISION return REVISION_SYM;
<INITIAL>ORGANIZATION return ORGANIZATION_SYM;
<INITIAL>CONTACT-INFO return CONTACT_SYM;
<INITIAL>LAST-UPDATED return UPDATE_SYM;
<INITIAL>DESCRIPTION return DESCRIPTION_SYM;
<INITIAL>MODULE-IDENTITY return MODULE_IDENTITY_SYM;
<INITIAL>MODULE-COMPLIANCE return MODULE_COMPLIANCE_SYM;
<INITIAL>OBJECTS return OBJECTS_SYM;
<INITIAL>GROUP return GROUP_SYM;
<INITIAL>AGENT[ \t\-]*CAPABILITIES return AGENT_CAPABILITIES_SYM;
<INITIAL>OBJECT-TYPE return OBJECT_TYPE_SYM;
<INITIAL>OBJECT-GROUP return OBJECT_GROUP_SYM;
<INITIAL>OBJECT-IDENTITY return OBJECT_IDENTITY_SYM;
<INITIAL>MANDATORY-GROUPS return MANDATORY_GROUPS_SYM;
<INITIAL>NOTIFICATION-GROUP return NOTIFICATION_GROUP_SYM;
<INITIAL>NOTIFICATION-TYPE return NOTIFICATION_TYPE_SYM;
<INITIAL>INTEGER return INTEGER_SYM;
<INITIAL>Integer32 return INTEGER32_SYM;
<INITIAL>UInteger32 return UNSIGNED32_SYM;
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
<INITIAL>OBJECT[ \t]*IDENTIFIER return OBJECT_IDENTIFIER_SYM;
<INITIAL>OBJECT return OBJECT_SYM;
<INITIAL>MIN return MIN_SYM;
<INITIAL>MAX return MAX_SYM;
<INITIAL>MODULE return MODULE_SYM;

<INITIAL>[A-Z][a-zA-Z0-9]*[\-a-zA-Z0-9]*{WHITESPC}*MACRO {
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
   int length, tl, count, multiSpace;

   tl = mpleng - 5;   // Strip trailing MACRO
   mplval.pszString = (char *)malloc(mpleng);
   CountNewlines(mptext, mpleng);
   for(length = 0, count = 0, multiSpace = 0; count < tl; count++)
   {
      switch(mptext[count])
      {
         case '\n':
         case '\r':
         case '\t':
         case ' ':
            if (multiSpace == 0)
            {
               mplval.pszString[length++] = ' ';
               multiSpace = 1;
            }
            break;
         default:
            mplval.pszString[length++] = mptext[count];
            multiSpace = 0;
            break;
      }
   }
   PushState(MACRO);
   mplval.pszString[length] = 0;
   StrStripA(mplval.pszString);
   return MACRO_SYM;
}

<INITIAL,MACRO>BEGIN {
   if (YYSTATE == MACRO)
      PushState(MACRO_BODY);
   return BEGIN_SYM;
}

<INITIAL,MACRO_BODY>END {
/*
 * If an END symbol is encountered while in the MACRO_BODY state
 * the state will be reverted to initial. To revert to the previous state
 * it must be preserved in the MACRO detection and reassigned here 
 */
   switch( YYSTATE )
   {
      case INITIAL:
         BEGIN(END); /* ignore everything after END symbol */
         break;
      case MACRO_BODY:
         PopState(); /* should revert to MACRO and fall through */
      case MACRO:
         PopState(); /* should clear the MACRO state */
         break;
      default:
         break;
   }
   return END_SYM;
}

<MACRO_BODY>([[:alnum:][:punct:]]*) {
   return TOKEN_SYM;
}

<INITIAL,KEYWORD>\'[0-1]*\'[Bb] {
   mplval.pszString = (char *)malloc(mpleng);
   strncpy(mplval.pszString, mptext + 1, mpleng -1);  // strip "'"s
   mplval.pszString[mpleng - 3] = '\0';
   return BSTRING_SYM;
}

<INITIAL,KEYWORD>\'[0-9A-Fa-f]*\'[Hh] {
   mplval.pszString = (char*)malloc(mpleng);
   strncpy(mplval.pszString, mptext + 1, mpleng - 1);   // strip "'"s
   mplval.pszString[mpleng - 3] = '\0';
   if (strlen(mplval.pszString) <= 8)
   {
     int n = strtoul(mplval.pszString, NULL, 16);
     free(mplval.pszString);
     mplval.number.value.nInt32 = n;
     mplval.number.nType = 0;
     return NUMBER_SYM;
   }
   return HSTRING_SYM;
}

<INITIAL,KEYWORD>\"([^\"]|"\"\"")*\"  {
   CountNewlines(mptext, mpleng);
   mplval.pszString = (char*)malloc(mpleng);
   strncpy(mplval.pszString, mptext + 1, mpleng - 1);  // strip '"'s
   mplval.pszString[mpleng - 2] = 0;
   return CSTRING_SYM;
}

<INITIAL,IMPORTS,EXPORTS,KEYWORD>[A-Z][a-zA-Z0-9_\-\/]* {
   mplval.pszString = strdup(mptext);
   return UCASEFIRST_IDENT_SYM;
}

<INITIAL,IMPORTS,EXPORTS,KEYWORD>[a-z][a-zA-Z0-9_\-\/]* {
   mplval.pszString = strdup(mptext);
   return LCASEFIRST_IDENT_SYM;
}

<INITIAL>([-]|0x)?[0-9]+ {
   INT64 nInt64 = strtoull(mptext, NULL, 0);
   LONG nInt32 = strtol(mptext, NULL, 0);

   if ((INT64)nInt32 != nInt64)
   {
      mplval.number.nType = 1;
      mplval.number.value.nInt64 = nInt64;
   }
   else
   {
      mplval.number.nType = 0;
      mplval.number.value.nInt32 = nInt32;
   }
   return NUMBER_SYM;
}

<INITIAL,COMMENT,KEYWORD,EXPORTS>. { 
   CountNewlines(mptext, mpleng);
}

<END>.	;
<END>\n	;

%%


static void PopState(void)
{
   if (m_nCurrStackPos > 0)
   {
      BEGIN(m_nStateStack[--m_nCurrStackPos]);
   }
	else
	{
      BEGIN(INITIAL);
	}
}

void ResetScanner()
{
      mprestart(mpin);
      BEGIN(INITIAL);
}

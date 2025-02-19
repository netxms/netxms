%{
/*
 * Copyright 2001, THE AUTHOR <mibparser@cvtt.net>
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
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

/*
 * A parser for the basic grammar to use for snmp V2c modules
 */

#ifdef _WIN32
#pragma warning(disable : 4065 4102)
#endif

/*
#ifndef __STDC__
#define __STDC__	1
#endif
*/

#define YYMALLOC	MemAlloc
#define YYFREE		MemFree

#include <nms_common.h>
#include <nms_util.h>
#include <string.h>
#include <time.h>
#include "mibparse.h"
#include "nxmibc.h"
#include "nxsnmp.h"

#define YYINCLUDED_STDLIB_H		1

#ifdef YYTEXT_POINTER
extern char *mptext;
#else
extern char mptext[];
#endif

#ifdef __64BIT__
#define YYSIZE_T  int64_t
#endif

extern FILE *mpin, *mpout;
extern int g_nCurrLine;

static MP_MODULE *m_pModule;
static MP_SYNTAX *s_currentSyntaxObject = nullptr;
#ifdef UNICODE
static char s_currentFilename[MAX_PATH];
#else
static const char *s_currentFilename;
#endif

int mperror(const char *pszMsg);
int mplex(void);
void ResetScanner();

MP_SYNTAX *create_std_syntax(int nSyntax)
{
   MP_SYNTAX *p = new MP_SYNTAX;
   p->nSyntax = nSyntax;
   return p;
}

static int AccessFromText(const char *pszText)
{
   static const char *pText[] = { "read-only", "read-write", "write-only",
                                  "not-accessible", "accessible-for-notify",
                                  "read-create", nullptr };
   for(int i = 0; pText[i] != nullptr; i++)
      if (strcmp(pszText, pText[i]))
         return i + 1;

   char errorMessage[256];
   snprintf(errorMessage, 256, "Invalid ACCESS value \"%s\"", pszText);
   mperror(errorMessage);
   return -1;
}

%}

/*
 *	Union structure.  A terminal or non-terminal can have
 *	one of these type values.
 */

%union
{
   int nInteger;
   char *pszString;
   MP_NUMERIC_VALUE number;
   Array *stringList;
   ObjectArray<MP_SUBID> *oid;
   ObjectArray<MP_IMPORT_MODULE> *pImportList;
   MP_IMPORT_MODULE *pImports;
   MP_OBJECT *pObject;
   MP_SUBID *pSubId;
   MP_SYNTAX *pSyntax;
}

%token ENTERPRISE_SYM
%token TRAP_TYPE_SYM
%token VARIABLES_SYM
%token EXPLICIT_SYM
%token IMPLICIT_SYM
%token IMPLIED_SYM
%token RIGHT_BRACE_SYM
%token LEFT_BRACE_SYM
%token RIGHT_BRACKET_SYM
%token LEFT_BRACKET_SYM
%token DEFINITIONS_SYM
%token ASSIGNMENT_SYM
%token BEGIN_SYM
%token END_SYM
%token FROM_SYM
%token IMPORTS_SYM
%token EXPORTS_SYM
%token COMMA_SYM
%token SEMI_COLON_SYM
%token DOT_SYM
%token DESCRIPTION_SYM
%token ORGANIZATION_SYM
%token CONTACT_SYM
%token UPDATE_SYM
%token MODULE_IDENTITY_SYM
%token MODULE_COMPLIANCE_SYM
%token OBJECT_IDENTIFIER_SYM
%token OBJECT_TYPE_SYM
%token OBJECT_GROUP_SYM
%token OBJECT_IDENTITY_SYM
%token OBJECTS_SYM
%token MANDATORY_GROUPS_SYM
%token GROUP_SYM
%token AGENT_CAPABILITIES_SYM
%token KEYWORD_SYM
%token KEYWORD_VALUE_SYM
%token KEYWORD_BIND_SYM
%token TOKEN_SYM

%token INTEGER_SYM
%token INTEGER32_SYM
%token UNSIGNED32_SYM
%token GAUGE32_SYM
%token COUNTER_SYM
%token COUNTER32_SYM
%token COUNTER64_SYM
%token BITS_SYM
%token STRING_SYM
%token OCTET_SYM
%token SEQUENCE_SYM
%token OF_SYM
%token TIMETICKS_SYM
%token IP_ADDRESS_SYM
%token NETWORK_ADDRESS_SYM
%token OPAQUE_SYM
%token REVISION_SYM
%token TEXTUAL_CONVENTION_SYM

%token ACCESS_SYM
%token MAX_ACCESS_SYM
%token MIN_ACCESS_SYM
%token SYNTAX_SYM
%token STATUS_SYM
%token INDEX_SYM
%token REFERENCE_SYM
%token DEFVAL_SYM
%token DISPLAY_HINT_SYM
%token LEFT_PAREN_SYM
%token RIGHT_PAREN_SYM
%token NOTIFICATIONS_SYM
%token NOTIFICATION_GROUP_SYM
%token NOTIFICATION_TYPE_SYM
%token SIZE_SYM
%token BAR_SYM
%token VARIATION_SYM
%token WRITE_SYNTAX_SYM
%token SUPPORTS_SYM
%token INCLUDES_SYM
%token CREATION_REQUIRES_SYM
%token PRODUCT_RELEASE_SYM
%token CHOICE_SYM
%token UNITS_SYM
%token AUGMENTS_SYM
%token OBJECT_SYM
%token TAGS_SYM
%token AUTOMATIC_SYM
%token MIN_SYM
%token MAX_SYM
%token MODULE_SYM

%token <pszString> MACRO_SYM
%token <pszString> UCASEFIRST_IDENT_SYM LCASEFIRST_IDENT_SYM
%token <pszString> BSTRING_SYM HSTRING_SYM CSTRING_SYM

%token <number> NUMBER_SYM

%type <nInteger> SnmpStatusPart SnmpAccessPart

%type <pSyntax> SnmpSyntaxPart SnmpSyntaxStatement Type BuiltInType NamedType
%type <pSyntax> TextualConventionAssignment BuiltInTypeAssignment

%type <pszString> UCidentifier LCidentifier Identifier
%type <pszString> ModuleIdentifier DefinedValue SnmpIdentityPart
%type <pszString> Symbol CharString
%type <pszString> SnmpDescriptionPart SnmpDescriptionStatement

%type <number> Number

%type <stringList> SymbolList SnmpIndexPart
%type <pImportList> SymbolsFromModuleList
%type <oid> AssignedIdentifierList AssignedIdentifier ObjectIdentifierList
%type <pImports> SymbolsFromModule
%type <pObject> ObjectIdentifierAssignment ObjectIdentityAssignment ObjectTypeAssignment
%type <pObject> MacroAssignment TypeOrValueAssignment ModuleIdentityAssignment
%type <pObject> SnmpObjectGroupAssignment SnmpNotificationGroupAssignment
%type <pObject> SnmpNotificationTypeAssignment AgentCapabilitiesAssignment
%type <pSubId> ObjectIdentifier

%destructor { MemFree($$); } <pszString>
%destructor { delete $$; } <oid>
%destructor { delete $$; } <pImportList>
%destructor { delete $$; } <pImports>
%destructor { delete $$; } <pObject>
%destructor { delete $$; } <pSubId>
%destructor { delete $$; } <pSyntax>

/*
 *  Type definitions of non-terminal symbols
*/

%start ModuleDefinition
%%

/*-----------------------------------------------------------------------*/
/* Module def/import/export productions */
/*-----------------------------------------------------------------------*/

ModuleDefinition:
	ModuleIdentifierAssignment AssignmentList End
|	ModuleIdentifierAssignment End
;

AssignmentList:
    AssignmentList Assignment
|   Assignment
;

Assignment:
    ObjectIdentifierAssignment
{
   m_pModule->pObjectList->add($1);
}
|   ObjectIdentityAssignment
{
   m_pModule->pObjectList->add($1);
}
|   ObjectTypeAssignment
{
   m_pModule->pObjectList->add($1);
}
|   MacroAssignment
{
   m_pModule->pObjectList->add($1);
}
|   TypeOrValueAssignment
{
   m_pModule->pObjectList->add($1);
}
|   SnmpNotificationTypeAssignment
{
   m_pModule->pObjectList->add($1);
}
|   ModuleComplianceAssignment
|   SnmpNotificationGroupAssignment
{
   m_pModule->pObjectList->add($1);
}
|   SnmpObjectGroupAssignment
{
   m_pModule->pObjectList->add($1);
}
|   SnmpKeywordAssignment
|   AgentCapabilitiesAssignment
{
   m_pModule->pObjectList->add($1);
}
|   ModuleIdentityAssignment
{
   m_pModule->pObjectList->add($1);
}
|   ImportsAssignment
|   ExportsAssignment
;

ModuleIdentifierAssignment:
    ModuleIdentifier
{
   m_pModule = new MP_MODULE;
   m_pModule->pszName = $1;
}
;

ModuleIdentifier:
    UCidentifier DEFINITIONS_SYM ASSIGNMENT_SYM Begin
{
   $$ = $1;
}
|   UCidentifier DEFINITIONS_SYM ASSIGNMENT_SYM AUTOMATIC_SYM TAGS_SYM Begin
{
   $$ = $1;
}
;

ObjectIdentifierAssignment:
    Identifier OBJECT_IDENTIFIER_SYM AssignedIdentifier
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->pszName = $1;
   $$->oid = $3;
   if (isupper($$->pszName[0]))
   {
      ReportError(ERR_UPPERCASE_IDENTIFIER, s_currentFilename, g_nCurrLine);
   }
}
|   LCidentifier AssignedIdentifier
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->pszName = $1;
   $$->oid = $2;
}
;

AssignedIdentifier:
    ASSIGNMENT_SYM AssignedIdentifierList
{
   $$ = $2;
}
|   ASSIGNMENT_SYM Number
{
   MP_SUBID *subid;

   subid = new MP_SUBID;
   subid->dwValue = $2.value.nInt32;
   subid->pszName = NULL;
   subid->bResolved = TRUE;
   $$ = new ObjectArray<MP_SUBID>(16, 16, Ownership::True);
   $$->add(subid);
}
;

AssignedIdentifierList:
    LEFT_BRACE_SYM ObjectIdentifierList RIGHT_BRACE_SYM
{
   $$ = $2;
}
;

ObjectIdentifierList:
    ObjectIdentifierList ObjectIdentifier
{
	$$ = $1;
   $$->add($2);
   $2 = nullptr;
}
|   ObjectIdentifier
{
   $$ = new ObjectArray<MP_SUBID>(16, 16, Ownership::True);
   $$->add($1);
   $1 = nullptr;
}
;

ObjectIdentifier:
    Number
{
   $$ = new MP_SUBID;
   $$->dwValue = $1.value.nInt32;
   $$->pszName = nullptr;
   $$->bResolved = TRUE;
}
|   DefinedValue
{
   $$ = new MP_SUBID;
   $$->dwValue = 0;
   $$->pszName = $1;
   $$->bResolved = FALSE;
   $1 = nullptr;
}
|   Identifier LEFT_PAREN_SYM Number RIGHT_PAREN_SYM
{
   $$ = new MP_SUBID;
   $$->dwValue = $3.value.nInt32;
   $$->pszName = $1;
   $$->bResolved = TRUE;
   $1 = nullptr;
}
;

NumericValue:
    NumberOrMinMax
|   DefinedValue
{
   MemFreeAndNull($1);
}
|   NumberOrMinMax DOT_SYM DOT_SYM NumberOrMinMax 
|   Identifier LEFT_PAREN_SYM NumberOrMinMax RIGHT_PAREN_SYM
{
   MemFreeAndNull($1);
}
;

NumberOrMinMax:
	Number
|	MIN_SYM
|	MAX_SYM
;

DefinedValue:
    UCidentifier DOT_SYM LCidentifier
{
   $$ = MemAllocStringA(strlen($1) + strlen($3) + 2);
   sprintf($$, "%s.%s", $1, $3);
   MemFreeAndNull($1);
   MemFreeAndNull($3);
}
|   LCidentifier
{
   $$ = $1;
}
;

Number:
    NUMBER_SYM
{
   $$ = $1;
}
;

ObjectIdentityAssignment:
    Identifier OBJECT_IDENTITY_SYM
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    AssignedIdentifier
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->pszName = $1;
   $$->iStatus = $3;
   $$->pszDescription = $4;
   $$->oid = $6;
   if (isupper($$->pszName[0]))
   {
      ReportError(ERR_UPPERCASE_IDENTIFIER, s_currentFilename, g_nCurrLine);
   }
}
;

ObjectTypeAssignment:
    Identifier OBJECT_TYPE_SYM
    SnmpSyntaxPart
    SnmpUnitsPart
    SnmpAccessPart
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    SnmpIndexPart
    SnmpDefValPart
    AssignedIdentifier
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->pszName = $1;
   $$->iSyntax = $3->nSyntax;
   $$->pszDataType = $3->pszStr;
   $3->pszStr = nullptr;
   delete $3;
   $$->iAccess = $5;
   $$->iStatus = $6;
   $$->pszDescription = $7;
   $$->index = $9;
   $$->oid = $11;
   if (isupper($$->pszName[0]))
   {
      ReportError(ERR_UPPERCASE_IDENTIFIER, s_currentFilename, g_nCurrLine);
   }
}
;

ModuleIdentityAssignment:
    SnmpIdentityPart
    SnmpUpdatePart
    SnmpOrganisationPart
    SnmpContactInfoPart
    SnmpDescriptionPart
    SnmpRevisionPart
    AssignedIdentifier
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->iSyntax = MIB_TYPE_MODID;
   $$->pszName = $1;
   $$->pszDescription = $5;
   $$->oid = $7;
}
;

ImportsAssignment:
    IMPORTS_SYM SymbolsFromModuleList SEMI_COLON_SYM
{
	delete m_pModule->pImportList;
   m_pModule->pImportList = $2;
}
;

ExportsAssignment:
    EXPORTS_SYM SymbolList SEMI_COLON_SYM
{
   delete $2;
}
;

SnmpRevisionPart:
    SnmpRevisionList
|
;

SnmpRevisionList:
    SnmpRevisionList SnmpRevisionObject
|   SnmpRevisionObject
;

SnmpRevisionObject:
    REVISION_SYM CharString
    SnmpDescriptionPart
{
   MemFreeAndNull($2);
   MemFreeAndNull($3);
}
;

SnmpIdentityPart:
    LCidentifier MODULE_IDENTITY_SYM
{
   $$ = $1;
}
;

SnmpOrganisationPart:
    ORGANIZATION_SYM CharString
{
   MemFreeAndNull($2);
}
;

SnmpContactInfoPart:
    CONTACT_SYM CharString
{
   MemFreeAndNull($2);
}
;

SnmpUpdatePart:
    UPDATE_SYM CharString
{
   MemFreeAndNull($2);
}
;

SnmpDescriptionPart:
    SnmpDescriptionStatement
{
   $$ = $1;
}
|
{
   $$ = nullptr;
}
;

SnmpDescriptionStatement:
    DESCRIPTION_SYM CharString
{
   $$ = $2;
}
;

ValueConstraint:
    LEFT_PAREN_SYM NumericValueConstraintList DanglingComma RIGHT_PAREN_SYM
|   LEFT_PAREN_SYM SIZE_SYM LEFT_PAREN_SYM NumericValueConstraintList DanglingComma RIGHT_PAREN_SYM RIGHT_PAREN_SYM
|   LEFT_BRACE_SYM NumericValueConstraintList DanglingComma RIGHT_BRACE_SYM
;

NumericValueConstraintList:
    NumericValueConstraintList BAR_SYM NumericValue
|   NumericValueConstraintList COMMA_SYM NumericValue
|   NumericValue
;

DanglingComma:
   COMMA_SYM
{
   ReportError(ERR_DANGLING_COMMA, s_currentFilename, g_nCurrLine);
}
|
;

SnmpKeywordAssignment:
    SnmpKeywordName
|   SnmpKeywordName SnmpKeywordValue
|   SnmpKeywordName SnmpKeywordBinding SnmpKeywordValue
;

SnmpKeywordName:
    KEYWORD_SYM ASSIGNMENT_SYM CharString
{
   MemFreeAndNull($3);
}
;

SnmpKeywordValue:
    KEYWORD_VALUE_SYM ASSIGNMENT_SYM CharString
{
   MemFreeAndNull($3);
}
;

SnmpKeywordBinding:
    KEYWORD_BIND_SYM ASSIGNMENT_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
   delete_and_null($4);
}
;

SnmpSyntaxPart:
    SnmpSyntaxStatement
{
   $$ = $1;
}
|
{
   $$ = new MP_SYNTAX;
   $$->nSyntax = MIB_TYPE_OTHER;
}
;

SnmpSyntaxStatement:
    SYNTAX_SYM Type
{
   $$ = $2;
}
;

SnmpUnitsPart:
    UNITS_SYM CharString
{
   MemFreeAndNull($2);
}
|
;

SnmpWriteSyntaxPart:
    WRITE_SYNTAX_SYM Type
{
	delete_and_null($2);
}
|
;

SnmpCreationPart:
    CREATION_REQUIRES_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
   delete_and_null($3);
}
|
;

TypeOrValueAssignment:
    BuiltInTypeAssignment
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_TYPEDEF;
   $$->oid = new ObjectArray<MP_SUBID>(0, 16, Ownership::True);
   if ($1 != nullptr)
   {
      $$->pszName = $1->pszStr;
      $$->iSyntax = $1->nSyntax;
      $$->pszDescription = $1->pszDescription;
      $1->pszStr = nullptr;
      $1->pszDescription = nullptr;
      delete_and_null($1);
   }
}
|   UCidentifier ASSIGNMENT_SYM TextualConventionAssignment
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_TEXTUAL_CONVENTION;
   $$->pszName = $1;
   $$->oid = new ObjectArray<MP_SUBID>(0, 16, Ownership::True);
   if ($3 != nullptr)
   {
      $$->iSyntax = $3->nSyntax;
      $$->pszDataType = $3->pszStr;
      $$->pszDescription = $3->pszDescription;
      $3->pszStr = nullptr;
      $3->pszDescription = nullptr;
      delete_and_null($3);
   }
}
|   UCidentifier ASSIGNMENT_SYM Type
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_TYPEDEF;
   $$->pszName = $1;
   $$->oid = new ObjectArray<MP_SUBID>(0, 16, Ownership::True);
   if ($3 != nullptr)
   {
      $$->iSyntax = $3->nSyntax;
      $$->pszDataType = $3->pszStr;
      $$->pszDescription = $3->pszDescription;
      $3->pszStr = nullptr;
      $3->pszDescription = nullptr;
      delete_and_null($3);
   }
}
|   UCidentifier ASSIGNMENT_SYM SEQUENCE_SYM SequenceAssignment
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_SEQUENCE;
   $$->pszName = $1;
   $$->oid = new ObjectArray<MP_SUBID>(0, 16, Ownership::True);
}
|   LCidentifier ASSIGNMENT_SYM SEQUENCE_SYM SequenceAssignment
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_SEQUENCE;
   $$->pszName = $1;
   $$->oid = new ObjectArray<MP_SUBID>(0, 16, Ownership::True);
}
|   UCidentifier ASSIGNMENT_SYM CHOICE_SYM SequenceAssignment
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_CHOICE;
   $$->pszName = $1;
   $$->oid = new ObjectArray<MP_SUBID>(0, 16, Ownership::True);
}
|   UCidentifier ASSIGNMENT_SYM Value
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_VALUE;
   $$->pszName = $1;
   $$->oid = new ObjectArray<MP_SUBID>(0, 16, Ownership::True);
}
;

Type:
    BuiltInType
{
   $$ = $1;
}
|   NamedType
{
   $$ = $1;
}
|   BuiltInType ValueConstraint
{
   $$ = $1;
}
|   NamedType ValueConstraint
{
   $$ = $1;
}
;

NamedType:
    SnmpTypeTagPart Identifier
{
   $$ = new MP_SYNTAX;
   $$->nSyntax = -1;
   $$->pszStr = $2;
}
;

BuiltInType:
    SnmpTypeTagPart INTEGER_SYM
{
   $$ = create_std_syntax(MIB_TYPE_INTEGER);
}
|   SnmpTypeTagPart IMPLICIT_SYM INTEGER_SYM
{
   $$ = create_std_syntax(MIB_TYPE_INTEGER);
}
|   SnmpTypeTagPart INTEGER32_SYM
{
   $$ = create_std_syntax(MIB_TYPE_INTEGER32);
}
|   SnmpTypeTagPart UNSIGNED32_SYM
{
   $$ = create_std_syntax(MIB_TYPE_UNSIGNED32);
}
|   SnmpTypeTagPart COUNTER_SYM
{
   $$ = create_std_syntax(MIB_TYPE_COUNTER);
}
|   SnmpTypeTagPart COUNTER32_SYM
{
   $$ = create_std_syntax(MIB_TYPE_COUNTER32);
}
|   SnmpTypeTagPart COUNTER64_SYM
{
   $$ = create_std_syntax(MIB_TYPE_COUNTER64);
}
|   SnmpTypeTagPart GAUGE32_SYM
{
   $$ = create_std_syntax(MIB_TYPE_GAUGE32);
}
|   SnmpTypeTagPart TIMETICKS_SYM
{
   $$ = create_std_syntax(MIB_TYPE_TIMETICKS);
}
|   SEQUENCE_SYM OF_SYM NamedType
{
   $$ = create_std_syntax(MIB_TYPE_SEQUENCE);
   delete $3;
}
|   SnmpTypeTagPart OBJECT_IDENTIFIER_SYM
{
   $$ = create_std_syntax(MIB_TYPE_OBJID);
}
|   SnmpTypeTagPart OctetStringType
{
   $$ = create_std_syntax(MIB_TYPE_OCTETSTR);
}
;

BuiltInTypeAssignment:
    IP_ADDRESS_SYM ASSIGNMENT_SYM TypeOrTextualConvention
{
   $$ = create_std_syntax(MIB_TYPE_IPADDR);
   $$->pszStr = MemCopyStringA("IpAddress");
}
|   OPAQUE_SYM ASSIGNMENT_SYM TypeOrTextualConvention
{
   $$ = create_std_syntax(MIB_TYPE_OPAQUE);
   $$->pszStr = MemCopyStringA("Opaque");
}
|   INTEGER32_SYM ASSIGNMENT_SYM TypeOrTextualConvention
{
   $$ = create_std_syntax(MIB_TYPE_INTEGER32);
   $$->pszStr = MemCopyStringA("Integer32");
}
|   UNSIGNED32_SYM ASSIGNMENT_SYM TypeOrTextualConvention
{
   $$ = create_std_syntax(MIB_TYPE_UNSIGNED32);
   $$->pszStr = MemCopyStringA("Unsigned32");
}
|   TIMETICKS_SYM ASSIGNMENT_SYM TypeOrTextualConvention
{
   $$ = create_std_syntax(MIB_TYPE_TIMETICKS);
   $$->pszStr = MemCopyStringA("TimeTicks");
}
|   COUNTER_SYM ASSIGNMENT_SYM TypeOrTextualConvention
{
   $$ = create_std_syntax(MIB_TYPE_COUNTER);
   $$->pszStr = MemCopyStringA("Counter");
}
|   COUNTER32_SYM ASSIGNMENT_SYM TypeOrTextualConvention
{
   $$ = create_std_syntax(MIB_TYPE_COUNTER32);
   $$->pszStr = MemCopyStringA("Counter32");
}
|   COUNTER64_SYM ASSIGNMENT_SYM TypeOrTextualConvention
{
   $$ = create_std_syntax(MIB_TYPE_COUNTER64);
   $$->pszStr = MemCopyStringA("Counter64");
}
|   GAUGE32_SYM ASSIGNMENT_SYM TypeOrTextualConvention
{
   $$ = create_std_syntax(MIB_TYPE_GAUGE32);
   $$->pszStr = MemCopyStringA("Gauge32");
}
;

TypeOrTextualConvention:
	Type
{
   delete $1;
}
|	TextualConventionAssignment
{
   delete $1;
}
;

MacroAssignment:
    MACRO_SYM ASSIGNMENT_SYM Begin TokenList End
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_MACRO;
   $$->pszName = $1;
   $$->oid = new ObjectArray<MP_SUBID>(0, 16, Ownership::True);
}
;

TokenList:
    TokenList TokenObject
|   TokenObject
;

TokenObject:
   TOKEN_SYM
;

TextualConventionAssignment:
   TEXTUAL_CONVENTION_SYM TextualConventionDefinition
{
   $$ = s_currentSyntaxObject;
   s_currentSyntaxObject = nullptr;
}
;

TextualConventionDefinition:
	TextualConventionDefinition TextualConventionDefinitionElement 
|	TextualConventionDefinitionElement
;

TextualConventionDefinitionElement: 
	SnmpDisplayHintStatement
|	SnmpStatusPart
|	SnmpDescriptionStatement
{
	if (s_currentSyntaxObject != nullptr)
	{
		MemFree(s_currentSyntaxObject->pszDescription);
	}
	else
	{
		s_currentSyntaxObject = new MP_SYNTAX();
	}
	s_currentSyntaxObject->pszDescription = $1;
}
|	SnmpReferenceStatement
|	SnmpSyntaxStatement
{
	if (s_currentSyntaxObject != nullptr)
	{
		s_currentSyntaxObject->nSyntax = $1->nSyntax;
		MemFree(s_currentSyntaxObject->pszStr);
		s_currentSyntaxObject->pszStr = $1->pszStr;
		$1->pszStr = nullptr;
		delete_and_null($1);
	}
	else
	{
		s_currentSyntaxObject = $1;
	}
}
;

SnmpNotificationTypeAssignment:
    Identifier NOTIFICATION_TYPE_SYM
    SnmpObjectsPart
    SnmpAccessPart
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    AssignedIdentifier
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->pszName = $1;
   $$->iSyntax = MIB_TYPE_NOTIFTYPE;
   $$->iAccess = $4;
   $$->iStatus = $5;
   $$->pszDescription = $6;
   $$->oid = $8;
   if (isupper($$->pszName[0]))
   {
      ReportError(ERR_UPPERCASE_IDENTIFIER, s_currentFilename, g_nCurrLine);
   }
}
|   Identifier TRAP_TYPE_SYM
    ENTERPRISE_SYM LCidentifier
    SnmpTrapVariablePart
    SnmpDescriptionPart
    SnmpReferencePart
    AssignedIdentifier
{
   MP_SUBID *pSubId;

   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->pszName = $1;
   $$->iSyntax = MIB_TYPE_TRAPTYPE;
   $$->pszDescription = $6;
   $$->oid = new ObjectArray<MP_SUBID>(16, 16, Ownership::True);

   pSubId = new MP_SUBID;
   pSubId->pszName = $4;
   $$->oid->add(pSubId);

   pSubId = new MP_SUBID;
   pSubId->pszName = (char *)MemAlloc(strlen($4) + 3);
   sprintf(pSubId->pszName, "%s#0", $4);
   pSubId->bResolved = TRUE;
   $$->oid->add(pSubId);

   for(int i = 0; i < $8->size(); i++)
      $$->oid->add($8->get(i));
   $8->setOwner(Ownership::False);
   delete $8;

   if (isupper($$->pszName[0]))
   {
      ReportError(ERR_UPPERCASE_IDENTIFIER, s_currentFilename, g_nCurrLine);
   }
}
;

SnmpTrapVariablePart:
    SnmpTrapVariableList
|
;

SnmpTrapVariableList:
    VARIABLES_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
   delete $3;
}
;

SnmpMandatoryGroupPart:
    SnmpMandatoryGroupList
|
;

SnmpMandatoryGroupList:
    SnmpMandatoryGroupList SnmpMandatoryGroup
|   SnmpMandatoryGroup
;

SnmpMandatoryGroup:
    MANDATORY_GROUPS_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
   delete $3;
}
;

SnmpCompliancePart:
    SnmpComplianceList
|
;

SnmpComplianceList:
    SnmpComplianceList SnmpComplianceObject
|   SnmpComplianceObject
;

SnmpComplianceObject:
    OBJECT_SYM LCidentifier
    SnmpSyntaxPart
    SnmpWriteSyntaxPart
    SnmpAccessPart
    SnmpDescriptionPart
{
   MemFreeAndNull($2);
   delete_and_null($3);
   MemFreeAndNull($6);
}
|   GROUP_SYM LCidentifier SnmpDescriptionPart
{
   MemFreeAndNull($2);
   MemFreeAndNull($3);
}
;

SnmpObjectsPart:
    OBJECTS_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
   delete_and_null($3);
}
|   OBJECTS_SYM LEFT_BRACE_SYM RIGHT_BRACE_SYM
|   NOTIFICATIONS_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
   delete_and_null($3);
}
|   NOTIFICATIONS_SYM LEFT_BRACE_SYM RIGHT_BRACE_SYM
|
;

SnmpObjectGroupAssignment:
    LCidentifier OBJECT_GROUP_SYM
    SnmpObjectsPart
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    AssignedIdentifier
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->pszName = $1;
   $$->iStatus = $4;
   $$->iSyntax = MIB_TYPE_OBJGROUP;
   $$->pszDescription = $5;
   $$->oid = $7;
}
;

SnmpNotificationGroupAssignment:
    LCidentifier NOTIFICATION_GROUP_SYM
    SnmpObjectsPart
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    AssignedIdentifier
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->pszName = $1;
   $$->iStatus = $4;
   $$->pszDescription = $5;
   $$->oid = $7;
}
;

ModuleComplianceAssignment:
    LCidentifier MODULE_COMPLIANCE_SYM
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    SnmpModuleComplianceList
    AssignedIdentifier
{
   MemFreeAndNull($1);
   MemFreeAndNull($4);
   delete_and_null($7);
}
;

SnmpModuleComplianceList:
    SnmpModuleComplianceList SnmpModuleComplianceObject
|   SnmpModuleComplianceObject
;

SnmpModuleComplianceObject:
    MODULE_SYM OptionalModuleName
    SnmpMandatoryGroupPart
    SnmpCompliancePart
;

OptionalModuleName:
	UCidentifier
{
   MemFreeAndNull($1);
}
|
;

SnmpVariationsListPart:
    SnmpVariationsList
|
;

SnmpVariationsList:
    SnmpVariationsList SnmpVariationPart
|   SnmpVariationPart
;

SnmpVariationPart:
    VARIATION_SYM LCidentifier
    SnmpSyntaxPart
    SnmpWriteSyntaxPart
    SnmpAccessPart
    SnmpCreationPart
    SnmpDefValPart
    SnmpDescriptionPart
{
   MemFreeAndNull($2);
   delete_and_null($3);
   MemFreeAndNull($8);
}
;

ModuleCapabilitiesList:
    ModuleCapabilitiesList ModuleCapabilitiesAssignment
|   ModuleCapabilitiesAssignment
;

ModuleCapabilitiesAssignment:
    SUPPORTS_SYM UCidentifier INCLUDES_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM SnmpVariationsListPart
{
   MemFreeAndNull($2);
   delete_and_null($5);
}
|    ModuleIdentifier INCLUDES_SYM SymbolList SnmpVariationsListPart
{
   MemFreeAndNull($1);
   delete_and_null($3);
}
;

AgentCapabilitiesAssignment:
    LCidentifier AGENT_CAPABILITIES_SYM
    PRODUCT_RELEASE_SYM CharString
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    ModuleCapabilitiesList
    AssignedIdentifier
{
   $$ = new MP_OBJECT;
   $$->iType = MIBC_OBJECT;
   $$->pszName = $1;
   $$->iStatus = $5;
   $$->iSyntax = MIB_TYPE_AGENTCAP;
   $$->pszDescription = $6;
   $$->oid = $9;
   MemFreeAndNull($4);
}
;

SnmpAccessPart:
    ACCESS_SYM LCidentifier
{
   $$ = AccessFromText($2);
   MemFreeAndNull($2);
}
|   MAX_ACCESS_SYM LCidentifier
{
   $$ = AccessFromText($2);
   MemFreeAndNull($2);
}
|   MIN_ACCESS_SYM LCidentifier
{
   $$ = AccessFromText($2);
   MemFreeAndNull($2);
}
|
{
   $$ = 0;
}
;

SnmpStatusPart:
    STATUS_SYM LCidentifier
{
   static const char *pStatusText[] = { "mandatory", "optional", "obsolete", "deprecated", "current", NULL };
   int i;

   for(i = 0; pStatusText[i] != NULL; i++)
      if (!stricmp(pStatusText[i], $2))
      {
         $$ = i + 1;
         break;
      }
   if (pStatusText[i] == nullptr)
   {
      char errorMessage[256];
      snprintf(errorMessage, 256, "Invalid STATUS value \"%s\"", $2);
      mperror(errorMessage);
   }
   MemFreeAndNull($2);
}
;

SnmpReferencePart:
	SnmpReferenceStatement
|
;

SnmpReferenceStatement:
	REFERENCE_SYM CharString
{
   MemFreeAndNull($2);
}
;

SnmpDisplayHintStatement:
    DISPLAY_HINT_SYM CharString
{
   MemFreeAndNull($2);
}
;

SnmpIndexPart:
    INDEX_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
   $$ = $3;
}
|   AUGMENTS_SYM LEFT_BRACE_SYM DefinedValue RIGHT_BRACE_SYM
{
   MemFreeAndNull($3);
   $$ = nullptr;
}
|
{
	$$ = nullptr;
}
;

SnmpDefValPart:
    DEFVAL_SYM LEFT_BRACE_SYM BinaryString RIGHT_BRACE_SYM
|   DEFVAL_SYM LEFT_BRACE_SYM HexString RIGHT_BRACE_SYM
|   DEFVAL_SYM LEFT_BRACE_SYM CharString RIGHT_BRACE_SYM
{
   MemFreeAndNull($3);
}
|   DEFVAL_SYM LEFT_BRACE_SYM Identifier RIGHT_BRACE_SYM
{
   MemFreeAndNull($3);
}
|   DEFVAL_SYM LEFT_BRACE_SYM DefValList RIGHT_BRACE_SYM
|   DEFVAL_SYM AssignedIdentifierList
{
   delete_and_null($2);
}
|
;

DefValList:
	DefValList COMMA_SYM DefValListElement
|	DefValListElement
;

DefValListElement:
	LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
   delete_and_null($2);
}
|	LEFT_BRACE_SYM  RIGHT_BRACE_SYM
;

BinaryString:
    BSTRING_SYM
{
   MemFreeAndNull($1);
}
;

HexString:
    HSTRING_SYM
{
   MemFreeAndNull($1);
}
;

CharString:
    CSTRING_SYM
{
   $$ = $1;
}
;

SymbolsFromModuleList:
    SymbolsFromModuleList SymbolsFromModule
{
	$$ = $1;
   $$->add($2);
}
|   SymbolsFromModule
{
   $$ = new ObjectArray<MP_IMPORT_MODULE>(16, 16, Ownership::True);
   $$->add($1);
}
;

SymbolsFromModule:
    SymbolList FROM_SYM UCidentifier
{
   $$ = new MP_IMPORT_MODULE($3, $1);
}
;

SymbolList:
    SymbolList COMMA_SYM Symbol
{
   $$->add($3);
}
|   Symbol
{
   $$ = new Array(16, 16, Ownership::True);
   $$->add($1);
}
;

Symbol:
    UCidentifier
{
   $$ = $1;
}
|   LCidentifier
{
   $$ = $1;
}
|   IMPLIED_SYM LCidentifier
{
   $$ = $2;
}
;

SequenceItem:
    Identifier Type
{
   MemFreeAndNull($1);
   delete_and_null($2);
}
;

SequenceList:
    SequenceItem
| SequenceList COMMA_SYM SequenceItem
;

SequenceAssignment:
    LEFT_BRACE_SYM SequenceList RIGHT_BRACE_SYM
;

SnmpTypeTagPart:
    SnmpTypeTagList
|
;

SnmpTypeTagList:
    SnmpTypeTagList SnmpTypeTagItem 
|   SnmpTypeTagItem
;

SnmpTypeTagItem:
    LEFT_BRACKET_SYM UCidentifier Number RIGHT_BRACKET_SYM
{
   MemFreeAndNull($2);
}
;

OctetStringType:
    OCTET_SYM STRING_SYM
|   EXPLICIT_SYM OCTET_SYM STRING_SYM
|   IMPLICIT_SYM OCTET_SYM STRING_SYM
|   IP_ADDRESS_SYM
|   NETWORK_ADDRESS_SYM
|   OPAQUE_SYM
|   BITS_SYM
;

Value:
    Number
|   HexString
|   BinaryString
|   CharString
{
   MemFreeAndNull($1);
}
;

Identifier:
   UCidentifier
{
   $$ = $1;
}
|  LCidentifier
{
   $$ = $1;
}
;

UCidentifier:
    UCASEFIRST_IDENT_SYM
{
   $$ = $1;
}
;

LCidentifier:
    LCASEFIRST_IDENT_SYM
{
   $$ = $1;
}
;

End:
    END_SYM
;

Begin:
    BEGIN_SYM
;

%%

MP_MODULE *ParseMIB(const TCHAR *fileName)
{
   m_pModule = nullptr;
   mpin = _tfopen(fileName, _T("r"));
   if (mpin != nullptr)
   {
#ifdef UNICODE
	  WideCharToMultiByteSysLocale(fileName, s_currentFilename, MAX_PATH);
#else
	  s_currentFilename = fileName;
#endif
      g_nCurrLine = 1;
      InitStateStack();
      /*mpdebug=1;*/
      ResetScanner();
      mpparse();
      fclose(mpin);
   }
   else
   {
#ifdef UNICODE
      char *name = MBStringFromWideString(fileName);
      ReportError(ERR_CANNOT_OPEN_FILE, name, strerror(errno));
      MemFree(name);
#else
      ReportError(ERR_CANNOT_OPEN_FILE, fileName, strerror(errno));
#endif
      return nullptr;
   }
   return m_pModule;
}

extern "C" int mpwrap()
{
	return 1;
}

int mperror(const char *pszMsg)
{
   ReportError(ERR_PARSER_ERROR, s_currentFilename, pszMsg, g_nCurrLine);
   return 0;
}

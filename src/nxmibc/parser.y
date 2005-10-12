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

#pragma warning(disable : 4102)

#include <string.h>
#include <time.h>
#include "ubi_dLinkList.h"
#include "mibparse.h"
#include "nxmibc.h"

#ifdef YYTEXT_POINTER
    extern char *mptext;
#else
extern char mptext[];
#endif

extern FILE *mpin, *mpout;

static SnmpParseTree *mibTree;
static char *currentFilename;

int mperror(char *pszMsg);
int mplex(void);

/*
* Binds keywords to the following object during the parsing phase
*/

void bind_keywords( ubi_dlListPtr itemList, TypeOrValue *mibItem )
{
    SnmpCommentType *keyword = NULL; 
	TypeOrValue *item = NULL;

    if ( ( mibItem->dataType == DATATYPE_VALUE ) &&
         ( ( mibItem->data.value->valueType == VALUETYPE_MODULE ) ||
           ( mibItem->data.value->valueType == VALUETYPE_IMPORTS ) ) )
    {
        /*
         * In the case of objects at the top of the mib
         * these will bind to the first object in the mib
         * ( Hopefully a module identity but you never know )
         * rather than to the mib module definition which is the true first
         * item in the mib - so we skip over it at this point.
		 *
		 * we also skip over the imports clause for the same reason
		 *
		 * This causes a problem with the check at the END symbol in the 
		 * ModuleDefinition. If a mib consists solely of a
		 * Module Identifier, Imports, a keyword and the END symbol then
		 * the keyword will not be be bound to the mib. It will sit
		 * lonely in the object list staring with longing at the other
		 * bound keywords and wonder why not it?
		 *
		 * On the other hand there's no good reason to do that
		 * I can think of so Tough Darts, Farmer!
         */
        mibItem->hasKeyword = FALSE;
	    return;
    }
	for( item = ( TypeOrValue *)ubi_dlFirst(itemList);
	     item;
		 item = ( TypeOrValue *)ubi_dlNext(item) )
    {
	    if ( ( item->dataType == DATATYPE_VALUE ) &&
	         ( item->data.value->valueType == VALUETYPE_KEYWORD ) )
	    {
            keyword = item->data.value->value.keywordValue;

    	    if( ( keyword->type == VALUETYPE_KEYWORD ) &&
    	        ( keyword->data.keyword.bindings == NULL ) )
            {
                SnmpSymbolType *binding=MT (SnmpSymbolType);
    
    		    keyword->data.keyword.bindings = lstNew();
    			if ( mibItem->dataType == DATATYPE_VALUE )
                {
                    binding->name = mibItem->data.value->name;
                    binding->lineNo =  mibItem->data.value->lineNo;
                }
    			else if ( mibItem->dataType == DATATYPE_TYPE )
                {
                    binding->name = mibItem->data.type->name;
                    binding->lineNo =  mibItem->data.type->lineNo;
                }
                binding->resolved = TRUE;
    			mibItem->hasKeyword = TRUE;
                ubi_dlAddTail(keyword->data.keyword.bindings,binding);
    	    }
	    }
	}
}

void *mpalloc(int size)
{
    void *t_ptr=malloc(size);
	if ( t_ptr == NULL )
	{
	    fprintf(stderr,"Malloc Failed for %d bytes errno=%d\n",size,errno);
		exit(1);
	}
	memset(t_ptr, 0, size);
	return t_ptr;
}

%}

/*
 *	Union structure.  A terminal or non-terminal can have
 *	one of these type values.
 */

%union
{
    char            *charPtr;
    SnmpNumberType  numberVal;
    SnmpAccess      accessVal;
    SnmpModuleType           *modulePtr;
    SnmpImportModuleType           *importModulePtr;
    SnmpModuleIdentityType   *moduleIdentity;
    SnmpObjectComplianceType *objectCompliancePtr;
    OID             *oidPtr;
    SnmpRevisionInfoType    *revisionPtr;
    SnmpSymbolType      *symbolPtr;
    SnmpObjectType  *objectTypePtr;
    SnmpStatus      statusVal;
    ubi_dlList      *listPtr;
    Type            *typePtr;
    Value           *valuePtr;
    SnmpDefValType *defValPtr;
    LineNumber      lineNoVal;
    SnmpValueConstraintType *constraintPtr;
    SnmpSequenceType *sequencePtr;
    SnmpTextualConventionType *textualConventionPtr;
    SnmpNotificationType *trapTypePtr;
    SnmpGroupType *groupTypePtr;
    SnmpCommentType *commentPtr;
    SnmpModuleCapabilitiesType *moduleCapabilitiesPtr;
    SnmpVariationType *variationPtr;
    SnmpAgentCapabilitiesType *agentCapabilitiesPtr;
    SnmpSequenceItemType *sequenceItemPtr;
    SnmpModuleComplianceType *moduleCompliancePtr;
    SnmpModuleComplianceObject *moduleComplianceObjPtr;
    SnmpTagType *tagPtr;
    TypeOrValue *typeOrValuePtr;
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

%token INTEGER_SYM
%token INTEGER32_SYM
%token UNSIGNED32_SYM
%token GAUGE_SYM
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

%token <charPtr> MACRO_SYM
%token <charPtr> MODULE_SYM
%token <charPtr> UCASEFIRST_IDENT_SYM LCASEFIRST_IDENT_SYM
%token <charPtr> BSTRING_SYM HSTRING_SYM CSTRING_SYM DISPLAY_HINT_SYM
%token <accessVal> MAX_ACCESS_SYM ACCESS_SYM MIN_ACCESS_SYM

%token <numberVal> NUMBER_SYM
%type <numberVal> Number

%type <charPtr> UCidentifier LCidentifier Identifier Symbol
%type <charPtr> BinaryString HexString CharString

%type <listPtr> AssignedIdentifier
%type <listPtr> AssignedIdentifierList

%type <modulePtr> ModuleIdentifier
%type <modulePtr> ModuleIdentifierAssignment
%type <importModulePtr> SymbolsFromModule

%type <moduleIdentity> ModuleIdentityAssignment

%type <lineNoVal> LineNo


%type <listPtr> ImportsAssignment SymbolList SnmpObjectsPart SnmpCreationPart
%type <listPtr>  SymbolsFromModuleList

%token <charPtr> TOKEN_SYM /* used by macro processing */
%type <listPtr>  TokenList /* used by macro processing */
%type <symbolPtr>  TokenObject /* used by macro processing */

%type <revisionPtr> SnmpRevisionObject
%type <accessVal> SnmpAccessPart
%type <charPtr> SnmpUnitsPart
%type <listPtr> SnmpRevisionList
%type <listPtr> SnmpRevisionPart
%type <charPtr> SnmpOrganisationPart
%type <charPtr> SnmpContactInfoPart
%type <charPtr> SnmpUpdatePart
%type <charPtr> SnmpDescriptionPart
%type <charPtr> SnmpIdentityPart
%type <charPtr> SnmpReferencePart
%type <charPtr> SnmpDisplayHintPart
%type <typePtr> SnmpSyntax SnmpSyntaxPart SnmpWriteSyntaxPart
%type <statusVal> SnmpStatusPart
%type <listPtr> SnmpIndexPart

%type <charPtr> DefinedValue
%type <typePtr> BuiltInType
%type <typePtr> Type
%type <typePtr> NamedType
%type <typePtr> OctetStringType
%type <typePtr> SequenceAssignment
%type <sequenceItemPtr> SequenceItem

%type <listPtr> SnmpTypeTagPart
%type <listPtr> SnmpTypeTagList
%type <tagPtr> SnmpTypeTagItem

%type <textualConventionPtr> TextualConventionAssignment

%type <valuePtr> Value NumericValue NumericValueItem
%type <defValPtr> SnmpDefValPart
%type <typeOrValuePtr> Assignment MacroAssignment  TypeOrValueAssignment
%type <typeOrValuePtr> BuiltInTypeAssignment
%type <trapTypePtr> SnmpNotificationTypeAssignment

%type <listPtr> ObjectIdentifierList
%type <oidPtr> ObjectIdentifier
%type <objectTypePtr> ObjectIdentifierAssignment
%type <objectTypePtr> ObjectTypeAssignment 
%type <objectTypePtr> ObjectIdentityAssignment

%type <listPtr> SequenceList
%type <listPtr> AssignmentList
%type <listPtr> SnmpVariationsList
%type <listPtr> SnmpVariationsListPart
%type <listPtr> ModuleCapabilitiesList
%type <listPtr> SnmpTrapVariablePart
%type <listPtr> SnmpTrapVariableList

%type <listPtr> NumericValueConstraintList
%type <constraintPtr> ValueConstraint

%type <groupTypePtr> SnmpObjectGroupAssignment SnmpNotificationGroupAssignment

%type <listPtr> SnmpModuleComplianceList
%type <moduleComplianceObjPtr> SnmpModuleComplianceObject

%type <listPtr> SnmpCompliancePart SnmpComplianceList
%type <objectCompliancePtr> SnmpComplianceObject

%type <groupTypePtr> SnmpMandatoryGroup
%type <listPtr> SnmpMandatoryGroupPart SnmpMandatoryGroupList

%type <moduleCompliancePtr> ModuleComplianceAssignment
%type <commentPtr> SnmpKeywordAssignment
%type <charPtr> SnmpKeywordName
%type <charPtr> SnmpKeywordValue
%type <listPtr> SnmpKeywordBinding
%type <variationPtr> SnmpVariationPart
%type <agentCapabilitiesPtr> AgentCapabilitiesAssignment
%type <moduleCapabilitiesPtr> ModuleCapabilitiesAssignment

/*
 *  Type definitions of non-terminal symbols
*/

%start ModuleDefinition
%%

LineNo:
{
    $$ = mpLineNoG;
}

/*-----------------------------------------------------------------------*/
/* Module def/import/export productions */
/*-----------------------------------------------------------------------*/

ModuleDefinition:
    AssignmentList
    End
{
    ubi_dlListPtr itemList = $1;
    TypeOrValue *item = NULL;

    mibTree = MT (SnmpParseTree);

	for( item = ( TypeOrValue *)ubi_dlFirst(itemList);
	     item;
		 item = ( TypeOrValue *)ubi_dlNext(item) )
    {
	    if ( ( item->dataType == DATATYPE_VALUE ) &&
	         ( item->data.value->valueType == VALUETYPE_MODULE ) )
		{
		    if( mibTree->name == NULL )
            {
                mibTree->name = item->data.value->value.moduleValue;
			    ubi_dlRemThis( itemList, item );
            }
			else
            {
                mperror("Multiple Module Identifier Values Found");
            }
			
		}
	    else if ( ( item->dataType == DATATYPE_VALUE ) &&
	         ( item->data.value->valueType == VALUETYPE_IMPORTS ) )
		{
		    if( mibTree->imports == NULL )
            {
                mibTree->imports = item->data.value->value.importListPtr;
			    ubi_dlRemThis( itemList, item );
            }
			else
            {
                mperror("Multiple Module Import Statements Found");
            }
			
		}
	}
	/*
	 * Any keywords between the final object and the END_SYM are bound
	 * to the last object 
	 */
	for( item = ( TypeOrValue *)ubi_dlLast(itemList);
	     item;
		 item = ( TypeOrValue *)ubi_dlPrev(item) )
    {
	    if ( ( item->dataType != DATATYPE_VALUE ) ||
	         ( item->data.value->valueType != VALUETYPE_KEYWORD ) )
	    {
	        bind_keywords(itemList,item);
	    }
    }
    mibTree->objects = $1;
    if ( mibTree->name == NULL )
    {
       mperror("Invalid Module No Module Identifier Found");
    }
}
;

ModuleIdentifierAssignment:
    ModuleIdentifier
{
    switch( $1->type )
	{
	    case VALUETYPE_MODULE:
		    $$ = $1;
			break;
	    case VALUETYPE_MODULE_IDENTITY:
	    case VALUETYPE_OID:
		default:
		    mperror("Invalid Module Identifier Assignment");
	        break;
	}
}

ModuleIdentifier:
UCidentifier DEFINITIONS_SYM ASSIGNMENT_SYM Begin LineNo
{
    $$ = MT (SnmpModuleType);
    $$->name = $1;
    $$->type = VALUETYPE_MODULE;
    $$->tagStrategy = TAG_STRATEGY_UNKNOWN;
    $$->lineNo = $5;
}
| UCidentifier DEFINITIONS_SYM ASSIGNMENT_SYM AUTOMATIC_SYM TAGS_SYM Begin LineNo
{
    $$ = MT (SnmpModuleType);
    $$->name = $1;
    $$->type = VALUETYPE_MODULE;
    $$->tagStrategy = TAG_STRATEGY_AUTOMATIC;
    $$->lineNo = $7;
}
| UCidentifier OBJECT_IDENTIFIER_SYM AssignedIdentifier LineNo
{
    $$ = MT (SnmpModuleType);
    $$->name = $1;
    $$->type = VALUETYPE_OID;
    $$->oidListPtr = $3;
    $$->tagStrategy = TAG_STRATEGY_UNKNOWN;
    $$->lineNo = $4;
}
| SUPPORTS_SYM UCidentifier LineNo
{
    $$ = MT (SnmpModuleType);
    $$->name = $2;
    $$->type = VALUETYPE_MODULE_IDENTITY;
    $$->tagStrategy = TAG_STRATEGY_UNKNOWN;
    $$->lineNo = $3;
}
| MODULE_SYM LineNo
{
    $$ = MT (SnmpModuleType);
    $$->name = $1;
    $$->type = VALUETYPE_MODULE_IDENTITY;
    $$->tagStrategy = TAG_STRATEGY_UNKNOWN;
    $$->lineNo = $2;
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
	LineNo
{
    $$ = MT (SnmpModuleIdentityType);
    $$->name = $1;
    $$->last_update = $2;
    $$->organization = $3;
    $$->contact_info = $4;
    $$->description = $5;
    $$->revisions = $6;
    $$->oidListPtr = $7;
    $$->lineNo = $8;
}
;

ImportsAssignment:
    IMPORTS_SYM SymbolsFromModuleList SEMI_COLON_SYM
{
    $$ = $2;
}
;

ExportsAssignment:
    EXPORTS_SYM DummySymbolList SEMI_COLON_SYM
;

SnmpRevisionPart:
    SnmpRevisionList
| { $$ = NULL; }
;

SnmpRevisionList:
    SnmpRevisionList SnmpRevisionObject
{
    ubi_dlAddTail($1,$2);
}
| SnmpRevisionObject
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

SnmpRevisionObject:
    REVISION_SYM CharString
    SnmpDescriptionPart
{
    $$ = MT(SnmpRevisionInfoType);
    $$->revision = $2;
    $$->description = $3;
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
	    $$ = $2;
}
;

SnmpContactInfoPart:
    CONTACT_SYM CharString
{
	    $$ = $2;
}
;

SnmpUpdatePart:
    UPDATE_SYM CharString
{
	    $$ = $2;
}
;

SnmpDescriptionPart:
    DESCRIPTION_SYM CharString
{
	    $$ = $2;
}
| { $$ = NULL; }
;

ValueConstraint:
    LEFT_PAREN_SYM NumericValueConstraintList RIGHT_PAREN_SYM
{
    $$ = MT (SnmpValueConstraintType);
    $$->type = SNMP_RANGE_CONSTRAINT;
    $$->a.valueList = $2;
}
| LEFT_PAREN_SYM SIZE_SYM LEFT_PAREN_SYM NumericValueConstraintList RIGHT_PAREN_SYM RIGHT_PAREN_SYM
{
    $$ = MT (SnmpValueConstraintType);
    $$->type = SNMP_SIZE_CONSTRAINT;
    $$->a.valueList = $4;
}
| LEFT_BRACE_SYM NumericValueConstraintList RIGHT_BRACE_SYM
{
    $$ = MT (SnmpValueConstraintType);
    $$->type = SNMP_RANGE_CONSTRAINT;
    $$->a.valueList = $2;
}
;

NumericValueConstraintList:
    NumericValueConstraintList BAR_SYM NumericValue 
{
    ubi_dlAddTail($1,$3);
}
| NumericValueConstraintList COMMA_SYM NumericValue 
{
    ubi_dlAddTail($1,$3);
}
| NumericValue 
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

SnmpKeywordAssignment:
    SnmpKeywordName LineNo
{
    $$= MT (SnmpCommentType);
    $$->type = VALUETYPE_KEYWORD;
    $$->data.keyword.name = $1;
    $$->lineNo = $2;
}
|   SnmpKeywordName SnmpKeywordValue LineNo
{
    $$= MT (SnmpCommentType);
    $$->type = VALUETYPE_KEYWORD;
    $$->data.keyword.name = $1;
    $$->data.keyword.value = $2;
    $$->lineNo = $3;
}
|   SnmpKeywordName SnmpKeywordBinding SnmpKeywordValue LineNo
{
    $$= MT (SnmpCommentType);
    $$->type = VALUETYPE_KEYWORD;
    $$->data.keyword.name = $1;
    $$->data.keyword.bindings = $2;
    $$->data.keyword.value = $3;
    $$->lineNo = $4;
}
;

SnmpKeywordName:
    KEYWORD_SYM ASSIGNMENT_SYM CharString
{
    $$ = $3;
}
;

SnmpKeywordValue:
    KEYWORD_VALUE_SYM ASSIGNMENT_SYM CharString
{
    $$ = $3;
}
;

SnmpKeywordBinding:
    KEYWORD_BIND_SYM ASSIGNMENT_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
    $$ = $4;
}
;

SnmpSyntax:
    SYNTAX_SYM Type
{
    $$ = $2;
}
;

SnmpSyntaxPart:
    SnmpSyntax
  { $$=$1; }
| { $$ = NULL; }
;

SnmpUnitsPart:
    UNITS_SYM CharString
{
    $$ = $2;
}
| { $$ = NULL; }
;

SnmpWriteSyntaxPart:
    WRITE_SYNTAX_SYM Type
{
    $$ = $2;
}
| { $$ = NULL; }
;

SnmpCreationPart:
    CREATION_REQUIRES_SYM SymbolList
{
    $$ = $2;
}
| { $$ = NULL; }
;

TypeOrValueAssignment:
    BuiltInTypeAssignment
{
    $$=$1;
}
| UCidentifier ASSIGNMENT_SYM TextualConventionAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = $1;
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
	/* propagate the implicit application tag (base type) up */
    $$->data.type->implicit = $3->syntax->implicit;
    $$->data.type->lineNo = $4;
    $$->data.type->typeData.tcValue = $3;
}
| UCidentifier ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = $1;
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = $1;
}
| UCidentifier ASSIGNMENT_SYM SEQUENCE_SYM SequenceAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = $4;
    $$->data.type->lineNo = $5;
    $$->data.type->name = $1;
    $$->data.type->implicit.tagClass = TAG_TYPE_UNIVERSAL;
    $$->data.type->implicit.className = strdup("UNIVERSAL");
    $$->data.type->implicit.tagVal = 16;
    $$->data.type->typeData.sequence->name = $1;
}
| UCidentifier ASSIGNMENT_SYM CHOICE_SYM SequenceAssignment LineNo
{
    /* 
	 * I'm not sure this is used outside of the SMI specification 
	 * documents. However the goal of the parser is to parse MIBs
	 * on a specification independent basis, setting aside the
	 * macro assignment problem. This puts the burden on the user
	 * to specify all relevant mibs (or the calling program to embed
	 * known specification mibs).
	 */
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = $4;
    $$->data.type->lineNo = $5;
    $$->data.type->name = $1;
    $$->data.type->type = SNMP_CHOICE;
    $$->data.type->implicit.tagClass = TAG_TYPE_UNIVERSAL;
    $$->data.type->implicit.className = strdup("UNIVERSAL");
    $$->data.type->implicit.tagVal = 16;
    $$->data.type->typeData.sequence->name = $1;
}
| UCidentifier ASSIGNMENT_SYM Value
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = $3;
}
;

BuiltInTypeAssignment:
    IP_ADDRESS_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("IpAddress");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("IpAddress");
}
| OPAQUE_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("Opaque");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("Opaque");
}
| INTEGER32_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("Integer32");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("Integer32");
}
| UNSIGNED32_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("Unsigned32");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("Unsigned32");
}
| TIMETICKS_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("TimeTicks");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("TimeTicks");
}
| COUNTER_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("Counter");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("Counter");
}
| COUNTER32_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("Counter32");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("Counter32");
}
| COUNTER64_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("Counter64");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("Counter64");
}
| GAUGE_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("Gauge");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("Gauge");
}
| GAUGE32_SYM ASSIGNMENT_SYM Type LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT( Type );
    $$->data.type->name = strdup("Gauge32");
    $$->data.type->type = SNMP_TEXTUAL_CONVENTION;
    $$->data.type->implicit = $3->implicit;
    $$->data.type->lineNo = $4;

    $$->data.type->typeData.tcValue = MT (SnmpTextualConventionType);
    $$->data.type->typeData.tcValue->syntax = $3;
    $$->data.type->typeData.tcValue->name = strdup("Gauge32");
}
;

MacroAssignment:
MACRO_SYM ASSIGNMENT_SYM Begin TokenList End LineNo 
{
    /* This is the macro specification problem. The simple
	 * parser does not support dynamic grammar specifications
	 * consequently all we can do is preserve the macro definition
	 * in the parse, however the parser will not process new values
	 * using a macro which is not known.
	 * 
	 * I think the future of ASN.1 macros, outside of new SNMP
	 * specifications is rather limited so all we do is preserve the
	 * token list so the macro definition can be reconstructed using
	 * the line numbers of the tokens.
	 * 
	 * It would not be that difficult to put in something to parse
	 * the macro body for syntax, its a simple assignment list. A more
	 * complex parser (check the bison docs for context dependent grammars)
	 * might be able to parse and implement macros dynamically.
	 */
    /* Known Macro Type */
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT (Type);
    $$->data.type->name = $1;
    $$->data.type->lineNo = $6;
    $$->data.type->type = SNMP_MACRO_DEFINITION;
    $$->data.type->resolved = TRUE;
    $$->data.type->typeData.macro_tokens = $4;
}
;

TokenList:
    TokenList TokenObject
{
    ubi_dlAddTail($1 ,$2);
}
| TokenObject
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}

TokenObject:
TOKEN_SYM LineNo
{
    $$ = MT (SnmpSymbolType);
    $$->name = $1;
    $$->lineNo = $2;
}
;

TextualConventionAssignment:
    TEXTUAL_CONVENTION_SYM
    SnmpDisplayHintPart
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    SnmpSyntaxPart
{
    $$ = MT( SnmpTextualConventionType );
    $$->displayHint=$2;
    $$->status=$3;
    $$->description=$4;
    $$->reference=$5;
    $$->syntax=$6;
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
    $$ = MT (SnmpObjectType);
    $$->name = $1;
    $$->syntax = $3;
    $$->units = $4;
    $$->access = $5;
    $$->status = $6;
    $$->description = $7;
    $$->reference = $8;
	/*
	 * When resolving index (post-parse) its important to differentiate between
	 * an INDEX and an AUGMENTS entry. If the index list has exactly
	 * one element and if that element is a table (SNMP_SEQUENCE_IDENTIFIER)
	 * then this is an AUGMENTS clause. Else if the index list element(s)
	 * point to one or more type values its an index.
	 */
    $$->index = $9;
	if ( $$->index != NULL )
	{
	Type *syntax = $$->syntax;
	    /*
		 * We have an augments or index entry. In that case update the 
		 * type value to indicate a sequence identifier type rather than
		 * a vanilla namedType. I'm not sure if its legal to have an
		 * augments or index entry for a named type. An example might
		 * be adding a single element to an existing table. In that case
		 * its logical to combine the OBJECT-TYPE declaration and the
		 * row entry rather than creating a seperate SEQUENCE and column
		 * object.
		 */
		 if ( syntax->type == SNMP_NAMED_TYPE )
		 {
		     syntax->type = SNMP_SEQUENCE_IDENTIFIER;
		 }
		 else if ( syntax->type != SNMP_SEQUENCE_IDENTIFIER )
		 {
         char tPtr[200];
              sprintf(tPtr,"Invalid SYNTAX %s Type (%d) for element %s",
                           $$->name,syntax->type,syntax->name);
              mperror(tPtr);
          }
	}
    $$->defVal = $10;
    $$->oidListPtr = $11;
    $$->hasGroup = FALSE;
    $$->resolved = FALSE;
}
;

ObjectIdentityAssignment:
    LCidentifier OBJECT_IDENTITY_SYM
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    AssignedIdentifier
{
    $$ = MT (SnmpObjectType);
    $$->name = $1;
    $$->status = $3;
    $$->description = $4;
    $$->reference = $5;
    $$->oidListPtr = $6;
    $$->hasGroup = FALSE;
    $$->resolved = FALSE;
}
;

SnmpNotificationTypeAssignment:
    LCidentifier NOTIFICATION_TYPE_SYM
    SnmpObjectsPart
    SnmpStatusPart
    SnmpDescriptionPart
    SnmpReferencePart
    AssignedIdentifier
{
    $$ = MT (SnmpNotificationType);
    $$->name = $1;
    $$->objects = $3;
    $$->status = $4;
    $$->description = $5;
    $$->reference = $6;
    $$->oidListPtr = $7;
}
| LCidentifier TRAP_TYPE_SYM
  ENTERPRISE_SYM LCidentifier
  SnmpTrapVariablePart
  SnmpDescriptionPart
  AssignedIdentifier
{
    OID *enterpriseOid = MT (OID);
    $$ = MT (SnmpNotificationType);
    $$->name = $1;
    $$->enterprise = $4;
    $$->objects = $5;
    $$->description = $6;
    $$->oidListPtr = $7;
	/* now synthesize the parent oid value */
    enterpriseOid->nodeName = $4;
    enterpriseOid->resolved = FALSE;
    ubi_dlAddHead($$->oidListPtr,enterpriseOid);
}
;

SnmpTrapVariablePart:
    SnmpTrapVariableList
| { $$ = NULL }
;

SnmpTrapVariableList:
    VARIABLES_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
    $$ = $3;
}
;

SnmpMandatoryGroupPart:
    SnmpMandatoryGroupList
| { $$ = NULL }
;

SnmpMandatoryGroupList:
    SnmpMandatoryGroupList SnmpMandatoryGroup
{
    ubi_dlAddTail($1 ,$2);
}
| SnmpMandatoryGroup
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

SnmpMandatoryGroup:
    MANDATORY_GROUPS_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
    $$ = MT (SnmpGroupType);
    $$->type = SNMP_GROUP_TYPE_MANDATORY;
    $$->objects = $3;
    $$->resolved = FALSE;
    $$->supported = FALSE;
}
;

SnmpCompliancePart:
    SnmpComplianceList
| { $$ = NULL }
;

SnmpComplianceList:
    SnmpComplianceList SnmpComplianceObject
{
    ubi_dlAddTail($1 ,$2);
}
| SnmpComplianceObject
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

SnmpComplianceObject:
    OBJECT_SYM LCidentifier
    SnmpSyntaxPart
    SnmpWriteSyntaxPart
    SnmpAccessPart
    SnmpDescriptionPart
{
    $$ = MT (SnmpObjectComplianceType);
    $$->name = $2;
    $$->type = COMPLIANCE_ITEM_OBJECT;
    $$->syntax = $3;
    $$->write_syntax = $4;
    $$->status = $5;
    $$->description = $6;
}
| GROUP_SYM LCidentifier SnmpDescriptionPart
{
    $$ = MT (SnmpObjectComplianceType);
    $$->name = $2;
    $$->type = COMPLIANCE_ITEM_GROUP;
    $$->description = $3;
    $$->resolved = FALSE;
    $$->supported = FALSE;
}
;

SnmpObjectsPart:
    OBJECTS_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM { $$ = $3 }
| NOTIFICATIONS_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM { $$ = $3 }
| { $$ = NULL }

SnmpObjectGroupAssignment:
    LCidentifier OBJECT_GROUP_SYM
    SnmpObjectsPart
    SnmpStatusPart
    SnmpDescriptionPart
    AssignedIdentifier
{
    $$ = MT (SnmpGroupType);
    $$->name = $1;
    $$->type = SNMP_GROUP_TYPE_OBJECT;
    $$->objects = $3;
    $$->status = $4;
    $$->description = $5;
    $$->resolved = FALSE;
    $$->supported = FALSE;
    $$->oidListPtr = $6;
}
;

SnmpNotificationGroupAssignment:
    LCidentifier NOTIFICATION_GROUP_SYM
    SnmpObjectsPart
    SnmpStatusPart
    SnmpDescriptionPart
    AssignedIdentifier
{
    $$ = MT (SnmpGroupType);
    $$->name = $1;
    $$->type = SNMP_GROUP_TYPE_NOTIFICATION;
    $$->objects = $3;
    $$->status = $4;
    $$->description = $5;
    $$->resolved = FALSE;
    $$->supported = FALSE;
    $$->oidListPtr = $6;
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
    $$ = MT (SnmpModuleComplianceType);
    $$->name = $1;
    $$->status = $3;
    $$->description = $4;
    $$->reference = $5;
    $$->compliance_objects = $6;
    $$->oidListPtr = $7;
}
;

SnmpModuleComplianceList:
    SnmpModuleComplianceList SnmpModuleComplianceObject
{
    ubi_dlAddTail($1,$2);
}
| SnmpModuleComplianceObject
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

SnmpModuleComplianceObject:
    ModuleIdentifier
    SnmpMandatoryGroupPart
    SnmpCompliancePart
{
    $$ = MT (SnmpModuleComplianceObject);
    $$->module = $1;
    $$->mandatory_groups = $2;
    $$->compliance_items = $3;
}
;

SnmpVariationsListPart:
    SnmpVariationsList
| { $$ = NULL }
;

SnmpVariationsList:
    SnmpVariationsList SnmpVariationPart
{
    ubi_dlAddTail($1,$2);
}
| SnmpVariationPart
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

SnmpVariationPart:
    VARIATION_SYM LCidentifier
    SnmpSyntaxPart
    SnmpWriteSyntaxPart
    SnmpAccessPart
    SnmpCreationPart
    SnmpDefValPart
{
    $$ = MT (SnmpVariationType);
    $$->object_name = $2;
    $$->syntax = $3;
    $$->write_syntax = $4;
    $$->access = $5;
    $$->creation = $6;
    $$->defval = $7;
}
;

ModuleCapabilitiesList:
    ModuleCapabilitiesList ModuleCapabilitiesAssignment
{
    ubi_dlAddTail($1,$2);
}
| ModuleCapabilitiesAssignment
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

ModuleCapabilitiesAssignment:
	ModuleIdentifier
    INCLUDES_SYM SymbolList
    SnmpVariationsListPart
{
    $$ = MT (SnmpModuleCapabilitiesType);
    $$->module = $1;
    $$->supported_groups = $3;
    $$->variations = $4;
}
;

AgentCapabilitiesAssignment:
    LCidentifier AGENT_CAPABILITIES_SYM
    PRODUCT_RELEASE_SYM CharString
    SnmpStatusPart
    SnmpDescriptionPart
    ModuleCapabilitiesList
    AssignedIdentifier
{
    $$ = MT (SnmpAgentCapabilitiesType);
    $$->name = $1;
    $$->product_release = $4;
    $$->status = $5;
    $$->description = $6;
    $$->module_capabilities = $7;
    $$->oidListPtr = $8;
}
;

SnmpAccessPart:
  ACCESS_SYM LCidentifier
| MAX_ACCESS_SYM LCidentifier
| MIN_ACCESS_SYM LCidentifier
{
    if (strcmp ($2, "read-only") == 0)
    $$ = SNMP_READ_ONLY;
    else if (strcmp ($2, "read-write") == 0)
    $$ = SNMP_READ_WRITE;
    else if (strcmp ($2, "read-create") == 0)
    $$ = SNMP_READ_WRITE;
    else if (strcmp ($2, "not-accessible") == 0)
    $$ = SNMP_NOT_ACCESSIBLE;
    else if (strcmp ($2, "accessible-for-notify") == 0)
    $$ = SNMP_ACCESSIBLE_FOR_NOTIFY;
    else
    {
        mperror ("ACCESS field can only be one of \"read-write\", \"write-only\" , \"not-accessible\" , \"accessible-for-notify\" or \"not-implemented\"");
        $$ = -1;
    }
}
| { $$ = SNMP_ACCESS_VALUE_NULL; }
;


SnmpStatusPart:
    STATUS_SYM LCidentifier
{
    if (strcmp ($2, "mandatory") == 0)
    $$ = SNMP_MANDATORY;
    else if (strcmp ($2, "optional") == 0)
    $$ = SNMP_OPTIONAL;
    else if (strcmp ($2, "obsolete") == 0)
    $$ = SNMP_OBSOLETE;
    else if (strcmp ($2, "current") == 0)
    $$ = SNMP_CURRENT;
    else if (strcmp ($2, "deprecated") == 0)
    $$ = SNMP_DEPRECATED;
    else
    {
        mperror ("STATUS field can only be one of \"optional\", \"obsolete\" or \"deprecated\" \"current\"");
        $$ = -1;
   }
}
;

SnmpReferencePart:
    REFERENCE_SYM CharString
{ $$ = $2; }
| { $$ = NULL; }
;

SnmpDisplayHintPart:
    DISPLAY_HINT_SYM CharString
{
    $$ = $2;
}
| { $$ = NULL; }
;

SnmpIndexPart:
    INDEX_SYM LEFT_BRACE_SYM SymbolList RIGHT_BRACE_SYM
{
    $$  = $3;
}
| AUGMENTS_SYM LEFT_BRACE_SYM DefinedValue RIGHT_BRACE_SYM LineNo
{
    SnmpSymbolType *ie=MT (SnmpSymbolType);
    ie->name = $3;
    ie->resolved = FALSE;
    ie->lineNo = $5;
    $$ = lstNew();
    ubi_dlAddTail($$,ie);
}
| { $$ = NULL; }
;

SnmpDefValPart:
    DEFVAL_SYM LEFT_BRACE_SYM NumericValueItem RIGHT_BRACE_SYM
{
    $$ = MT( SnmpDefValType );

    if ( $3->valueType == VALUETYPE_INTEGER )
	{
        $$->type = VALUETYPE_INTEGER;
        $$->defVal.intVal = $3->value.intVal;
	}
    else if ( $3->valueType == VALUETYPE_NAMED_NUMBER )
	{
        $$->type = VALUETYPE_NAMED_NUMBER;
        $$->defVal.namedNumber.name = $3->name;
        $$->defVal.namedNumber.intVal = $3->value.intVal;
	}
    else if ( $3->valueType == VALUETYPE_NAMED_VALUE )
	{
	int len=$3->value.strVal.len;
	char *src = $3->value.strVal.text;
	char *dst = $$->defVal.strVal.text;

        $$->type = VALUETYPE_NAMED_VALUE;
        $$->defVal.strVal.type = SNMP_ASCII_STRING;
        $$->defVal.strVal.text = (char *)malloc( len );
        $$->defVal.strVal.len = len;
		/* yes, this is exactly to make this fit on one line */
		memcpy(src, dst, len);
	}
    else if ( $3->valueType == VALUETYPE_RANGE_VALUE )
	{
	   char tPtr[200];
	   sprintf(tPtr,"Invalid DEFVAL Specification %ld...%ld",
                    $3->value.rangeValue.lowerEndValue,
	                $3->value.rangeValue.upperEndValue );
	    mperror(tPtr);
		$$=NULL;
	}
	free($3);

}
|  DEFVAL_SYM LEFT_BRACE_SYM BinaryString RIGHT_BRACE_SYM
{
    $$ = MT( SnmpDefValType );
    $$->type = VALUETYPE_OCTET_STRING;
    $$->defVal.strVal.type = SNMP_BINARY_STRING;
    $$->defVal.strVal.text = $3;
    $$->defVal.strVal.len = strlen($3);
}
| DEFVAL_SYM LEFT_BRACE_SYM HexString RIGHT_BRACE_SYM
{
    $$ = MT( SnmpDefValType );
    $$->type = VALUETYPE_OCTET_STRING;
    $$->defVal.strVal.type = SNMP_HEX_STRING;
    $$->defVal.strVal.text = $3;
    $$->defVal.strVal.len = strlen($3);
}
| DEFVAL_SYM LEFT_BRACE_SYM CharString RIGHT_BRACE_SYM
{
    $$ = MT( SnmpDefValType );
    $$->type = VALUETYPE_OCTET_STRING;
    $$->defVal.strVal.type = SNMP_ASCII_STRING;
    $$->defVal.strVal.text = $3;
    $$->defVal.strVal.len = strlen($3);
}
| DEFVAL_SYM AssignedIdentifierList
{
    /* This is the default value type for Object Identifier types */
    $$ = MT( SnmpDefValType );
    $$->type = VALUETYPE_OID;
    $$->defVal.oidListPtr = $2;
}
| { $$ = NULL; }
;

ObjectIdentifierAssignment:
    LCidentifier OBJECT_IDENTIFIER_SYM AssignedIdentifier LineNo
{
    $$ = MT (SnmpObjectType);
    $$->name = $1;
    $$->oidListPtr = $3;
    $$->hasGroup = FALSE;
    $$->resolved = FALSE;
}
| LCidentifier AssignedIdentifier LineNo
{
    $$ = MT (SnmpObjectType);
    $$->name = $1;
    $$->oidListPtr = $2;
    $$->hasGroup = FALSE;
    $$->resolved = FALSE;
}
;


BinaryString:
    BSTRING_SYM
;

HexString:
    HSTRING_SYM
;

CharString:
    CSTRING_SYM
;

AssignedIdentifierList:
    LEFT_BRACE_SYM ObjectIdentifierList RIGHT_BRACE_SYM
{
    $$ = $2;
}
;

AssignedIdentifier:
    ASSIGNMENT_SYM AssignedIdentifierList
{
    $$ = $2;
}
| ASSIGNMENT_SYM Number
{
    OID *oid = MT (OID);
    oid->nodeName = NULL;
	if ( $2.type == SNMP_INTEGER32 )
	{
        oid->oidVal = $2.data.intVal;
	}
    $$ = lstNew();
    ubi_dlAddTail($$,oid);
}
;

ObjectIdentifierList:
    ObjectIdentifierList ObjectIdentifier
{
    ubi_dlAddTail($1 ,$2);
}
| ObjectIdentifier
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

ObjectIdentifier:
    NumericValue
{
    $$ = MT (OID);
    if ( $1->valueType == VALUETYPE_INTEGER )
	{
        $$->nodeName = NULL;
        $$->oidVal = $1->value.intVal;
	}
    else if ( $1->valueType == VALUETYPE_NAMED_NUMBER )
	{
        $$->nodeName = $1->name;
        $$->oidVal = $1->value.intVal;
	}
    else if ( $1->valueType == VALUETYPE_NAMED_VALUE )
	{
        $$->nodeName = (char *)malloc( $1->value.strVal.len + 1 );
		sprintf($$->nodeName,"%.*s",$1->value.strVal.len,$1->value.strVal.text);
        $$->oidVal = 0;
	}
    else if ( $1->valueType == VALUETYPE_RANGE_VALUE )
	{
	   char tPtr[200];
	   sprintf(tPtr,"Invalid OID Specification %ld...%ld",
                    $1->value.rangeValue.lowerEndValue,
	                $1->value.rangeValue.upperEndValue );
	    mperror(tPtr);
		$$=NULL;
	}
	free($1);
}
;

SymbolsFromModuleList:
    SymbolsFromModuleList SymbolsFromModule
{
    ubi_dlAddTail($1 ,$2);
}
| SymbolsFromModule
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

SymbolsFromModule:
    SymbolList FROM_SYM LineNo UCidentifier
{
    $$ = MT (SnmpImportModuleType);
    $$->module.name   = $4;
    $$->lineNo = $3;
    $$->imports = $1;
}
;

SymbolList:
    SymbolList COMMA_SYM Symbol LineNo
{
    SnmpSymbolType *ie=MT (SnmpSymbolType);
    ie->name = $3;
    ie->resolved = FALSE;
    ie->lineNo = $4;
    ubi_dlAddTail($1,ie);
	$$=$1
}
| Symbol LineNo
{
    SnmpSymbolType *ie=MT (SnmpSymbolType);
    ie->name = $1;
    ie->resolved = FALSE;
    ie->lineNo = $2;
    /* called for the first element only, so create list head */
    $$ = lstNew();
    ubi_dlAddTail($$,ie);
}
;

Symbol:
  UCidentifier { $$ = $1 }
| LCidentifier { $$ = $1 }
| IMPLIED_SYM LCidentifier { $$ = $2 }
;

DummySymbolList:
    DummySymbolList COMMA_SYM DummySymbol LineNo
| DummySymbol LineNo
;

DummySymbol:
  UCidentifier
| LCidentifier
| IMPLIED_SYM LCidentifier
;

AssignmentList:
    AssignmentList Assignment
{
    ubi_dlAddTail($1,$2);
	if ( ( $2->dataType != DATATYPE_VALUE ) ||
	     ( $2->data.value->valueType != VALUETYPE_KEYWORD ) )
	{
	    bind_keywords($1,$2);
	}
    $$ = $1;
}
| Assignment
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
	if ( ( $1->dataType != DATATYPE_VALUE ) ||
	     ( $1->data.value->valueType != VALUETYPE_KEYWORD ) )
	{
	    bind_keywords($$,$1);
	}
}
;

Assignment:
    ObjectIdentifierAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = $1->name;
    $$->data.value->valueType = VALUETYPE_OID;
    $$->data.value->value.oidListPtr = $1->oidListPtr;
    $$->data.value->lineNo = $2;
}
| ObjectIdentityAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = $1->name;
    $$->data.value->valueType = VALUETYPE_OID;
    $$->data.value->value.oidListPtr = $1->oidListPtr;
    $$->data.value->lineNo = $2;
}
| ObjectTypeAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT(Value);
    $$->data.value->valueType = VALUETYPE_OBJECT_TYPE;
    $$->data.value->name = $1->name;
    $$->data.value->lineNo = $2;
    $$->data.value->value.object = $1;
}
| MacroAssignment LineNo
{
    $$=$1;
    $$->data.value->lineNo = $2;
}
| TypeOrValueAssignment LineNo
{
    $$=$1;
    $$->data.value->lineNo = $2;
}
| SnmpNotificationTypeAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = $1->name;
    $$->data.value->valueType = VALUETYPE_NOTIFICATION;
    $$->data.value->value.notificationInfo = $1;
    $$->data.value->lineNo = $2;
}
| ModuleComplianceAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_TYPE;
    $$->hasKeyword = FALSE;
    $$->data.type = MT ( Type );
    $$->data.type->name = $1->name;
    $$->data.type->type = SNMP_MODULE_COMPLIANCE;
    $$->data.type->typeData.moduleCompliance = $1;
    $$->data.type->lineNo = $2;
}
| SnmpNotificationGroupAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = $1->name;
    $$->data.value->valueType = VALUETYPE_GROUP_VALUE;
    $$->data.value->value.group = $1;
    $$->data.value->lineNo = $2;
}
| SnmpObjectGroupAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = $1->name;
    $$->data.value->valueType = VALUETYPE_GROUP_VALUE;
    $$->data.value->value.group = $1;
    $$->data.value->lineNo = $2;
}
| SnmpKeywordAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = $1->data.keyword.name;
    $$->data.value->valueType = VALUETYPE_KEYWORD;
    $$->data.value->value.keywordValue = $1;
    $$->data.value->lineNo = $2;
}
| AgentCapabilitiesAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = $1->name;
    $$->data.value->valueType = VALUETYPE_CAPABILITIES;
    $$->data.value->value.agentCapabilitiesValue = $1;
    $$->data.value->lineNo = $2;
}
| ModuleIdentityAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = $1->name;
    $$->data.value->valueType = VALUETYPE_MODULE_IDENTITY;
    $$->data.value->value.moduleIdentityValue = $1;
    $$->data.value->lineNo = $2;
}
| ModuleIdentifierAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = $1->name;
    $$->data.value->valueType = VALUETYPE_MODULE;
    $$->data.value->value.moduleValue = $1;
    $$->data.value->lineNo = $2;
}
| ImportsAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_VALUE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = NULL;
    $$->data.value->valueType = VALUETYPE_IMPORTS;
    $$->data.value->value.importListPtr = $1;
    $$->data.value->lineNo = $2;
}
| ExportsAssignment LineNo
{
    $$ = MT (TypeOrValue);
    $$->dataType = DATATYPE_NONE;
    $$->hasKeyword = FALSE;
    $$->data.value = MT( Value );
    $$->data.value->name = NULL;
    $$->data.value->valueType = VALUETYPE_NULL;
    $$->data.value->lineNo = $2;
}
;

Type:
    BuiltInType
{
    $$=$1;
}
| NamedType
{
    $$=$1;
}
| BuiltInType ValueConstraint
{
    $$=$1;
    $$->typeData.values=$2;
}
| NamedType ValueConstraint
{
    $$=$1;
    $$->typeData.values=$2;
}
;

NamedType:
    SnmpTypeTagPart UCidentifier LineNo
{
    $$ = MT (Type);
    $$->tags = $1;
    $$->name = $2;
    $$->lineNo = $3;
    $$->type = SNMP_NAMED_TYPE;
    $$->resolved = FALSE;
	/* the base type of named types is unknown
	 * until post parse resolution
	 */
}
;

BuiltInType:
    SnmpTypeTagPart INTEGER_SYM LineNo
{
    $$ = MT (Type);
    $$->name = strdup("INTEGER");
    $$->type = SNMP_INTEGER;
    $$->implicit.tagClass = TAG_TYPE_UNIVERSAL;
    $$->implicit.className = strdup("UNIVERSAL");
    $$->implicit.tagVal = 2;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SnmpTypeTagPart IMPLICIT_SYM INTEGER_SYM LineNo
{
    $$ = MT (Type);
    $$->name = strdup("INTEGER");
    $$->type = SNMP_INTEGER;
    $$->implicit.tagClass = TAG_TYPE_UNIVERSAL;
    $$->implicit.className = strdup("UNIVERSAL");
    $$->implicit.tagVal = 2;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $4;
}
| SnmpTypeTagPart INTEGER32_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_INTEGER32;
    $$->name = strdup("Integer32");
    $$->implicit.tagClass = TAG_TYPE_UNIVERSAL;
    $$->implicit.className = strdup("UNIVERSAL");
    $$->implicit.tagVal = 2;
    $$->resolved = TRUE;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.valueRange.lowerEndValue = 0x80000000;
    $$->typeData.values->a.valueRange.upperEndValue = 0x7FFFFFFF;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SnmpTypeTagPart UNSIGNED32_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_UNSIGNED32;
    $$->name = strdup("Unsigned32");
    $$->implicit.tagClass = TAG_TYPE_UNIVERSAL;
    $$->implicit.className = strdup("UNIVERSAL");
    $$->implicit.tagVal = 2;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.valueRange.lowerEndValue = 0;
    $$->typeData.values->a.valueRange.upperEndValue = 0xFFFFFFFF;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SnmpTypeTagPart COUNTER_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_COUNTER;
    $$->name = strdup("Counter");
    $$->implicit.tagClass = TAG_TYPE_APPLICATION;
    $$->implicit.className = strdup("APPLICATION");
    $$->implicit.tagVal = 1;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.valueRange.lowerEndValue  = 0;
    $$->typeData.values->a.valueRange.upperEndValue = 0xFFFFFFFF;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SnmpTypeTagPart COUNTER32_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_COUNTER32;
    $$->name = strdup("Counter32");
    $$->implicit.tagClass = TAG_TYPE_APPLICATION;
    $$->implicit.className = strdup("APPLICATION");
    $$->implicit.tagVal = 1;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.valueRange.lowerEndValue  = 0;
    $$->typeData.values->a.valueRange.upperEndValue = 0xFFFFFFFF;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SnmpTypeTagPart COUNTER64_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_COUNTER64;
    $$->name = strdup("Counter64");
    $$->implicit.tagClass = TAG_TYPE_APPLICATION;
    $$->implicit.className = strdup("APPLICATION");
    $$->implicit.tagVal = 6;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.bigValueRange.lowerEndValue = 0;
    $$->typeData.values->a.bigValueRange.upperEndValue = 0xFFFFFFFFFFFFFFFF;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SnmpTypeTagPart GAUGE_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_GAUGE;
    $$->name = strdup("Gauge");
    $$->implicit.tagClass = TAG_TYPE_APPLICATION;
    $$->implicit.className = strdup("APPLICATION");
    $$->implicit.tagVal = 2;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.valueRange.lowerEndValue  = 0;
    $$->typeData.values->a.valueRange.upperEndValue = 0xFFFFFFFF;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SnmpTypeTagPart GAUGE32_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_GAUGE32;
    $$->name = strdup("Gauge32");
    $$->implicit.tagClass = TAG_TYPE_APPLICATION;
    $$->implicit.className = strdup("APPLICATION");
    $$->implicit.tagVal = 2;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.valueRange.lowerEndValue  = 0;
    $$->typeData.values->a.valueRange.upperEndValue = 0xFFFFFFFF;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SnmpTypeTagPart TIMETICKS_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_TIME_TICKS;
    $$->name = strdup("TimeTicks");
    $$->implicit.tagClass = TAG_TYPE_APPLICATION;
    $$->implicit.className = strdup("APPLICATION");
    $$->implicit.tagVal = 3;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.valueRange.lowerEndValue  = 0;
    $$->typeData.values->a.valueRange.upperEndValue = 0xFFFFFFFF;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SEQUENCE_SYM OF_SYM NamedType
{
    $$ = $3;
	/*
	 * This does not just identify a named type, it identifies
	 * a specific class of named type.
	 */
	$$->type = SNMP_SEQUENCE_IDENTIFIER;
    $$->implicit.tagClass = TAG_TYPE_UNIVERSAL;
    $$->implicit.className = strdup("UNIVERSAL");
    $$->implicit.tagVal = 16;
}
| SnmpTypeTagPart OBJECT_IDENTIFIER_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_OID;
    $$->name = strdup("OBJECT IDENTIFIER");
    $$->implicit.tagClass = TAG_TYPE_UNIVERSAL;
    $$->implicit.className = strdup("UNIVERSAL");
    $$->implicit.tagVal = 6;
    $$->resolved = TRUE;
    $$->tags = $1;
    $$->lineNo = $3;
}
| SnmpTypeTagPart OctetStringType LineNo
{
    $$ = $2;
    $$->implicit.tagClass = TAG_TYPE_UNIVERSAL;
    $$->implicit.className = strdup("UNIVERSAL");
    $$->implicit.tagVal = 4;
    $$->tags = $1;
    $$->lineNo = $3;
}
;

DefinedValue:
    UCidentifier DOT_SYM LCidentifier
{
    void *t_ptr=(void *)mpalloc(strlen((void *)$1)+strlen((void *)$3)+2);
    sprintf(t_ptr,"%s.%s",$1,$3);
    $$=t_ptr;
}
| LCidentifier  /* a named elmt ref  */
{
    $$=$1;
}
;


SequenceItem:
    Identifier LineNo Type 
{
    $$ = MT (SnmpSequenceItemType);
    $$->name = $1;
    $$->resolved = FALSE;
/* 
 * Both the sequence item and the sequence item
 * type should be resolvable (and resolved) by the post parser.
 * otherwise there is probably a syntax error in the mib
 */
    $$->lineNo = $2;
    $$->type = $3;
}
;

SequenceList:
    SequenceItem
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
| SequenceList COMMA_SYM SequenceItem
{
    ubi_dlAddTail( $1, $3 );
    $$ = $1;
}
;

SequenceAssignment:
    LEFT_BRACE_SYM SequenceList RIGHT_BRACE_SYM
{
    $$ = MT (Type);
    $$->type = SNMP_SEQUENCE;
    $$->resolved = TRUE;
    $$->typeData.sequence = MT(SnmpSequenceType);
    $$->typeData.sequence->sequence = $2;
}
;

SnmpTypeTagPart:
    SnmpTypeTagList
| { $$ = NULL; }
;

SnmpTypeTagList:
    SnmpTypeTagList SnmpTypeTagItem 
{
    ubi_dlAddTail( $1, $2 );
    $$ = $1;
}
| SnmpTypeTagItem
{
    $$ = lstNew();
    ubi_dlAddTail($$,$1);
}
;

SnmpTypeTagItem:
    LEFT_BRACKET_SYM UCidentifier Number RIGHT_BRACKET_SYM
{
    $$ = MT( SnmpTagType );

    if( strcmp( $2, "UNIVERSAL" ) )
    {
         $$->tagClass = TAG_TYPE_UNIVERSAL;
         $$->className = strdup("UNIVERSAL");
    }
    else if( strcmp( $2, "APPLICATION" ) )
    {
         $$->tagClass = TAG_TYPE_APPLICATION;
         $$->className = strdup("APPLICATION");
    }
    else if( strcmp( $2, "PRIVATE" ) )
    {
         $$->tagClass = TAG_TYPE_PRIVATE;
         $$->className = strdup("PRIVATE");
    }
    else
    {
         $$->tagClass = TAG_TYPE_CONTEXT;
         $$->className = strdup($2);
    }
	if ( $3.type == SNMP_INTEGER32 )
	{
        $$->tagVal = $3.data.intVal;
	}
}
;

OctetStringType:
    OCTET_SYM STRING_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_OCTET_STRING;
    $$->name = strdup("OCTET STRING");
    $$->resolved = TRUE;
    $$->implied = FALSE;
    $$->lineNo = $3;
}
| EXPLICIT_SYM OCTET_SYM STRING_SYM LineNo
{
    $$ = MT (Type);
    $$->name = strdup("EXPLICIT OCTET STRING");
    $$->type = SNMP_OCTET_STRING;
    $$->resolved = TRUE;
    $$->implied = FALSE;
    $$->lineNo = $4;
}
| IMPLICIT_SYM OCTET_SYM STRING_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_OCTET_STRING;
    $$->name = strdup("IMPLICIT OCTET STRING");
    $$->resolved = TRUE;
    $$->implied = FALSE;
    $$->lineNo = $4;
}
| IP_ADDRESS_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_IP_ADDR;
    $$->name = strdup("IpAddress");
    $$->resolved = TRUE;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.valueRange.lowerEndValue  = 0;
    $$->typeData.values->a.valueRange.upperEndValue = 0x4;
    $$->lineNo = $2;
}
| NETWORK_ADDRESS_SYM LineNo
{
    $$ = MT (Type);
    $$->name = strdup("NetworkAddress");
    $$->type = SNMP_NETWORK_ADDR;
    $$->resolved = TRUE;
	$$->typeData.values = MT ( SnmpValueConstraintType );
    $$->typeData.values->a.valueRange.lowerEndValue  = 0;
    $$->typeData.values->a.valueRange.upperEndValue = 0x4;
    $$->lineNo = $2;
}
| OPAQUE_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_OPAQUE_DATA;
    $$->name = strdup("Opaque");
    $$->resolved = TRUE;
    $$->lineNo = $2;
}
| BITS_SYM LineNo
{
    $$ = MT (Type);
    $$->type = SNMP_BITS;
    $$->name = strdup("BITS");
    $$->resolved = TRUE;
    $$->lineNo = $2;
}
;

Value:
    Number         /* IntegerValue  or "0" real val*/
{
    $$ = MT( Value );
    $$->valueType = VALUETYPE_INTEGER;
	if ( $1.type == SNMP_INTEGER32 )
	{
        $$->value.int32Val = $1.data.intVal;
	}
}
| HexString    /* OctetStringValue or BinaryStringValue */
{
    $$ = MT( Value );
    $$->valueType = VALUETYPE_OCTET_STRING;
    $$->value.strVal.type = SNMP_HEX_STRING;
    $$->value.strVal.text = $1;
    $$->value.strVal.len = strlen($1);
}
| BinaryString    /*  BinaryStringValue */
{
    $$ = MT( Value );
    $$->valueType = VALUETYPE_OCTET_STRING;
    $$->value.strVal.type = SNMP_BINARY_STRING;
    $$->value.strVal.text = $1;
    $$->value.strVal.len = strlen($1);
}
| CharString
{
    $$ = MT( Value );
    $$->valueType = VALUETYPE_OCTET_STRING;
    $$->value.strVal.type = SNMP_ASCII_STRING;
    $$->value.strVal.text = $1;
    $$->value.strVal.len = strlen($1);
}
;

NumericValueItem:
    LEFT_BRACE_SYM NumericValue RIGHT_BRACE_SYM
{
    /* 
	 * This is actually required (I guess) by the compiler
	 * Otherwise the DEFVAL clause can't tell the difference
	 * between a default value which is a numeric value and
	 * an OID List
	 */
    $$ = $2;
}
;

NumericValue:
    Number
{
    $$ = MT( Value );
    $$->valueType = VALUETYPE_INTEGER;
	if ( $1.type == SNMP_INTEGER32 )
	{
        $$->value.int32Val = $1.data.intVal;
	}
}
| DefinedValue
{
    $$ = MT( Value );
    $$->valueType = VALUETYPE_NAMED_VALUE;
    $$->value.strVal.type = SNMP_ASCII_STRING;
    $$->value.strVal.text = (char *)$1;
    $$->value.strVal.len = strlen((char *)$1);
}
| Number DOT_SYM DOT_SYM Number 
{
    $$ = MT( Value );
    $$->valueType = VALUETYPE_RANGE_VALUE;
	if ( $1.type == SNMP_INTEGER32 )
	{
        $$->value.rangeValue.lowerEndValue  = $1.data.intVal;
	}
	if ( $4.type == SNMP_INTEGER32 )
	{
        $$->value.rangeValue.upperEndValue = $4.data.intVal;
	}
}
| Identifier LEFT_PAREN_SYM Number RIGHT_PAREN_SYM
{
    $$ = MT( Value );
    $$->valueType = VALUETYPE_NAMED_NUMBER;
    $$->name = $1;
	if ( $3.type == SNMP_INTEGER32 )
	{
        $$->value.int32Val = $3.data.intVal;
	}
}
;

Number:
    NUMBER_SYM
{
    $$=$1;
}
;

Identifier:
   UCidentifier
|  LCidentifier
;

UCidentifier:
    UCASEFIRST_IDENT_SYM
;

LCidentifier:
    LCASEFIRST_IDENT_SYM
;

End:
    END_SYM
;

Begin:
    BEGIN_SYM
;

%%

SnmpParseTree *mpParseMib(char *filename)
{
   mpLineNoG = 1;
   mpin = fopen(filename, "r");
   if(mpin != NULL)
   {
	   currentFilename = filename;
	   mpparse();
   }
   else
   {
      Error(ERR_CANNOT_OPEN_FILE, filename, strerror(errno));
      return NULL;
   }
   return mibTree;
}

int mpwrap()
{
	return(1);
}

int mperror(char *s)
{
   Error(ERR_PARSER_ERROR, currentFilename, s, mpLineNoG);
   return 0;
}

#ifndef MIBPARSE_H
#define MIBPARSE_H
/*
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

#include <nms_common.h>
#include "ubi_dLinkList.h"

#define YYERROR_VERBOSE

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE 
#define FALSE !(TRUE)
#endif
#ifndef OK
#define OK 1
#endif
#ifndef ERROR
#define ERROR -1
#endif

typedef BOOL TruthValue;

#define MAX_COMMENT_LEN 8098

#define LOCAL static;
#define lstNew() ubi_dlInitList((ubi_dlListPtr)malloc(sizeof(ubi_dlList)))

typedef int LineNumber;

extern LineNumber mpLineNoG;

typedef enum value_type_enum
{
    VALUETYPE_UNKNOWN,
    VALUETYPE_EMPTY,
    VALUETYPE_MODULE,
    VALUETYPE_MODULE_IDENTITY,
    VALUETYPE_IMPORTS,
    VALUETYPE_IMPORT_ITEM,
    VALUETYPE_COMMENT,
    VALUETYPE_KEYWORD,
    VALUETYPE_CAPABILITIES,
    VALUETYPE_INTEGER,
    VALUETYPE_INTEGER32,
    VALUETYPE_BOOLEAN,
    VALUETYPE_OCTET_STRING,
    VALUETYPE_OID,
    VALUETYPE_OBJECT_TYPE,
    VALUETYPE_NAMED_VALUE,
    VALUETYPE_RANGE_VALUE,
    VALUETYPE_SIZE_VALUE,
    VALUETYPE_GROUP_VALUE,
    VALUETYPE_NAMED_NUMBER,
    VALUETYPE_NOTIFICATION,
    VALUETYPE_NULL,
    VALUETYPE_EXPORTS
} ValueType;

typedef enum {
    OBJECT_TYPE_UNKNOWN,
    OBJECT_TYPE_TRUNK,  /* Set of and Sequence of Objects */
    OBJECT_TYPE_BRANCH, /* groups of individual objects, Entries or Groups */
    OBJECT_TYPE_LEAF,   /* Individual objects */
    OBJECT_TYPE_LAST
} ObjectType;

typedef enum {
    TAG_TYPE_UNKNOWN,
    TAG_TYPE_UNIVERSAL,  /* Reserved for Standards Organisation */
    TAG_TYPE_APPLICATION, /* Generally Application Globals */
    TAG_TYPE_PRIVATE,   /* Secret Stuff */
    TAG_TYPE_CONTEXT,   /* Lists of various types */
    TAG_TYPE_LAST
} TagClassType;

typedef enum
{
    SNMP_UNKNOWN,
    SNMP_IMPORT_ITEM,
    SNMP_OID,
    SNMP_BITS,
    SNMP_INTEGER,
    SNMP_INTEGER32,
    SNMP_INTEGER64, /* not an SNMP type, used by lexer for large values */
    SNMP_UNSIGNED32,
    SNMP_COUNTER,
    SNMP_COUNTER32,
    SNMP_COUNTER64,
    SNMP_GAUGE,
    SNMP_GAUGE32,
    SNMP_TIME_TICKS,
    SNMP_OCTET_STRING,
    SNMP_OPAQUE_DATA,
    SNMP_IP_ADDR,
    SNMP_PHYS_ADDR,
    SNMP_NETWORK_ADDR,
    SNMP_NAMED_TYPE,
    SNMP_SEQUENCE_IDENTIFIER,
    SNMP_SEQUENCE,
    SNMP_CHOICE,
    SNMP_TEXTUAL_CONVENTION,
    SNMP_MACRO_DEFINITION,
    SNMP_MODULE_COMPLIANCE,
    SNMP_LAST,
} SnmpBaseType;

typedef enum {
    OBJECT_RESOLVED=1,
    OBJECT_NOT_RESOLVED=2,
    OBJECT_NOT_FOUND=3
} ResolutionStatus;


typedef struct list_reference_struct 
{
    ubi_dlNode    node; 
	void *        data;
} ListReferenceType;

typedef struct type_tag_struct
{
    ubi_dlNode    node; /* For explicit tags on list */
    TagClassType  tagClass;
	char *className; /* For context specific tags this is the context name */
	long tagVal;
} SnmpTagType;

/* 
 * These are the entries which are parsed from an assigned identifier 
 * they will be massaged into an object identifier value in the post
 * parse processing
 */

typedef struct oid_struct
{
    ubi_dlNode    node;
    TruthValue resolved;
    char * nodeName;
    long int  oidVal;
} OID;

typedef struct snmp_keyword_struct
{
    char * name;
    ubi_dlListPtr bindings;
    char * value;
}
SnmpKeywordType;

typedef struct snmp_comment_struct
{
    ubi_dlNode    node;
    SnmpBaseType type;
    LineNumber lineNo;
    union {
        char * comment;
        SnmpKeywordType keyword;
    } data;
}
SnmpCommentType;

typedef struct snmp_number_struct
{
    SnmpBaseType type;
    union {
        long int intVal;
        QWORD qwVal;
    } data;
}
SnmpNumberType;

typedef enum tag_strategy_type
{
    TAG_STRATEGY_NONE = 0,
    TAG_STRATEGY_UNKNOWN = 1,
    TAG_STRATEGY_AUTOMATIC = 2,
    TAG_STRATEGY_LAST = 3,
} TagStrategyType;

typedef struct snmp_module_struct
{
    char *name;
	ValueType type;
    ubi_dlListPtr oidListPtr;
    TagStrategyType tagStrategy;
    LineNumber lineNo;
}
SnmpModuleType;

typedef struct snmp_module_capabilities_struct
{
    SnmpModuleType *module;
    ubi_dlListPtr supported_groups;
    ubi_dlListPtr variations;
    LineNumber lineNo;
}
SnmpModuleCapabilitiesType;

typedef struct snmp_import_module_struct
{
    ubi_dlNode node;
    SnmpModuleType module;
    ubi_dlListPtr imports;
    LineNumber lineNo;
}
SnmpImportModuleType;

typedef enum
{
    SNMP_ACCESS_VALUE_NULL = -1,
    SNMP_READ_ONLY = 0,
    SNMP_READ_WRITE = 1,
    SNMP_READ_CREATE = 2,
    SNMP_NOT_ACCESSIBLE = 3,
    SNMP_ACCESSIBLE_FOR_NOTIFY = 4
} SnmpAccess;

typedef enum
{
    SNMP_MANDATORY = 0,
    SNMP_OPTIONAL = 1,
    SNMP_OBSOLETE = 2,
    SNMP_DEPRECATED = 3,
    SNMP_CURRENT = 4
} SnmpStatus;

typedef struct snmp_range_constraint_struct
{
    long lowerEndValue;
    long upperEndValue;
}
SnmpRangeConstraintType;

typedef struct snmp_double_range_constraint_struct
{
    INT64 lowerEndValue;
    INT64 upperEndValue;
}
Snmp64BitRangeConstraintType;

typedef enum
{
    SNMP_VALUE_CONSTRAINT = 1, /* A single Integer Value */
    SNMP_RANGE_CONSTRAINT = 2, /* A list of legal ranges */
	/*
	 * A list of ranges, the upper end is the required size
     */
    SNMP_SIZE_CONSTRAINT = 3
} SnmpConstraintType;

typedef enum
{
    SNMP_GROUP_TYPE_UNKNOWN,
    SNMP_GROUP_TYPE_MANDATORY,
    SNMP_GROUP_TYPE_COMPLIANCE,
    SNMP_GROUP_TYPE_OBJECT,
    SNMP_GROUP_TYPE_NOTIFICATION,
    SNMP_GROUP_TYPE_LAST
} SnmpGroupEntryType;

typedef struct snmp_group_struct
{
    char *name;
    char *module; /* module name for this compliance structure */
    SnmpGroupEntryType type;
    ubi_dlListPtr objects; /* objects or mandatory groups */
    SnmpStatus status;
    char  *description;
    char  *reference;
    TruthValue resolved;
    TruthValue supported; /* Indicates Agent Compliance support for group */
    ubi_dlListPtr oidListPtr;
}
SnmpGroupType;

typedef struct snmp_value_constraint_struct
{
    LineNumber lineNo; /* INTEGER */
    SnmpConstraintType type;
    union
    {
        ubi_dlListPtr  valueList;
        ubi_dlListPtr  rangeList;
        ubi_dlListPtr  namedValueList; 
        int integer;
        SnmpRangeConstraintType valueRange; 
        Snmp64BitRangeConstraintType bigValueRange; 
    } a;
}
SnmpValueConstraintType;

typedef enum
{
    SNMP_HEX_STRING,
    SNMP_ASCII_STRING,
    SNMP_BINARY_STRING
} SnmpOctetStringDataType;

typedef struct snmp_octet_string_struct
{
    SnmpOctetStringDataType type;
    int len;
    char *text;
}
SnmpOctetStringType;

/* 
 * These items are kept on lists for Sequence assignments and CHOICE
 * objects
 */
typedef struct snmp_sequence_item_struct
{
    ubi_dlNode node;
    char *name;
    TruthValue resolved;
	struct snmp_type_struct *type;
    LineNumber lineNo;
	/* 
	 * The name and type data may be on different lines. The "Type"
	 * parser should pick up the line number for the type specification
	 * this line number is for the name. Comments and/or keywords
	 * would also have their own line numbers
	 */

}
SnmpSequenceItemType;

typedef struct snmp_sequence_object_struct /* SEQUENCE */
{
    char *name;
    ubi_dlListPtr sequence;
}
SnmpSequenceType;

typedef struct snmp_module_identity_struct
{
    char * name;
    char * last_update;
    char * organization;
    char * contact_info;
    char * description;
    char * references;
    ubi_dlListPtr  revisions;
    ubi_dlListPtr  capabilities; /* agent capabilities information */
    LineNumber lineNo;
    ubi_dlListPtr oidListPtr;
}
SnmpModuleIdentityType;

typedef enum {
    COMPLIANCE_ITEM_NONE=0,
    COMPLIANCE_ITEM_GROUP=1,
    COMPLIANCE_ITEM_OBJECT=2,
    COMPLIANCE_ITEM_LAST=3
} SnmpComplianceItemType;

typedef struct snmp_object_compliance_struct
{
    char *name;
	SnmpComplianceItemType type;
    struct snmp_type_struct *syntax;
    struct snmp_type_struct *write_syntax;
    SnmpStatus status;
    char *description;
    TruthValue resolved;
    TruthValue supported;
}
SnmpObjectComplianceType;

/* 
 * These items are kept on the imports list of an SnmpImportModuleType
 * which is itself kept on a list in the parse tree
 */
typedef struct snmp_symbol_struct
{
    ubi_dlNode node;
    TruthValue resolved;
    char *name;
    LineNumber lineNo;
}
SnmpSymbolType;

typedef struct snmp_module_compliance_object
{
    SnmpModuleType *module;
    ubi_dlListPtr mandatory_groups;
    ubi_dlListPtr compliance_items;
}
SnmpModuleComplianceObject;

typedef struct snmp_module_compliance_type
{
    char *name;
    SnmpStatus status;
    char *description;
    char *reference;
    ubi_dlListPtr compliance_objects;
    ubi_dlListPtr oidListPtr;
}
SnmpModuleComplianceType;

typedef struct snmp_type_struct
{
    char *name; /* the text name of this type */
    LineNumber lineNo;
    SnmpBaseType type; /* the base type */
    TruthValue resolved;
    TruthValue implied;
    SnmpTagType implicit; /* implicit tag value */
    ubi_dlListPtr tags; /* list of explicit tag values */
    ubi_dlListPtr capabilities; /* list of agent capabilities modifiers */
    union {
        struct snmp_textual_convention_struct *tcValue;
        SnmpModuleComplianceType *moduleCompliance;
        SnmpValueConstraintType *values; /* Constraints for this type */
        SnmpSequenceType *sequence; /* Sequence Elements for this type */
        unsigned long intVal; /* Named Number Values */
        ubi_dlListPtr macro_tokens; /* Tokens which make up the macro body */
		SnmpSymbolType symbols;
    } typeData;
}
Type;

typedef struct snmp_textual_convention_struct
{
    char *name;
    SnmpStatus status;
    char  *displayHint;
    char  *description;
    char * reference;
    Type *syntax;
}
SnmpTextualConventionType;


typedef struct snmp_defval_struct
{
    ValueType type;
    union {
        unsigned long intVal;
        TruthValue truthVal;
        SnmpOctetStringType strVal; /* string value from mib */
        ubi_dlListPtr oidListPtr;
		struct named_number_struct
        {
		    char *name;
			unsigned long intVal;
        } namedNumber;
    } defVal;
}
SnmpDefValType;

typedef struct snmp_object_struct
{
    char *name;
    SnmpAccess access;
    Type *syntax;
    SnmpStatus status;
    char *units;
    char *description;
    char *augments;
    char *reference;
    ubi_dlListPtr index;
    ubi_dlListPtr agentInfo; /* agent variations for object */
    SnmpDefValType *defVal;
    LineNumber lineNo;
    TruthValue hasGroup;
    TruthValue resolved;
    ubi_dlListPtr oidListPtr;
}
SnmpObjectType;


typedef struct snmp_agent_capabilities
{
    char *name;
    char *product_release;
    SnmpStatus status;
    char *description;
    char * reference;
    ubi_dlListPtr module_capabilities;
    ubi_dlListPtr oidListPtr;
}
SnmpAgentCapabilitiesType;

typedef struct snmp_notification_struct
{
    ubi_dlNode node;
    char *name;
    ubi_dlListPtr objects; /* trap data objects */
    SnmpStatus status;
    char *description;
    char * reference;
    char * enterprise;
    ubi_dlListPtr oidListPtr;
}
SnmpNotificationType;

typedef struct value_struct
{
    char *name;
    LineNumber lineNo;
    ValueType valueType;
    ubi_dlListPtr capabilities; /* list of agent capabilities modifiers */
    union {
        unsigned long intVal;
        unsigned long int32Val;
        TruthValue truthVal;
        SnmpModuleIdentityType *moduleIdentityValue;
		SnmpModuleType *moduleValue;
        ubi_dlListPtr importListPtr;
        SnmpOctetStringType strVal;
        SnmpRangeConstraintType rangeValue;
        ubi_dlListPtr oidListPtr;
        SnmpObjectType *object;
        SnmpGroupType *group;
        SnmpCommentType *keywordValue;
        SnmpAgentCapabilitiesType *agentCapabilitiesValue;
        SnmpNotificationType *notificationInfo;
    } value;
}
Value;

typedef struct snmp_revision_struct
{
    ubi_dlNode node;
    char *revision;
    char *description;
}
SnmpRevisionInfoType;


typedef enum
{
    DATATYPE_NONE,
    DATATYPE_TYPE,
    DATATYPE_VALUE,
    DATATYPE_IMPORT,
    DATATYPE_LAST
} TypeOrValueType;

typedef struct snmp_type_or_value_struct
{
    ubi_dlNode    node;
    TypeOrValueType dataType;
    TruthValue hasKeyword; /* one or more keywords bound to this object */
    union
    {
        Type *type;
        Value *value;
    } data;
}
TypeOrValue;

typedef struct snmp_parse_tree
{
    ubi_dlNode    node;
    SnmpModuleType *name;
    SnmpModuleIdentityType *identity;
    SnmpModuleComplianceType *compliance;
    ubi_dlListPtr imports; /* post-parse: imported objects in this module */
    ubi_dlListPtr tables; /* post-parse: tables defined in this module */
    ubi_dlListPtr objects; /* post-parse: that which isn't something else */
    ubi_dlListPtr types; /* post-parse: types defined in this module */
    ubi_dlListPtr groups; /* post-parse: groups defined in this module */
    ubi_dlListPtr notifications; /* post-parse: traps defined in this module */
    ubi_dlListPtr capabilities;  /* post-parse: agent capabilities in this module */
}
SnmpParseTree;

typedef struct snmp_table_struct
{
   ubi_dlNode node;
   Type *table; /* Pointer to the table definition */
   Type *row; /* Pointer to the row definition */
   Type *sequence; /* Row Elements */
} SnmpTableType;

typedef struct snmp_variation
{
    ubi_dlNode node;
    char *object_name;
    Type *syntax;
    Type *write_syntax;
    SnmpAccess access;
    ubi_dlListPtr creation;
    SnmpDefValType *defval;
}
SnmpVariationType;


#ifdef __cplusplus
extern "C"
{
#endif

void *mpalloc(int size);
SnmpParseTree *mpParseMib(char *filename);
TypeOrValueType create_builtin_type(char *name, TypeOrValue **item);
SnmpParseTree * find_module_by_name(char *mibname);
TypeOrValueType find_list_object_by_name(ubi_dlListPtr itemList,
                                         char *name,
                                         TypeOrValue **item);
TypeOrValueType find_mib_object_by_name(SnmpParseTree *mibTree,
                                        char *name,
                                        TypeOrValue **item);
TypeOrValueType find_named_type(char *name, TypeOrValue **item);
ResolutionStatus find_import_object_by_name(ubi_dlListPtr importList,
                                            char *name,
                                            LineNumber *lineNo,
                                            SnmpParseTree **ppTree);

void resolve_imports( SnmpParseTree *parseTree );
OID * oid_value(TypeOrValue *mibItem,ubi_dlList **oidListPtr);
TypeOrValueType resolve_textual_convention( TypeOrValue *mibItem);
void resolve_module_identity( SnmpParseTree *parseTree );
void resolve_oid_values(SnmpParseTree *mibTree);
void resolve_types_and_groups(SnmpParseTree *mibTree);
void resolve_agent_capabilities(SnmpParseTree *mibTree);
void resolve_table_objects(SnmpParseTree *mibTree);

#ifdef __cplusplus
}
#endif

#define MT(type)   (type *)mpalloc (sizeof (type))


#endif /* conditional include of mibparse.h */

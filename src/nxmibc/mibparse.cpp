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

#include "mibparse.h"
#include <nms_util.h>
#include "nxmibc.h"

ubi_dlList mibList;

extern int mpdebug;

#define RESOLVED_STR(x) ( x == TRUE ? "Resolved":"Unresolved")

static char *object_name(TypeOrValue *object)
{ 
   return object == NULL ? NULL :
        object->dataType == DATATYPE_TYPE ? object->data.type->name :
        object->dataType == DATATYPE_VALUE ? object->data.value->name : NULL;
}


/*
 * This function is responsible for fabricating builtin type definitions
 * found in the snmp V2 specs.
 * If the request item is not one of the known types then we return
 * DATATYPE_NONE and set the item value to NULL
 *
 * This function is also used by the parser to generate the data values
 * when a builtin type is actually defined. If someone feeds us the actual
 * SNMPV2-MIB we won't barf but its not required. An important note
 * is that the mib definitions will override the built in values. This
 * it is possible to hack up the standard definitions and the parser will
 * return what are effectively enterprise versions of the standard
 * variables.
 */
static TypeOrValueType create_builtin_type(char *name, TypeOrValue **item)
{
    TypeOrValueType itemType=DATATYPE_NONE;
    TypeOrValue *mibItem=NULL;

    if( !strcmp( name, "INTEGER" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_INTEGER;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_UNIVERSAL;
        mibItem->data.type->implicit.className = strdup("UNIVERSAL");
        mibItem->data.type->implicit.tagVal = 2;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( ( !strcmp( name, "org" ) ) ||
             ( !strcmp( name, "dod" ) ) ||
             ( !strcmp( name, "internet" ) ) ||
             ( !strcmp( name, "directory" ) ) ||
             ( !strcmp( name, "mgmt" ) ) ||
             ( !strcmp( name, "mib-2" ) ) || /* from the v1 spec */
             ( !strcmp( name, "transmission" ) ) ||
             ( !strcmp( name, "experimental" ) ) ||
             ( !strcmp( name, "private" ) ) ||
             ( !strcmp( name, "enterprises" ) ) ||
             ( !strcmp( name, "security" ) ) ||
             ( !strcmp( name, "snmpV2" ) ) ||
             ( !strcmp( name, "snmpDomains" ) ) ||
             ( !strcmp( name, "snmpProxys" ) ) ||
             ( !strcmp( name, "snmpModules" ) ) )
    {
        OID *oidPtr = MT ( OID );
        char *parentName = NULL;
        int parentValue = 0;

        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_VALUE;
        mibItem->hasKeyword = FALSE;
        mibItem->data.value = MT( Value );
        mibItem->data.value->name = strdup(name);
        mibItem->data.value->valueType = VALUETYPE_OID;
        mibItem->data.value->value.oidListPtr = (ubi_dlList *)mpalloc(sizeof(ubi_dlList));

        oidPtr->nodeName = strdup( name );
        if ( !strcmp( name, "iso" ) ) 
        {
            parentName="zerodotzero";
            parentValue=0;
            oidPtr->oidVal = 1;
        }
        else if (!stricmp( name, "zerodotzero" ) ) 
        {
            parentName="ccitt";
            parentValue=0;
            oidPtr->oidVal = 0;
        }
        else if ( !strcmp( name, "ccitt" ) ) 
        {
            parentName="zerodotzero";
            parentValue=0;
            oidPtr->oidVal = 0;
        }
        else if ( !strcmp( name, "org" ) ) 
        {
            parentName="iso";
            parentValue=1;
            oidPtr->oidVal = 1;
        }
        else if ( !strcmp( name, "dod" ) ) 
        {
            parentName="org";
            parentValue=3;
            oidPtr->oidVal = 6;
        }
        else if ( !strcmp( name, "internet" ) ) 
        {
            parentName="dod";
            parentValue=1;
            oidPtr->oidVal = 1;
        }
        else if ( !strcmp( name, "directory" ) ) 
        {
            parentName="internet";
            parentValue=1;
            oidPtr->oidVal = 1;
        }
        else if ( !strcmp( name, "mgmt" ) ) 
        {
            parentName="internet";
            parentValue=1;
            oidPtr->oidVal = 2;
        }
        else if ( !strcmp( name, "mib-2" ) ) 
        {
            parentName="transmission";
            parentValue=2;
            oidPtr->oidVal = 1;
        } /* v1 */
        else if ( !strcmp( name, "transmission" ) ) 
        {
            parentName="mib-2";
            parentValue=1;
            oidPtr->oidVal = 10;
        }
        else if ( !strcmp( name, "experimental" ) ) 
        {
            parentName="internet";
            parentValue=1;
            oidPtr->oidVal = 3;
        }
        else if ( !strcmp( name, "private" ) ) 
        {
            parentName="internet";
            parentValue=1;
            oidPtr->oidVal = 4;
        }
        else if ( !strcmp( name, "enterprises" ) ) 
        {
            parentName="private";
            parentValue=1;
            oidPtr->oidVal = 1;
        }
        else if ( !strcmp( name, "security" ) ) 
        {
            parentName="internet";
            parentValue=1;
            oidPtr->oidVal = 5;
        }
        else if ( !strcmp( name, "snmpV2" ) ) 
        {
            parentName="internet";
            parentValue=1;
            oidPtr->oidVal = 6;
        }
        else if ( !strcmp( name, "snmpDomains" ) ) 
        {
            parentName="snmpV2";
            parentValue=6;
            oidPtr->oidVal = 1;
        }
        else if ( !strcmp( name, "snmpProxys" ) ) 
        {
            parentName="snmpV2";
            parentValue=6;
            oidPtr->oidVal = 2;
        }
        else if ( !strcmp( name, "snmpModules" ) ) 
        {
            parentName="snmpV2";
            parentValue=6;
            oidPtr->oidVal = 3;
        }

        /*
         * Add the OID value since we already allocated it
         */
        oidPtr->oidVal = oidPtr->oidVal;
        oidPtr->resolved = TRUE;
        ubi_dlAddTail(mibItem->data.value->value.oidListPtr,
                       (ubi_dlNodePtr)oidPtr);
        /*
         * Now add the parent OID value at the head of the list
         */
        oidPtr = MT ( OID );
        oidPtr->nodeName = strdup(parentName);
        oidPtr->oidVal = parentValue;
        oidPtr->resolved = TRUE;
        ubi_dlAddHead(mibItem->data.value->value.oidListPtr,
                       (ubi_dlNodePtr)oidPtr);
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "Integer32" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_INTEGER32;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_UNIVERSAL;
        mibItem->data.type->implicit.className = strdup("UNIVERSAL");
        mibItem->data.type->implicit.tagVal = 2;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "Unsigned32" ) ||
             !strcmp( name, "UInteger32" ) )
    {
        /* 
         * The UInteger32 definition was imported (allegedly) from
         * the version of SNMPv2-PARTY-MIB included with my
         * ucd distribution. Added here instead of fixed in the
         * mib because this might be a common value and its not
         * actually a syntax error.
         */
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_UNSIGNED32;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_UNIVERSAL;
        mibItem->data.type->implicit.className = strdup("UNIVERSAL");
        mibItem->data.type->implicit.tagVal = 2;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "Counter" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_COUNTER;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "Counter32" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_COUNTER32;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_APPLICATION;
        mibItem->data.type->implicit.className = strdup("APPLICATION");
        mibItem->data.type->implicit.tagVal = 1;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "Counter64" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_COUNTER64;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_APPLICATION;
        mibItem->data.type->implicit.className = strdup("APPLICATION");
        mibItem->data.type->implicit.tagVal = 6;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "Gauge" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_GAUGE;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_APPLICATION;
        mibItem->data.type->implicit.className = strdup("APPLICATION");
        mibItem->data.type->implicit.tagVal = 1;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "Gauge32" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_GAUGE32;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_APPLICATION;
        mibItem->data.type->implicit.className = strdup("APPLICATION");
        mibItem->data.type->implicit.tagVal = 2;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "TimeTicks" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_TIME_TICKS;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_APPLICATION;
        mibItem->data.type->implicit.className = strdup("APPLICATION");
        mibItem->data.type->implicit.tagVal = 3;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "BITS" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_BITS;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_UNIVERSAL;
        mibItem->data.type->implicit.className = strdup("UNIVERSAL");
        mibItem->data.type->implicit.tagVal = 3;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "IpAddress" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_IP_ADDR;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_UNIVERSAL;
        mibItem->data.type->implicit.className = strdup("UNIVERSAL");
        mibItem->data.type->implicit.tagVal = 3;
        mibItem->data.type->resolved = TRUE;
        mibItem->data.type->typeData.values = MT ( SnmpValueConstraintType );
        mibItem->data.type->typeData.values->type = SNMP_SIZE_CONSTRAINT;
        mibItem->data.type->typeData.values->a.valueRange.lowerEndValue  = 0;
        mibItem->data.type->typeData.values->a.valueRange.upperEndValue = 0x4;

        itemType=mibItem->dataType;
    }
    else if( !strcmp( name, "NetworkAddress" ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_NETWORK_ADDR;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->resolved = TRUE;
        mibItem->data.type->typeData.values = MT ( SnmpValueConstraintType );
        mibItem->data.type->typeData.values->type = SNMP_SIZE_CONSTRAINT;
        mibItem->data.type->typeData.values->a.valueRange.lowerEndValue  = 0;
        mibItem->data.type->typeData.values->a.valueRange.upperEndValue = 0x4;

        itemType=mibItem->dataType;
    }
    else if( strstr( name, "OCTET" ) != NULL )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_OCTET_STRING;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->implicit.tagClass = TAG_TYPE_UNIVERSAL;
        mibItem->data.type->implicit.className = strdup("UNIVERSAL");
        mibItem->data.type->implicit.tagVal = 4;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    else if( ( !strcmp( name, "MODULE-COMPLIANCE" ) ) ||
             ( !strcmp( name, "MODULE-IDENTITY" ) ) ||
             ( !strcmp( name, "OBJECT-TYPE" ) ) ||
             ( !strcmp( name, "OBJECT-GROUP" ) ) ||
             ( !strcmp( name, "OBJECT-IDENTITY" ) ) ||
             ( !strcmp( name, "TRAP-TYPE" ) ) || /* from the v1 spec */
             ( !strcmp( name, "NOTIFICATION-GROUP" ) ) ||
             ( !strcmp( name, "NOTIFICATION-TYPE" ) ) ||
             ( !strcmp( name, "TEXTUAL-CONVENTION" ) ) )
    {
        mibItem = MT (TypeOrValue);
        mibItem->dataType = DATATYPE_TYPE;
        mibItem->hasKeyword = FALSE;
        mibItem->data.type = MT( Type );
        mibItem->data.type->name = strdup(name);
        mibItem->data.type->type = SNMP_MACRO_DEFINITION;
        mibItem->data.type->lineNo = 0;
        mibItem->data.type->resolved = TRUE;
        itemType=mibItem->dataType;
    }
    if ( item != NULL )
    {
        *item=mibItem;
    }
    return(itemType);
}

/*
 * This function takes a module name and returns the parseTree for that
 * module
 */
static SnmpParseTree *find_module_by_name(char *mibname)
{
    SnmpParseTree *mibTree=NULL;

    for( mibTree=(SnmpParseTree *)ubi_dlFirst(&mibList);
         mibTree;
         mibTree=(SnmpParseTree *)ubi_dlNext(mibTree))
    {
        if ( !strcmp(mibTree->name->name,mibname ) )
                break; /* found it */
    }
    return(mibTree);
}

/*
 * This function takes an item name and an assignment list for a SNMP
 * module. It returns the TypeOrValue structure associated with the object
 * in the 'item' parameter. The return value is the datatype of the
 * associated object.
 *
 * If the requested object is not found it returns DATATYPE_NONE
 * and sets the value pointed to by to NULL.
 */
TypeOrValueType
find_list_object_by_name(ubi_dlListPtr itemList, char *name, TypeOrValue **item)
{
    TypeOrValue *mibItem=NULL;
    char *pszName;

    for( mibItem=(TypeOrValue *)ubi_dlFirst(itemList);
         mibItem;
         mibItem=(TypeOrValue *)ubi_dlNext(mibItem))
    {
        pszName = object_name(mibItem);
        if (pszName != NULL)
           if (!strcmp(pszName, name))
           {
               break;
           }
    }
    *item=mibItem;
    return(mibItem == NULL ? DATATYPE_NONE:mibItem->dataType);
}


/*
 * This function takes a mib Tree and a name an SNMP object.
 * It returns the TypeOrValue structure associated with the object
 * in the 'item' parameter. The return value is the datatype of the
 * associated object.
 *
 * If the requested object is not found it returns DATATYPE_NONE
 * and sets the value pointed to by to NULL.
 */

TypeOrValueType
find_mib_object_by_name(SnmpParseTree *mibTree, char *name, TypeOrValue **item)
{
    TypeOrValue *mibItem=NULL;
    ubi_dlList *itemList=NULL;
    TypeOrValueType itemType=DATATYPE_NONE;

    /*
     * This will create the standard data types from well known mibs 
     * if they are not otherwise found during parsing. Does not
     * generally include textual conventions so everyone should include
     * the current SNMPv2-TC specification.
	 *
     * If mibTree is null it indicates an embedded mib has been imported
	 * but the actual mib is not included in the parse tree. This is
	 * probably an error on the part of the user since strictly speaking
	 * we only know about SNMPv2-SMI and RFC1155-SMI. I am going to give
	 * them the leeway to do this and resolve any imports of well known
	 * types
	 *
	 * By default this will return the v2 definition for the types
     */
    if ( mibTree == NULL )
    {
        itemType=create_builtin_type(name,&mibItem);
    }
    else if ( mibTree->identity != NULL )
    {
        if( !strcmp(mibTree->identity->name,name) )
        {
            mibItem = MT (TypeOrValue);
            mibItem->dataType = DATATYPE_VALUE;
            mibItem->data.value = MT( Value );
            mibItem->data.value->name = mibTree->identity->name;
            mibItem->data.value->valueType = VALUETYPE_MODULE_IDENTITY;
            mibItem->data.value->value.moduleIdentityValue = mibTree->identity;
            mibItem->data.value->lineNo = mibTree->identity->lineNo;
            itemType = mibItem->dataType;

        }
    }
    if ( mibItem == NULL )
    {
        itemList=mibTree->objects;
        if ( itemList != NULL )
        {
            itemType=find_list_object_by_name(itemList,name,&mibItem);
        }
    }
    if ( mibItem == NULL )
    {
        itemList=mibTree->types;
        if ( itemList != NULL )
        {
            itemType=find_list_object_by_name(itemList,name,&mibItem);
        }
    }
    if ( mibItem == NULL )
    {
        itemList=mibTree->tables;
        if ( itemList != NULL )
        {
            itemType=find_list_object_by_name(itemList,name,&mibItem);
        }
    }
    if (mibItem == NULL)
    {
        itemList = mibTree->groups;
        if (itemList != NULL)
        {
            itemType = find_list_object_by_name(itemList, name, &mibItem);
        }
    }
    if ( mibItem == NULL )
    {
        itemList=mibTree->imports;
        if ( itemList != NULL )
        {
        ResolutionStatus itemStatus;
        LineNumber lineNo;

            itemStatus = find_import_object_by_name(itemList, name, &lineNo, NULL);
            switch( itemStatus )
            {
                case OBJECT_NOT_FOUND:
                    break;
                case OBJECT_RESOLVED:
                    /* in the case of resolved objects find the base object 
                     * and return the underlying mib object 
                     */
                    itemType=DATATYPE_IMPORT;
                    break;
                case OBJECT_NOT_RESOLVED:
                default:
                    break;
            }
        }
    }
    /*
     * This will create the standard data types from well known mibs 
     * if they are not otherwise found during parsing. Does not
     * generally include textual conventions so everyone should include
     * the current SNMPv2-TC specification.
	 *
	 * This code will be hit if a mib object has a SYNTAX clause
	 * of a well known type but the mib is not actually included in
	 * the parse tree. Again this is probably a user error but I'm
	 * unwilling to penalize them for it.
     */
    if ( ( itemType == DATATYPE_NONE ) &&
         ( ( strcmp(name,"SNMPv2-SMI") == 0 ) ||
           ( strcmp(name,"RFC1155-SMI") == 0 ) ||
           ( strcmp(name,"RFC-1215") == 0 ) ||
           ( strcmp(name,"RFC1212") == 0 ) || /* typo in "std" mib */
           ( strcmp(name,"RFC-1212") == 0 ) ) )
    {
        itemType=create_builtin_type(name,&mibItem);
    }

    if ( item != NULL )
    {
        *item=mibItem;
    }
    return(itemType);
}

/*
 * This function is responsible for resolving named types. It fabricates
 * the type entries for the known types. If the request item is not
 * one of the known types then we go through the mib list to identify
 * the named type. If we can't identify the named type then we check the
 * imports list to determine if the type is imported. If so then we presume
 * that the user has a left a mib off of the list and we create a bogus 
 * import item type entry for it.
 */
TypeOrValueType
find_named_type(char *name, TypeOrValue **item)
{
    TypeOrValue *mibItem=NULL;
    TypeOrValueType itemType=DATATYPE_NONE;
    SnmpParseTree *mibTree=NULL;
    ubi_dlList *itemList=NULL;

    if( name == NULL || strlen( name ) == 0  )
    {
        *item=mibItem;
        return(itemType);
    }
    else for( mibTree=(SnmpParseTree *)ubi_dlFirst(&mibList);
         mibTree;
         mibTree=(SnmpParseTree *)ubi_dlNext(mibTree))
    {
            itemType=find_mib_object_by_name(mibTree,name,&mibItem);
            if ( ( itemType == DATATYPE_TYPE ) || 
                 ( itemType == DATATYPE_VALUE ) ) 
            {
                /* 
                 * Here we have to keep looping until we find the actual
                 * type definition. Otherwise our ability to resolve this
                 * depends in a large part on the mib with the type
                 * definition appearing in the mib list ahead of the
                 * files which import it. This type of forward reference
                 * could be resolved in three ways, enforce strict ordering
                 * on the command line (sucks), dynamically order after the
                 * mibs are parsed but before we start resolution or
                 * something like this where will roll through until we
                 * find the actual definition
                 */
                break;
            }
    }
    if ( mibItem == NULL )
    {
        /*
         * if its not an object and its not an import it might be a
         * builtin type. If so this object will create it.
         */

        itemType=create_builtin_type(name,&mibItem);
    }
    *item=mibItem;
    return(itemType);
}

/*
 * This function takes an item name and an Import assignment list for a SNMP
 * module. It returns the resolution status associated with the object
 * in the imports list. This allows the caller to differentiate between
 * imported but unresolved (the import module is not in the master file
 * list ) and imported but undefined ( the imported object does not exist )
 */
ResolutionStatus
find_import_object_by_name(ubi_dlListPtr importList,
                           char *name,
                           LineNumber *lineNo,
                           SnmpParseTree **ppTree)
{
    SnmpImportModuleType *importModule=NULL;
    ResolutionStatus retval=OBJECT_NOT_FOUND;

    if (ppTree != NULL)
       *ppTree = NULL;

    if( ( importList != NULL ) && ( ubi_dlCount(importList) ) )
    {
        for( importModule=(SnmpImportModuleType *)ubi_dlFirst(importList);
             importModule;
             importModule=(SnmpImportModuleType *)ubi_dlNext(importModule))
        {
            ubi_dlListPtr itemList=importModule->imports;
            SnmpSymbolType *symbolItem=NULL;

            if( itemList == NULL || ubi_dlCount(itemList) == 0 )
            {
                /*
                 * This probably also indicates a mib error. The user
                 * has, for some reason, defined an import with no
                 * objects
                 */
                continue;
            }
            for( symbolItem=(SnmpSymbolType *)ubi_dlFirst(itemList);
                 symbolItem;
                 symbolItem=(SnmpSymbolType *)ubi_dlNext(symbolItem))
            {
                if( !strcmp(symbolItem->name,name) )
                {
                    if ( symbolItem->resolved == TRUE )
                    {
                        /* imported and the import is resolved */
                        retval = OBJECT_RESOLVED;
                        if (ppTree != NULL)
                           *ppTree = find_module_by_name(importModule->module.name);
                    }
                    else
                    {
                        /* imported but the import is not resolved */
                        retval = OBJECT_NOT_RESOLVED;
                    }
                    *lineNo=symbolItem->lineNo;
                    break;
                }
            }
            if( symbolItem != NULL )
            {
                break;
            }
        }
    }
   if ( retval == OBJECT_NOT_RESOLVED )
   {
      TypeOrValueType itemType = DATATYPE_NONE;
      TypeOrValue *mibItem = NULL;
        itemType=create_builtin_type(name,&mibItem);
        switch(itemType)
        {
            case DATATYPE_IMPORT:
                printf("Unexpected Import %s\n", name );
                break;
            case DATATYPE_TYPE:
            case DATATYPE_VALUE:
                /* imported and the import is resolved */
                retval = OBJECT_RESOLVED;
                free(mibItem);
                break;
            case DATATYPE_NONE:
            case DATATYPE_LAST:
            default:
                break;
        }
    }
    return(retval);
}

void
resolve_imports(SnmpParseTree *parseTree)
{
    ubi_dlList *importList=parseTree->imports;
    SnmpImportModuleType *importModule=NULL;
    SnmpSymbolType *importItem=NULL;
    SnmpParseTree *mibTree=NULL;
    TypeOrValue *mibItem=NULL;
    TypeOrValueType itemType;

    if( importList == NULL || ubi_dlCount(importList) == 0 )
    {
        return;
    }

    for( importModule=(SnmpImportModuleType *)ubi_dlFirst(importList);
         importModule;
         importModule=(SnmpImportModuleType *)ubi_dlNext(importModule))
    {
        TruthValue moduleResolved=TRUE;
        char *name=importModule->module.name;
        if ( (mibTree=find_module_by_name(name)) == NULL )
        {
            Error(ERR_UNRESOLVED_MODULE, parseTree->name->name, name);
            continue; 
        }
        else if ( mibTree == parseTree )
        {
            /* No point in searching ourselves for imports */
            continue; 
        }
        /*
         * For each imported item search the module assignment list
         * to find the item. If found mark it as resolved
         */
        for( importItem=(SnmpSymbolType *)ubi_dlFirst(importModule->imports);
             importItem;
             importItem=(SnmpSymbolType *)ubi_dlNext(importItem))
        {
        TruthValue itemResolved=FALSE;
            itemType=find_mib_object_by_name(mibTree,importItem->name,&mibItem);
            if( itemType == DATATYPE_IMPORT )
            {
                /*
                 * This indicate that mib A imports Type C from mib B.
                 * However mib B also imports Type C from mib D where
                 * it is actually defined. That's actually an SNMP
                 * logical error, I don't think the parser can surmount 
                 * it
                 */
                printf("\nIllegal Import of Imported Type %s From %s\n",
                       importItem->name, name );
                continue;
            }
            switch(itemType)
            {
                case DATATYPE_TYPE:
                case DATATYPE_VALUE:
                    importItem->resolved=TRUE;
                    itemResolved=TRUE;
                    break;
                case DATATYPE_NONE:
                case DATATYPE_LAST:
                    /*
                     * if its not an object and its not an import it might be a
                     * builtin type. If so this object will create it.
                     */
                    itemType=create_builtin_type(importItem->name,NULL);
                    switch(itemType)
                    {
                        case DATATYPE_TYPE:
                        case DATATYPE_VALUE:
                            importItem->resolved=TRUE;
                            itemResolved=TRUE;
                            break;
                        case DATATYPE_NONE:
                        case DATATYPE_LAST:
                        case DATATYPE_IMPORT:
                        default:
                            moduleResolved=FALSE;
                            break;
                    }
                    break;
                case DATATYPE_IMPORT:
                    /* can't get here, itemType filtered before switch */
                    break;
                default:
                    importItem->resolved=FALSE;
                    moduleResolved=FALSE;
                    itemResolved=TRUE;
                    break;
            }
            /* This is a stub switch for post resolution type processing */
            switch(itemType)
            {
                case DATATYPE_IMPORT:
                    break;
                case DATATYPE_TYPE:
                case DATATYPE_VALUE:
                    break;
                case DATATYPE_NONE:
                case DATATYPE_LAST:
                default:
                    break;
            }
        }
    }
}

/*
 * This function is used to get the oid value for an object. It is used 
 * primary to resolve the parent oid value in an oid list but may have
 * many other interesting and useful porpoises.
 *
 * It returns the last item in the oid chain (if any) for the passed in
 * object. If the objects self reference has not been resolved the
 * object name is duplicated and the oid marked as resolved.
 */
OID *oid_value(TypeOrValue *mibItem,ubi_dlList **oidListPtr)
{
    TypeOrValueType itemType=mibItem->dataType;
    OID *oidPtr=NULL;
    ubi_dlList *retListPtr=NULL;
    int length=0;

    switch(itemType)
    {
        case DATATYPE_IMPORT:
            printf("oid_value: Illegal data type DATATYPE_IMPORT\n");
            break;
        case DATATYPE_NONE:
            break;
        case DATATYPE_TYPE:
            switch( mibItem->data.type->type )
            {
                case SNMP_NAMED_TYPE:
                case SNMP_IMPORT_ITEM:
                case SNMP_TEXTUAL_CONVENTION:
                /* Does NOT have OID */
                    break;
                case SNMP_UNKNOWN:
                /* 
                 * These are named types. May require resolution
                 * of the base type to determine if appropriate
                 * to add to oid table 
                 */
                    break;
                case SNMP_INTEGER:
                case SNMP_INTEGER32:
                case SNMP_UNSIGNED32:
                case SNMP_COUNTER:
                case SNMP_COUNTER32:
                case SNMP_COUNTER64:
                case SNMP_GAUGE:
                case SNMP_GAUGE32:
                case SNMP_OCTET_STRING:
                case SNMP_SEQUENCE_IDENTIFIER:
                case SNMP_SEQUENCE:
                    break;
                case SNMP_OID:
                /* Has OID, this is used for SYNTAX type OBJECT IDENTIFIER */
                    retListPtr=mibItem->data.type->typeData.moduleCompliance->oidListPtr;
                    break;
                case SNMP_MODULE_COMPLIANCE:
                /* Has OID */
                    retListPtr=mibItem->data.type->typeData.moduleCompliance->oidListPtr;
                    break;
                default:
                    break;
            }
            break;
        case DATATYPE_VALUE:
            switch( mibItem->data.value->valueType)
            {
                case VALUETYPE_UNKNOWN:
                case VALUETYPE_EMPTY:
                case VALUETYPE_IMPORT_ITEM:
                case VALUETYPE_COMMENT:
                case VALUETYPE_INTEGER:
                case VALUETYPE_INTEGER32:
                case VALUETYPE_BOOLEAN:
                case VALUETYPE_OCTET_STRING:
                case VALUETYPE_RANGE_VALUE:
                case VALUETYPE_SIZE_VALUE:
                case VALUETYPE_NULL:
                /* Does Not Have OID */
                    break;
                case VALUETYPE_MODULE_IDENTITY:
                /* Has OID */
                    retListPtr=mibItem->data.value->value.moduleIdentityValue->oidListPtr;
                    break;
                case VALUETYPE_OID:
                    retListPtr=mibItem->data.value->value.oidListPtr;
                    break;
                case VALUETYPE_OBJECT_TYPE:
                    retListPtr=mibItem->data.value->value.object->oidListPtr;
                    break;
                case VALUETYPE_CAPABILITIES:
                    break;
                case VALUETYPE_NOTIFICATION:
                /* Has OID */
                    retListPtr=mibItem->data.value->value.notificationInfo->oidListPtr;
                    break;
                case VALUETYPE_NAMED_NUMBER:
                /* Has OID */
                    oidPtr = MT ( OID );
                    oidPtr->nodeName = strdup( mibItem->data.value->name );
                    oidPtr->oidVal = mibItem->data.value->value.intVal;
                    /* No List, just oid value */
                    retListPtr=NULL;
                    break;
                case VALUETYPE_NAMED_VALUE:
                /* Has OID */
                    oidPtr = MT ( OID );
                    length=mibItem->data.value->value.strVal.len;
                    oidPtr->nodeName = (char *)malloc(length);
                    sprintf( oidPtr->nodeName,"%.*s",
                             mibItem->data.value->value.strVal.len,
                             mibItem->data.value->value.strVal.text );
                    oidPtr->oidVal = 0;
                    retListPtr=NULL;
                    break;
                case VALUETYPE_GROUP_VALUE:
                /* Has OID */
                    retListPtr=mibItem->data.value->value.group->oidListPtr;
                    break;
                default:
                    break;
            }
            break;
        case DATATYPE_LAST:
        default:
            break;
    }
    if( retListPtr != NULL )
    {
        oidPtr = ( OID * )ubi_dlLast(retListPtr);
        /*
         * If the list is not NULL it must have at least one entry
         */
        if ( oidPtr->nodeName == NULL )
        {
            /* Resolve the name if not resolved */
            oidPtr->nodeName = strdup(object_name(mibItem));
            oidPtr->resolved = TRUE;

        }
        if( oidListPtr != NULL )
        {
            *oidListPtr = retListPtr;
        }
    }
    return(oidPtr);
}


/*
 * This function is responsible for resolving Textual Convention types.
 * It returns the base type value of the textual convention
 */
TypeOrValueType
resolve_textual_convention( TypeOrValue *mibItem)
{
    TypeOrValueType itemType=DATATYPE_NONE;
    TypeOrValue *typeItem=mibItem;
    Type *syntax=NULL;
    TruthValue done=FALSE;
    char *name;

    syntax = mibItem->data.type->typeData.tcValue->syntax;
    itemType = (TypeOrValueType)syntax->type;
    name = syntax->name;

    switch(itemType)
    {

        case SNMP_IMPORT_ITEM:
        case SNMP_OID:
        case SNMP_BITS:
        case SNMP_INTEGER:
        case SNMP_INTEGER32:
        case SNMP_UNSIGNED32:
        case SNMP_COUNTER:
        case SNMP_COUNTER32:
        case SNMP_COUNTER64:
        case SNMP_GAUGE:
        case SNMP_GAUGE32:
        case SNMP_TIME_TICKS:
        case SNMP_OCTET_STRING:
        case SNMP_OPAQUE_DATA:
        case SNMP_IP_ADDR:
        case SNMP_PHYS_ADDR:
        case SNMP_NETWORK_ADDR:
            mibItem->data.type->resolved = TRUE;
            done=TRUE;
            break;
        case SNMP_NAMED_TYPE:
           mibItem->data.type->resolved = FALSE;
           done=FALSE;
           break;
        case SNMP_SEQUENCE_IDENTIFIER:
        case SNMP_SEQUENCE:
        case SNMP_CHOICE:
        case SNMP_TEXTUAL_CONVENTION:
           done=FALSE;
           break;
        case SNMP_MACRO_DEFINITION:
        case SNMP_MODULE_COMPLIANCE:
        case SNMP_UNKNOWN:
        case SNMP_INTEGER64:
        case SNMP_LAST:
            done=TRUE;
           break;
        default:
            done=TRUE;
            break;
    }

    /*
     * I believe the following loop will drill down through multiple layers
     * of textual convention refinements to identify the base type.
     *
     * The arguments to this function could be modified to return anything
     * from the type value for the base type to a linked list of all
     * relevent type definitions.
     */
    while( !done )
    {
        switch(itemType)
        {
            case SNMP_NAMED_TYPE:
               itemType=find_named_type(name,&typeItem);
               if ( itemType != DATATYPE_TYPE )
               {
                   /* Sorry, all textual convention values must be a type */
                    printf(" Syntax Type %s Unresolved\n",name);
                    done=TRUE;
                    continue;
               }
               else if ( typeItem->data.type->type != SNMP_NAMED_TYPE )
               {
                   /* we have drilled all the way down to a base type */
                   mibItem->data.type->resolved = TRUE;
                   done=TRUE;
               }
               else
               {
                   /* Okay we found another named type, fetch it */
                   name=typeItem->data.type->name;
               }
               break;
            case SNMP_TEXTUAL_CONVENTION:
               itemType=find_named_type(name,&typeItem);
               if ( itemType != DATATYPE_TYPE )
               {
                   /* Sorry, all textual convention values must be a type */
                    printf(" Syntax Type %s Unresolved\n",name);
                    done=TRUE;
                    continue;
               }
               else if ( typeItem->data.type->type != SNMP_NAMED_TYPE )
               {
                   /* we have drilled all the way down to a base type */
                   mibItem->data.type->resolved = TRUE;
                   done=TRUE;
               }
               else switch ( typeItem->data.type->type )
               {
                   case SNMP_NAMED_TYPE:
                       /* Okay we found another named type, fetch it */
                       name=typeItem->data.type->name;
                       break;
                   case SNMP_TEXTUAL_CONVENTION:
                       /* Okay we found another named type, fetch it */
                       syntax=typeItem->data.type->typeData.tcValue->syntax;
                       name = syntax->name;
                       break;
                   default:
                       done=TRUE;
                       break;
               }
               break;
            case SNMP_IMPORT_ITEM:
            case SNMP_OID:
            case SNMP_BITS:
            case SNMP_INTEGER:
            case SNMP_INTEGER32:
            case SNMP_UNSIGNED32:
            case SNMP_COUNTER:
            case SNMP_COUNTER32:
            case SNMP_COUNTER64:
            case SNMP_GAUGE:
            case SNMP_GAUGE32:
            case SNMP_TIME_TICKS:
            case SNMP_OCTET_STRING:
            case SNMP_OPAQUE_DATA:
            case SNMP_IP_ADDR:
            case SNMP_PHYS_ADDR:
            case SNMP_NETWORK_ADDR:
            case SNMP_SEQUENCE_IDENTIFIER:
            case SNMP_SEQUENCE:
            case SNMP_CHOICE:
            case SNMP_MACRO_DEFINITION:
            case SNMP_MODULE_COMPLIANCE:
                done=TRUE;
                break;
            default:
                printf("Unresolvable Base Type %s",name);
                done=TRUE;
                break;
        }
    }
    return(itemType);
}

void
resolve_module_identity(SnmpParseTree *parseTree)
{
    SnmpModuleIdentityType *moduleIdentity=parseTree->identity;
    OID *oidPtr=NULL;
    OID *parentOidPtr=NULL;
    ubi_dlList *itemList=parseTree->objects;
    TypeOrValue *mibItem=NULL;
    char *parentName=NULL;

    if( ( moduleIdentity == NULL ) && ( itemList != NULL ) )
    {
    TypeOrValueType itemType=DATATYPE_NONE;
        /* 
         * Module identity value may not yet be assigned 
         * Search objects for module identity assignment 
         * I don't search the imports list because even
         * if it is legal it would be moronic to import
         * a module identity.
         */
        for( mibItem=(TypeOrValue *)ubi_dlFirst(itemList);
             mibItem;
             mibItem=(TypeOrValue *)ubi_dlNext(mibItem))
        {
            itemType = (TypeOrValueType)mibItem->data.value->valueType;
            if ( ( mibItem->dataType == DATATYPE_VALUE ) &&
                 ( itemType == VALUETYPE_MODULE_IDENTITY ) )
            {
                break; /* found it */
            }

        }
        if (mibItem == NULL)
        {
            moduleIdentity = NULL;
        }
        else
        {
            /* assign the identity module */
            moduleIdentity=mibItem->data.value->value.moduleIdentityValue;
            parseTree->identity=moduleIdentity;
            /* and remove it from the assignments list */
            ubi_dlRemove(itemList,(ubi_dlNodePtr)mibItem);
        }
    }
    if( moduleIdentity != NULL )
    {
    ubi_dlListPtr oidListPtr=moduleIdentity->oidListPtr;
    int entryCount =  ( oidListPtr == NULL ? 0: ubi_dlCount(oidListPtr) );

        /*
         * For the last named value on the list we will check to 
         * see if the name is assigned, if not we assign the object
         * name.
         * For the preceeding values we check for the name. If the name
         * is found we use that the look up the object oid value and
         * resolve this object chain.
         *
         * It may be possible, in some cases to have a parent oid value
         * where the parent name is not known. An example would be the
         * assignment foobar := { 6 5 }. I would sincerely hope that all
         * of these will turn out to be value assignments and most
         * fervently desire never to see them in the module identity
         */
        oidPtr=( OID *)ubi_dlLast(oidListPtr);
        if ( oidPtr->nodeName == NULL )
        {
            oidPtr->nodeName=moduleIdentity->name;
            oidPtr->resolved=TRUE;
        }
//        parentOidPtr=( OID * )ubi_dlPrev(oidPtr);
    }
}


/*
 * This function takes a parse tree and resolves the OIDs in
 * the object list.
 */
void
resolve_oid_values(SnmpParseTree *mibTree)
{
    TypeOrValue *mibItem=NULL;
    ubi_dlList *itemList=mibTree->objects;
    TypeOrValueType itemType=DATATYPE_NONE;

    if( itemList == NULL || ubi_dlCount(itemList) == 0 )
    {
        return;
    }
    for( mibItem=(TypeOrValue *)ubi_dlFirst(itemList);
         mibItem;
         mibItem=(TypeOrValue *)ubi_dlNext(mibItem))
    {
    OID *itemOid=NULL; /* OID value for this item */
    ubi_dlList *oidListPtr=NULL;

        itemOid=oid_value(mibItem,&oidListPtr);
        if( itemOid == NULL )
        {
            continue;
        }
        itemType = mibItem->dataType;

        for( itemOid=(OID *)ubi_dlPrev(itemOid);
             itemOid;
             itemOid=(OID *)ubi_dlPrev(itemOid))
        {
        SnmpParseTree *parseTree=NULL;
        char *name = itemOid->nodeName;

            itemOid->resolved=FALSE;

            if( ( itemOid->oidVal == 0 ) && ( name == NULL ) )
            {
                itemOid->nodeName = strdup("zeroDotZero");
                itemOid->resolved=TRUE;
            }
            else if( ( itemOid->oidVal == 1 ) && ( name == NULL ) )
            {
                itemOid->nodeName = strdup("iso");
                itemOid->resolved=TRUE;
            }
            else if( strncmp( name, "iso", 3 ) == 0 )
            {
                itemOid->oidVal=1;
                itemOid->resolved=TRUE;
            }
            else if( ( ( itemOid->resolved == FALSE ) && ( name != NULL ) ) &&
                       ( ( strnicmp( name, "zerodotzero", 11 ) == 0 ) ||
                         ( strnicmp( name, "ccitt", 5 ) == 0 ) ) )
            {
                itemOid->oidVal=0;
                itemOid->resolved=TRUE;
            }
            else for( parseTree=(SnmpParseTree *)ubi_dlFirst(&mibList);
                 parseTree;
                 parseTree=(SnmpParseTree *)ubi_dlNext(parseTree))
            {
            /* mibItem for OID value from the parse tree(s)*/
            OID *listOid=NULL;
            /* OID value from searching the parse tree(s) */
            TypeOrValue *listItem = NULL;

                itemType=find_mib_object_by_name(parseTree,name, &listItem);
                if (itemType == DATATYPE_IMPORT)
                {
                    continue;
                }
                else switch(itemType)
                {    
                    case DATATYPE_IMPORT:
                        break;
                    case DATATYPE_VALUE:
                    case DATATYPE_TYPE:
                        listOid = oid_value(listItem, NULL);
                        if (listOid != NULL)
                        {
                            itemOid->oidVal = listOid->oidVal;
                            itemOid->resolved = TRUE;
                        }
                        break;
                    case DATATYPE_NONE:
                    case DATATYPE_LAST:
                    default:
                        break;
                }
                if ( listOid != NULL )
                {
                    /* found it, this way to the egress */
                    break;
                }
            }
        }
    }
    return;
}

/*
 * This function takes a parse tree and resolves the type objects in
 * the object list. At the end the parseTree->types object will be
 * populated with the objects for all non table types defined in the mib
 * For the table types ( SEQUENCE OF, Entry and SEQUENCE objects )
 * These have been moved onto the tables list.
 */
void
resolve_types_and_groups(SnmpParseTree *parseTree)
{
    TypeOrValue *mibItem=NULL;
    TypeOrValue *nextItem=NULL;
    TypeOrValue *typeItem=NULL;
    TypeOrValue *groupItem=NULL;
    TypeOrValue *listItem=NULL;
    SnmpSequenceItemType *seqItem=NULL;

    ubi_dlListPtr objList=parseTree->objects;
    ubi_dlListPtr typeList=parseTree->types;
    ubi_dlListPtr tableList=parseTree->tables;
    ubi_dlListPtr groupList=parseTree->groups;
    ubi_dlListPtr capabilitiesList=parseTree->capabilities;
    ubi_dlListPtr seqList=NULL;
    ubi_dlListPtr itemList=NULL;
    ubi_dlListPtr symbolList=NULL;

    Type *syntax;
    SnmpBaseType itemType=SNMP_UNKNOWN;

    Value *itemValue=NULL;
    char *name=NULL;
    OID *itemOid=NULL;

    TypeOrValueType dataType;
    TypeOrValueType groupItemType;

    SnmpSymbolType *symbolItem=NULL;
    SnmpObjectType *symbolObject=NULL;
    Value *symbolValue=NULL;

    if( objList == NULL || ubi_dlCount(objList) == 0 )
    {
        return;
    }
    /* first we are going to resolve all the type objects in the 
     * parseTree. then we will resolve the named types in terms
     * of the type objects
     */
    for( mibItem=(TypeOrValue *)ubi_dlFirst(objList);
         mibItem;
         mibItem=nextItem)
    {
        nextItem=(TypeOrValue *)ubi_dlNext(mibItem);
        switch(mibItem->dataType)
        {
            case DATATYPE_NONE:
                break;
            case DATATYPE_TYPE:
                switch( mibItem->data.type->type )
                {
                    case SNMP_MACRO_DEFINITION:
                    case SNMP_MODULE_COMPLIANCE:
                    case SNMP_TEXTUAL_CONVENTION:
                    case SNMP_SEQUENCE_IDENTIFIER:
                        break;
                    case SNMP_SEQUENCE:
                        /* add it to the table list */
                        if( tableList == NULL )
                        {
                            tableList=parseTree->tables=lstNew();
                        }
                        /* remove it from the assignments list */
                        ubi_dlRemove(objList,(ubi_dlNodePtr)mibItem);
                        /* Add it to the tables list */
                        ubi_dlAddTail(tableList,(ubi_dlNodePtr)mibItem);

                        /*
                         * For each item in the sequence search the module list
                         * to find the item. If found mark it as resolved
                         */
                        seqList=mibItem->data.type->typeData.sequence->sequence;
                        for( seqItem=
                                 (SnmpSequenceItemType *)ubi_dlFirst(seqList);
                             seqItem;
                             seqItem=(SnmpSequenceItemType *)ubi_dlNext(seqItem))
                        {
                        TypeOrValue *listItem=NULL;
                        TypeOrValueType seqItemType=DATATYPE_NONE;

                            name = seqItem->name;
                            seqItemType=find_list_object_by_name(objList,
                                                                 name,
                                                                 &listItem);
                            switch(seqItemType)
                            {
                                case DATATYPE_IMPORT:
                                    printf("Illegal Sequence data type DATATYPE_IMPORT\n");
                                    break;
                                case DATATYPE_NONE:
                                    /* not found */
                                    printf("\tItem %s Unresolved\n",name);
                                    seqItem->resolved=FALSE;
                                    break;
                                case DATATYPE_TYPE:
                                    /* I wouldn't expect to see Types here */
                                    seqItem->resolved=TRUE;
                                    break;
                                case DATATYPE_VALUE:
                                    /* if we get here then seqItem is valid */
                                   seqItem->resolved=TRUE;
                                   break;
                                case DATATYPE_LAST:
                                default:
                                    /* either default or LAST is an error */
                                    printf("\tInvalid Item %s Unresolved\n",name);
                                    seqItem->resolved=FALSE;
                                    break;
                            }
                        }
                        break;
                    case SNMP_CHOICE:
                        /* add it to the type list */
                        if( typeList == NULL )
                        {
                            typeList=parseTree->types=lstNew();
                        }
                        /* remove it from the assignments list */
                        ubi_dlRemove(typeList,(ubi_dlNodePtr)mibItem);
                        /* Add it to the types list */
                        ubi_dlAddTail(typeList,(ubi_dlNodePtr)mibItem);

                        /*
                         * For each item in the sequence mark it as resolved
                         */
                        seqList=mibItem->data.type->typeData.sequence->sequence;
                        for( seqItem=
                                 (SnmpSequenceItemType *)ubi_dlFirst(seqList);
                             seqItem;
                             seqItem=(SnmpSequenceItemType *)ubi_dlNext(seqItem))
                        {
                            seqItem->resolved=TRUE;
                        }
                        break;
                    case SNMP_OID:
                    case SNMP_BITS:
                    case SNMP_INTEGER:
                    case SNMP_INTEGER32:
                    case SNMP_COUNTER:
                    case SNMP_COUNTER32:
                    case SNMP_COUNTER64:
                    case SNMP_GAUGE:
                    case SNMP_GAUGE32:
                    case SNMP_OCTET_STRING:
                    case SNMP_NAMED_TYPE:
                    case SNMP_TIME_TICKS:
                    case SNMP_OPAQUE_DATA:
                    case SNMP_IP_ADDR:
                    case SNMP_PHYS_ADDR:
                    case SNMP_NETWORK_ADDR:
                        /* add it to the type list */
                        if( typeList == NULL )
                        {
                            typeList=parseTree->types=lstNew();
                        }
                        /* remove it from the assignments list */
                        ubi_dlRemove(objList,(ubi_dlNodePtr)mibItem);
                        /* Add it to the types list */
                        ubi_dlAddTail(typeList,(ubi_dlNodePtr)mibItem);
                        printf("\tImplicit Tag %s %d\n",
                               mibItem->data.type->implicit.className,
                               mibItem->data.type->implicit.tagVal);
                        break;
                    case SNMP_IMPORT_ITEM:
                        printf("Unresolved Import %s\n", object_name(mibItem));
                        break;
                    default:
                        printf("Unresolved Snmp Type Object %s Type %d\n", 
                               object_name(mibItem),mibItem->data.type->type);
                        break;
                     
                }
                break;
            case DATATYPE_VALUE:
            case DATATYPE_LAST:
            case DATATYPE_IMPORT:
            default:
                break;
        }
    }
    /* Now we are going to resolve all the textual conventions objects in the 
     * parseTree. This done after type resolution to ease nested reference
     * problems. At this point all base types have been defined and
     * resolved
     */
    for( mibItem=(TypeOrValue *)ubi_dlFirst(objList);
         mibItem;
         mibItem=nextItem)
    {
        itemType=mibItem->data.type->type;
        nextItem=(TypeOrValue *)ubi_dlNext(mibItem);
        switch(mibItem->dataType)
        {
            case DATATYPE_IMPORT:
                printf("Illegal data type DATATYPE_IMPORT\n");
                break;
            case DATATYPE_TYPE:
                itemType = mibItem->data.type->type;
                if( itemType == SNMP_TEXTUAL_CONVENTION )
                {
                    itemType = (SnmpBaseType)resolve_textual_convention(mibItem);
                    if( itemType == DATATYPE_TYPE )
                    {
                        /* add it to the type list */
                        if( typeList == NULL )
                        {
                            typeList=parseTree->types=lstNew();
                        }
                        /* remove it from the assignments list */
                        ubi_dlRemove(objList,(ubi_dlNodePtr)mibItem);
                        /* Add it to the types list */
                        ubi_dlAddTail(typeList,(ubi_dlNodePtr)mibItem);
                        printf("\tImplicit Tag %s %d\n",
                               mibItem->data.type->implicit.className,
                               mibItem->data.type->implicit.tagVal);
                         
                    }
                }
                break;
            case DATATYPE_NONE:
            case DATATYPE_VALUE:
            case DATATYPE_LAST:
            default:
                break;
        }
    }
    for( mibItem=(TypeOrValue *)ubi_dlFirst(objList);
         mibItem;
         mibItem=nextItem)
    {
        char tag[255];

        nextItem=(TypeOrValue *)ubi_dlNext(mibItem);
        typeItem=NULL;

        memset(tag, 0, sizeof(tag));

        switch(mibItem->dataType)
        {
            case DATATYPE_IMPORT:
                printf("Illegal data type DATATYPE_IMPORT\n");
                break;
            case DATATYPE_NONE:
                break;
            case DATATYPE_TYPE:
                break;
            case DATATYPE_VALUE:
                itemValue=mibItem->data.value;
                name=itemValue->name;
                if ( itemValue->valueType == VALUETYPE_OBJECT_TYPE )
                {
                char *resolved=NULL;
                char *tPtr;
                char *className = NULL;
                    syntax=itemValue->value.object->syntax;
                    sprintf(tag,"Object %s Syntax ",name);
                    if ( ( syntax->type == SNMP_NAMED_TYPE ) &&
                         ( syntax->resolved == FALSE ) )
                    {
                        /* we found an object with a named type 
                         * syntax. Resolve it.
                         */
                         dataType=find_named_type(syntax->name,&typeItem);
                         if ( typeItem != NULL )
                         {
                            if( typeItem->data.type->type != SNMP_IMPORT_ITEM )
                            {
                                 syntax->resolved=TRUE;
                                 resolved="Resolved";
                                 syntax->implicit=typeItem->data.type->implicit;
                             }
                             else
                             {
                                 resolved="Unresolved Import";
                             }
                         }
                    }
                    /*
                     * if we have the type information and we don't
                     * know the base type of the object assign a default
                     */
                    if( ( typeItem != NULL ) &&
                      ( ( syntax->type <= SNMP_UNKNOWN ) ||
                        ( syntax->type >= SNMP_LAST ) ) )
                    {
                        itemType=typeItem->data.type->type;

                        syntax->type=itemType;
                        if( itemType == SNMP_IMPORT_ITEM )
                        {
                            /*
                             * This is another place where it would
                             * be fairly easy to resolve the imports 
                             * to the actual imported type value, if
                             * if available .
                             */
                            resolved="Imported Type";
                        }
                        else 
                        {
                            resolved="Unresolved Invalid Type";
                            syntax->type = SNMP_UNKNOWN;
                        }
                    }
                    switch( syntax->type )
                    {
                       case SNMP_UNKNOWN:
                            resolved = "Unresolved";
                            strcat(tag,"UNKNOWN TYPE ");
                            break;
                       case SNMP_BITS:
                            strcat(tag,"SNMP BITS ");
                            resolved = "Resolved";
                            break;
                       case SNMP_OID:
                            strcat(tag,"SNMP OID ");
                            resolved = "Resolved";
                            break;
                       case SNMP_INTEGER:
                            strcat(tag,"SNMP INTEGER ");
                            resolved = "Resolved";
                            break;
                       case SNMP_INTEGER32:
                            strcat(tag,"SNMP INTEGER32 ");
                            resolved = "Resolved";
                            break;
                       case SNMP_COUNTER:
                            strcat(tag,"SNMP COUNTER ");
                            resolved = "Resolved";
                            break;
                       case SNMP_COUNTER32:
                            strcat(tag,"SNMP COUNTER32 ");
                            resolved = "Resolved";
                            break;
                       case SNMP_COUNTER64:
                            strcat(tag,"SNMP COUNTER64 ");
                            resolved = "Resolved";
                            break;
                       case SNMP_GAUGE:
                            strcat(tag,"SNMP GAUGE ");
                            resolved = "Resolved";
                            break;
                       case SNMP_GAUGE32:
                            strcat(tag,"SNMP GAUGE32 ");
                            resolved = "Resolved";
                            break;
                       case SNMP_MACRO_DEFINITION:
                            strcat(tag,"SNMP MACRO DEFINITION ");
                            resolved = "Resolved";
                            break;
                       case SNMP_IP_ADDR:
                            strcat(tag,"SNMP IP ADDRESS ");
                            resolved = "Resolved";
                            break;
                       case SNMP_OCTET_STRING:
                            strcat(tag,"SNMP OCTET STRING ");
                            resolved = "Resolved";
                            break;
                       case SNMP_SEQUENCE_IDENTIFIER:
                            /* Now add it to the tables list */
                            if( tableList == NULL )
                            {
                                tableList=parseTree->tables=lstNew();
                            }
                            /* remove it from the assignments list */
                            ubi_dlRemove(objList,(ubi_dlNodePtr)mibItem);
                            /* Add it to the tables list */
                            ubi_dlAddTail(tableList,(ubi_dlNodePtr)mibItem);

                            syntax=mibItem->data.value->value.object->syntax;
                            tPtr = tag + strlen(tag);
                            sprintf(tPtr,"SEQUENCE IDENTIFIER %s",syntax->name);
                            dataType=find_named_type(syntax->name,&typeItem);
                            switch(dataType)
                            {
                                case DATATYPE_IMPORT:
                                     resolved="Unresolved Import";
                                    break;
                                case DATATYPE_VALUE:
                                case DATATYPE_TYPE:
                                     syntax->resolved=TRUE;
                                     resolved="Resolved";
                                     syntax->implicit=
                                         typeItem->data.type->implicit;
                                    break;
                                case DATATYPE_NONE:
                                case DATATYPE_LAST:
                                default:
                                    resolved="Unrecognized Type";
                                    break;
                            }
                            break;
                       case SNMP_SEQUENCE:
                            strcat(tag,"SNMP SEQUENCE ");
                            resolved = "Resolved";
                            break;
                       case SNMP_TEXTUAL_CONVENTION:
                            strcat(tag,"SNMP TEXTUAL CONVENTION ");
                            break;
                       case SNMP_MODULE_COMPLIANCE:
                            strcat(tag,"SNMP MODULE COMPLIANCE ");
                            break;
                       case SNMP_IMPORT_ITEM:
                            strcat(tag,"SNMP IMPORT ITEM ");
                            break;
                       case SNMP_NAMED_TYPE:
                            tPtr = tag + strlen(tag);
                            sprintf(tPtr,"SNMP NAMED TYPE %s ", syntax->name);
                            break;
                       case SNMP_TIME_TICKS:
                            strcat(tag,"SNMP TIME TICKS ");
                            resolved = "Resolved";
                            break;
                        default:
                            /* Should never get here */
                            resolved="ERROR";
                            tPtr = tag + strlen(tag);
                            sprintf(tPtr,"Unknown Type %s Value %d ",
                                   syntax->name,syntax->type);
                            break;
                        
                    }
                    className = syntax->implicit.className;

                    if ( className != NULL )
                    {
                        tPtr = tag + strlen(tag);
                        printf(tPtr," Implicit Tag [%s %d]",
                                className, syntax->implicit.tagVal);
                    }
                }
                else if ( itemValue->valueType == VALUETYPE_CAPABILITIES )
                {
                    /* remove it from the assignments list */
                    ubi_dlRemove(objList,(ubi_dlNodePtr)mibItem);
                    /* Add it to the capabilities list */
                    ubi_dlAddTail(capabilitiesList,(ubi_dlNodePtr)mibItem);
                }
                else if ( itemValue->valueType == VALUETYPE_GROUP_VALUE )
                {
                    /* add it to the group list */
                    if( groupList == NULL )
                    {
                        groupList=parseTree->groups=lstNew();
                    }
                    /* remove it from the assignments list */
                    ubi_dlRemove(objList,(ubi_dlNodePtr)mibItem);
                    /* Add it to the groups list */
                    ubi_dlAddTail(groupList,(ubi_dlNodePtr)mibItem);
                    /*
                     * For each item in the group search the module list
                     * to find the item. If found mark it as resolved
                     */
                    itemList=mibItem->data.value->value.group->objects;
                    for( symbolItem=(SnmpSymbolType *)ubi_dlFirst(itemList);
                         symbolItem;
                         symbolItem=(SnmpSymbolType *)ubi_dlNext(symbolItem))
                    {
                        groupItemType=find_list_object_by_name(objList,
                                                          symbolItem->name,
                                                          &groupItem);
                        switch(groupItemType)
                        {
                            case DATATYPE_IMPORT:
                                    printf("Illegal Group Item data type DATATYPE_IMPORT\n");
                                break;
                            case DATATYPE_VALUE:
                                /* if we get here then groupItem is valid */
                                symbolValue=groupItem->data.value;
                                name=symbolValue->name;
                                switch( symbolValue->valueType )
                                {
                                    case VALUETYPE_UNKNOWN:
                                    case VALUETYPE_EMPTY:
                                    case VALUETYPE_IMPORT_ITEM:
                                    case VALUETYPE_MODULE_IDENTITY:
                                    case VALUETYPE_COMMENT:
                                    case VALUETYPE_CAPABILITIES:
                                    case VALUETYPE_INTEGER:
                                    case VALUETYPE_INTEGER32:
                                    case VALUETYPE_BOOLEAN:
                                    case VALUETYPE_OCTET_STRING:
                                    case VALUETYPE_OID:
                                        break;
                                    case VALUETYPE_OBJECT_TYPE:
                                        /* mark the object as in group
                                         * later we can verify group membership
                                         * for all objects 
                                         */
                                        symbolObject=symbolValue->value.object;
                                        symbolObject->hasGroup=TRUE;
                                        /*
                                         * mark the group item as resolved
                                         */
                                        symbolItem->resolved=TRUE;
                                        break;
                                    case VALUETYPE_NAMED_VALUE:
                                    case VALUETYPE_RANGE_VALUE:
                                    case VALUETYPE_SIZE_VALUE:
                                    case VALUETYPE_GROUP_VALUE:
                                    case VALUETYPE_NAMED_NUMBER:
                                    case VALUETYPE_NOTIFICATION:
                                    case VALUETYPE_NULL:
                                    default:
                                        break;
                                }

                                break;
                            case DATATYPE_NONE:
                                /* not found */
                            case DATATYPE_TYPE:
                                /* I wouldn't expect to see types in groups */
                            case DATATYPE_LAST:
                            default:
                                /* either default or LAST is an error */
                                break;
                        }
                    }
                }
                else if ( itemValue->valueType == VALUETYPE_OID )
                {
/*                ubi_dlListPtr oidListPtr = itemValue->value.oidListPtr;
                OID *itemOid=NULL;
                TruthValue first=TRUE;

                    for( itemOid=(OID *)ubi_dlFirst(oidListPtr);
                         itemOid;
                         itemOid=(OID *)ubi_dlNext(itemOid))
                    {
                    char *name=itemOid->nodeName;
                    int val=itemOid->oidVal;

                        if ( !first )
                        {
                            printf(".");
                        }
                        first=FALSE;
                        if( name != NULL )
                        {
                             printf("%s",name);
                        }
                        else
                        {
                            printf("%d",val);
                        }

                    }
                    printf(" }\n");*/
                }
                else if ( itemValue->valueType == VALUETYPE_NOTIFICATION )
                {
                }
                else if ( itemValue->valueType == VALUETYPE_KEYWORD )
                {
                   SnmpCommentType *keyword = itemValue->value.keywordValue;
                    if( keyword->data.keyword.bindings != NULL )
                    {
                        itemList = keyword->data.keyword.bindings;
                        for( symbolItem=(SnmpSymbolType *)ubi_dlFirst(itemList);
                             symbolItem;
                             symbolItem=(SnmpSymbolType *)ubi_dlNext(symbolItem))
                        {
                            name=symbolItem->name;

                            if( symbolItem->resolved == FALSE )
                            {
                                itemType = (SnmpBaseType)find_mib_object_by_name(parseTree,
                                                                                 name,
                                                                                 &listItem);
                                switch(itemType)
                                {
                                    case DATATYPE_IMPORT:
                                        printf("Illegal Keyword Item data type DATATYPE_IMPORT\n");
                                        break;
                                    case DATATYPE_VALUE:
                                    case DATATYPE_TYPE:
                                        listItem->hasKeyword = TRUE;
                                        symbolItem->resolved = TRUE;
                                        break;
                                    case DATATYPE_NONE:
                                    case DATATYPE_LAST:
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                }
                break;
            case DATATYPE_LAST:
            default:
                break;
        }
    }
    return;
}

/*
 * This function takes an agent capabilities objects and resolves it.
 * At the end the module identity
 * capabilities object will be populated with the resolved agent
 * capabilities data. In addition each referenced object in the module
 * capabilities section gets updated with a list of the relevant
 * variations in the agentInfo list.
 */
static void resolve_agent_capabilities(SnmpParseTree *parseTree)
{
    ubi_dlListPtr itemList=parseTree->objects;
    TypeOrValue *mibItem=NULL;
    SnmpVariationType *moduleInfo=NULL;

    if (itemList == NULL || ubi_dlCount(itemList) == 0)
    {
        return;
    }
    /* first we are going to find all the compliance objects in the 
     * parseTree. then we will resolve the named types in terms
     * of the type objects
     */
    for( mibItem=(TypeOrValue *)ubi_dlFirst(itemList);
         mibItem;
         mibItem=(TypeOrValue *)ubi_dlNext(mibItem) )
    {
    SnmpAgentCapabilitiesType *agentInfo=NULL;
    SnmpModuleCapabilitiesType *module=NULL;
    SnmpVariationType *moduleItem=NULL;

        switch(mibItem->dataType)
        {
            case DATATYPE_IMPORT:
                printf("Illegal data type DATATYPE_IMPORT\n");
                break;
            case DATATYPE_VALUE:
                agentInfo=NULL;
                if ( mibItem->data.value->valueType == VALUETYPE_CAPABILITIES )
                {
                    agentInfo = mibItem->data.value->value.agentCapabilitiesValue;
                }
                break;
            case DATATYPE_NONE:
            case DATATYPE_TYPE:
            case DATATYPE_LAST:
            default:
                break;
        }
        if ( agentInfo == NULL )
        {
            continue;
        }
        for( module=(SnmpModuleCapabilitiesType *)ubi_dlFirst(itemList);
             module;
             module=(SnmpModuleCapabilitiesType *)ubi_dlNext(module) )
        {
            ubi_dlList *varianceList=module->variations;
            ubi_dlList *groupList=module->supported_groups;
            TypeOrValueType itemType=DATATYPE_NONE;
            SnmpVariationType *varianceItem=NULL;
            SnmpParseTree *mibTree=NULL;

            mibTree=find_module_by_name(module->module->name);
            if( mibTree == NULL )
            {
                continue;
            }

            if( groupList != NULL && ubi_dlCount(groupList) > 0 )
            {
            SnmpSymbolType *groupItem=NULL;
            ubi_dlList *mibGroups=mibTree->groups;
            char *name=NULL;

                for( groupItem=(SnmpSymbolType *)ubi_dlFirst(groupList);
                     groupItem;
                     groupItem=(SnmpSymbolType *)ubi_dlNext(groupItem) )
                {
                    name=groupItem->name;
                    if ( name == NULL )
                    {
                        continue;
                    }
                    itemType=find_list_object_by_name(mibGroups,name,&mibItem);
                    printf("\tSupported Group %s ", object_name(mibItem));
                    if( mibItem != NULL )
                    {
                        groupItem->resolved=TRUE;
                        mibItem->data.value->value.group->supported=TRUE;
                        printf("Resolved\n", object_name(mibItem));
                        /*
                         * indicates that there is an agent compliance
                         * object for this group
                         */
                    }
                    else
                    {
                        printf("Not Found in Mib %s\n", object_name(mibItem));
                    }
                }
            }
            if( varianceList == NULL || ubi_dlCount(varianceList) == 0 )
            {
                continue;
            }
            for( varianceItem=(SnmpVariationType *)ubi_dlFirst(varianceList);
                 varianceItem;
                 varianceItem=(SnmpVariationType *)ubi_dlNext(varianceItem) )
            {
            TypeOrValueType itemType;
            TypeOrValue *listItem=NULL;
            char *name = varianceItem->object_name;
            ubi_dlListPtr itemList=NULL;

                itemType = find_mib_object_by_name(parseTree, name, &listItem);
                switch(itemType)
                {
                    case DATATYPE_IMPORT:
                        printf("Unexpected Import %s\n", name );
                        break;
                    case DATATYPE_TYPE:
                        itemList = listItem->data.type->capabilities;
                        break;
                    case DATATYPE_VALUE:
                        itemList = listItem->data.value->capabilities;
                        break;
                    case DATATYPE_NONE:
                    case DATATYPE_LAST:
                    default:
                        break;
                }
                if( itemList != NULL )
                {
                    ubi_dlRemove(varianceList,(ubi_dlNodePtr)varianceItem);
                    ubi_dlAddTail(itemList,(ubi_dlNodePtr)varianceItem);
                }
                else
                {
                    printf("Unresolved Agent Variation Object %s\n",name);
                }
            }
        }
    }
    return;
}


//
// Object status
//

static int MIB_ObjectStatus(TypeOrValue *pItem)
{
   if (pItem->dataType == DATATYPE_VALUE)
   {
      if ((pItem->data.value->valueType == VALUETYPE_OBJECT_TYPE) ||
          (pItem->data.value->valueType == VALUETYPE_GROUP_VALUE) ||
          (pItem->data.value->valueType == VALUETYPE_NOTIFICATION))
      {
         return pItem->data.value->value.object->status;
      }
   }
   return -1;
}


//
// Object access
//

static int MIB_ObjectAccess(TypeOrValue *pItem)
{
   if (pItem->dataType == DATATYPE_VALUE)
   {
      if (pItem->data.value->valueType == VALUETYPE_OBJECT_TYPE)
      {
         return pItem->data.value->value.object->access;
      }
   }
   return -1;
}


//
// Object type
//

static int MIB_ObjectType(TypeOrValue *pItem)
{
   if (pItem->dataType == DATATYPE_VALUE)
   {
      if (pItem->data.value->valueType == VALUETYPE_OBJECT_TYPE)
      {
         return pItem->data.value->value.object->syntax->type;
      }
   }
   return -1;
}


//
// Object description
//

static char *MIB_ObjectDescription(TypeOrValue *pItem)
{
   if (pItem->dataType == DATATYPE_VALUE)
   {
      if ((pItem->data.value->valueType == VALUETYPE_OBJECT_TYPE) ||
          (pItem->data.value->valueType == VALUETYPE_GROUP_VALUE) ||
          (pItem->data.value->valueType == VALUETYPE_NOTIFICATION) ||
          (pItem->data.value->valueType == VALUETYPE_MODULE))
      {
         return pItem->data.value->value.object->description;
      }
   }
   return NULL;
}


//
// Build full OID
//

ubi_dlListPtr BuildFullOID(SnmpParseTree *pStartTree, OID *pStart)
{
   OID *pNew, *pCurr = pStart;
   ubi_dlListPtr pFullOID;
   SnmpParseTree *pImportTree, *pTree = pStartTree;
   int nTemp;

   pFullOID = (ubi_dlListPtr)malloc(sizeof(ubi_dlList));
   ubi_dlInitList(pFullOID);
   while(pCurr != NULL)
   {
      pNew = (OID *)nx_memdup(pCurr, sizeof(OID));
      ubi_dlAddHead(pFullOID, pNew);

      if ((ubi_dlPrev(pCurr) == NULL) && (stricmp(pCurr->nodeName, "zeroDotZero")))
      {
         TypeOrValue *pItem;
         int nType;

restart_search:
         nType = find_mib_object_by_name(pTree, pCurr->nodeName, &pItem);
         switch(nType)
         {
            case DATATYPE_VALUE:
            case DATATYPE_TYPE:
               pCurr = oid_value(pItem, NULL);
               if (pCurr != NULL)
               {
                  pCurr = (OID *)ubi_dlPrev(pCurr);
               }
               break;
            case DATATYPE_IMPORT:
               if (find_import_object_by_name(pTree->imports, pCurr->nodeName,
                                              &nTemp, &pImportTree) == OBJECT_RESOLVED)
               {
                  if (pImportTree != NULL)
                  {
                     pTree = pImportTree;
                     goto restart_search;
                  }
                  else
                  {
                     Error(ERR_UNRESOLVED_SYMBOL, pTree->name->name, pCurr->nodeName);
                     pCurr = NULL;
                  }
               }
               else
               {
                  Error(ERR_UNRESOLVED_SYMBOL, pTree->name->name, pCurr->nodeName);
                  pCurr = NULL;
               }
               break;
            default:
               pCurr = NULL;
               break;
         }
      }
      else
      {
         pCurr = (OID *)ubi_dlPrev(pCurr);
      }
   }
   return pFullOID;
}


//
// Build MIB tree from item list
//

static void BuildMIBTree(SNMP_MIBObject *pRoot, SnmpParseTree *pTree, ubi_dlList *pList)
{
   TypeOrValue *pItem;
   OID *oid;
   ubi_dlList *oidListPtr = NULL;
   SNMP_MIBObject *pCurrObj, *pNewObj;

   if (pList == NULL)
      return;
   for(pItem = (TypeOrValue *)ubi_dlFirst(pList); pItem != NULL; pItem = (TypeOrValue *)ubi_dlNext(pItem))
   {
      oid = oid_value(pItem, &oidListPtr);
      if(oid != NULL)
      {
         ubi_dlListPtr pFullOID;

         pFullOID = BuildFullOID(pTree, oid);
         oid = (OID *)ubi_dlFirst(pFullOID);
         pCurrObj = pRoot;
         while(oid != NULL)
         {
            pNewObj = pCurrObj->FindChildByID(oid->oidVal);
            if (pNewObj == NULL)
            {
               if (ubi_dlNext(oid) == NULL)
                  pNewObj = new SNMP_MIBObject(oid->oidVal, oid->nodeName,
                                               MIB_ObjectType(pItem),
                                               MIB_ObjectStatus(pItem),
                                               MIB_ObjectAccess(pItem),
                                               MIB_ObjectDescription(pItem));
               else
                  pNewObj = new SNMP_MIBObject(oid->oidVal, oid->nodeName);
               pCurrObj->AddChild(pNewObj);
            }
            else
            {
               if (ubi_dlNext(oid) == NULL)
               {
                  // Last OID in chain, pItem contains actual object information
                  pNewObj->SetInfo(MIB_ObjectType(pItem),
                                   MIB_ObjectStatus(pItem),
                                   MIB_ObjectAccess(pItem),
                                   MIB_ObjectDescription(pItem));
               }
            }
            pCurrObj = pNewObj;
            oid = (OID *)ubi_dlNext(oid);
         }
      }
   }
}


//
// Interface to parser
//

int ParseMIBFiles(int nNumFiles, char **ppszFileList)
{
   int i, nRet;
   SnmpParseTree *pTree;
   SNMP_MIBObject *pRoot;

   printf("Parsing source files:\n");
   for(i = 0; i < nNumFiles; i++)
   {
      printf("   %s\n", ppszFileList[i]);
      pTree = mpParseMib(ppszFileList[i]);
      if (pTree == NULL)
      {
         nRet = SNMP_MPE_PARSE_ERROR;
         goto parse_error;
      }
      ubi_dlAddTail(&mibList, (ubi_dlNodePtr)pTree);
   }

   printf("Liking modules:\n");
   for(pTree = (SnmpParseTree *)ubi_dlFirst(&mibList); pTree != NULL;
       pTree = (SnmpParseTree *)ubi_dlNext(pTree))
   {
      printf("   %s\n", pTree->name->name);
      resolve_oid_values(pTree);
      resolve_module_identity(pTree);
      resolve_imports(pTree);
      resolve_types_and_groups(pTree);
   }

   printf("Creating MIB tree:\n");
   pRoot = new SNMP_MIBObject;
   for(pTree = (SnmpParseTree *)ubi_dlFirst(&mibList); pTree != NULL;
       pTree = (SnmpParseTree *)ubi_dlNext(pTree))
   {
      printf("   %s\n", pTree->name->name);
      BuildMIBTree(pRoot, pTree, pTree->groups);
      BuildMIBTree(pRoot, pTree, pTree->tables);
      BuildMIBTree(pRoot, pTree, pTree->objects);
      BuildMIBTree(pRoot, pTree, pTree->notifications);
   }

   pRoot->Print(0);
   return SNMP_MPE_SUCCESS;

parse_error:
   return nRet;
}

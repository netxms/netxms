/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: ldap.cpp
**
**/

#include "nxcore.h"

#define LDAP_DEFAULT 0
#define LDAP_USER 1
#define LDAP_GROUP 2

Entry::Entry()
{
   m_type = LDAP_DEFAULT;
   m_loginName = NULL;
   m_fullName = NULL;
   m_description = NULL;
   m_memberList = new StringList();
}

Entry::~Entry()
{
   safe_free(m_loginName);
   safe_free(m_fullName);
   safe_free(m_description);
   delete m_memberList;
}

#if INCLUDE_LDAP

/**
 * Init connection with LDAP(init search line, start checking thread, init check interval)
 */
void LDAPConnection::initLDAP()
{
   DbgPrintf(4, _T("LDAPConnection::initLDAP(): Connect to ldap."));
   int errorCode = ldap_initialize(&m_ldapConn, m_connList);
   if(errorCode != LDAP_SUCCESS)
   {
      TCHAR * error = getErrorString(errorCode);
      DbgPrintf(4, _T("LDAPConnection::initLDAP(): LDAP could not connect to correct server. Error code: %s"), error);
      safe_free(error);
      return;
   }
   //set all LDAP options
   int version = LDAP_VERSION3;
   int result = ldap_set_option(m_ldapConn, LDAP_OPT_PROTOCOL_VERSION, &version); //default verion 2, it's recomended to use version 3

   //reorganize connection list. Set as fist server - just connected server(if is is not so).
}

/**
 * Reads required synchronization parameters
 */
void LDAPConnection::getAllSyncParameters()
{
   ConfigReadStrUTF8(_T("LdapConnectionString"), m_connList, MAX_DB_STRING, (""));
   ConfigReadStrUTF8(_T("LdapSyncUser"), m_userDN, MAX_DB_STRING, (""));
   ConfigReadStrUTF8(_T("LdapSyncUserPassword"), m_userPassword, MAX_DB_STRING, (""));
   ConfigReadStrUTF8(_T("LdapSearchBase"), m_searchBase, MAX_DB_STRING, (""));
   ConfigReadStrUTF8(_T("LdapSearchFilter"), m_searchFilter, MAX_DB_STRING, (""));
   m_action = ConfigReadInt(_T("LdapUserDeleteAction"), 1); //default value - to disable user(value=1)
   ConfigReadStrUTF8(_T("LdapMappingName"), m_ldapLoginNameAttr, MAX_DB_STRING, (""));
   ConfigReadStrUTF8(_T("LdapMappingFullName"), m_ldapFullNameAttr, MAX_DB_STRING, (""));
   ConfigReadStrUTF8(_T("LdapMappingDescription"), m_ldapDescriptionAttr, MAX_DB_STRING, (""));
   ConfigReadStr(_T("LdapGroupClass"), m_groupClass, MAX_DB_STRING, _T(""));
   ConfigReadStr(_T("LdapUserClass"), m_userClass, MAX_DB_STRING, _T(""));
}

/**
 * Get users, according to search line & insetr in netxms DB missing
 */
void LDAPConnection::syncUsers()
{
   getAllSyncParameters();
   initLDAP();
   if (loginLDAP() != RCC_SUCCESS)
   {
      DbgPrintf(6, _T("LDAPConnection::syncUsers(): Could not login."));
      return;
   }

   int rc, i;
   struct timeval timeOut = {10,0}; // 10 second connecion/search timeout
   LDAPMessage *searchResult, *entry;
   char *attribute;
   BerElement *ber;
   StringObjectMap<Entry> *userEntryList = new StringObjectMap<Entry>(true); //as unique string ID is used dn
   StringObjectMap<Entry> *groupEntryList= new StringObjectMap<Entry>(true); //as unique string ID is used dn

   //arg - contain LDAP link, search line,
   rc = ldap_search_ext_s(
            m_ldapConn,		// LDAP session handle
            m_searchBase,	// Search Base
            LDAP_SCOPE_SUBTREE,	// Search Scope – everything below o=Acme
            m_searchFilter, // Search Filter – only inetOrgPerson objects
            NULL,	// returnAllAttributes – NULL means Yes
            0,		// attributesOnly – False means we want values
            NULL,	// Server controls – There are none
            NULL,	// Client controls – There are none
            &timeOut,	// search Timeout
            LDAP_NO_LIMIT,	// no size limit
            &searchResult );

   if ( rc != LDAP_SUCCESS )
   {
      TCHAR* error = getErrorString(rc);
      DbgPrintf(1, _T("LDAPConnection::syncUsers(): LDAP could not get search results. Error code: %s"), error);
      safe_free(error);
      return;
   }
   bool update = false;
   DbgPrintf(4, _T("LDAPConnection::syncUsers(): Found entry count: %d"), ldap_count_entries(m_ldapConn, searchResult));
   for (entry = ldap_first_entry(m_ldapConn, searchResult); entry != NULL; entry = ldap_next_entry(m_ldapConn, entry))
   {
      Entry *newObj = new Entry();
#ifdef UNICODE
      char *_dn = ldap_get_dn(m_ldapConn, entry);
      TCHAR *dn = WideStringFromUTF8String(_dn);
#else
      char *dn = ldap_get_dn(m_ldapConn, entry);
#endif // UNICODE
      DbgPrintf(4, _T("LDAPConnection::syncUsers(): Found dn: %s"), dn);
      TCHAR *value;
      for(i = 0, value = getAttrValue(entry, "objectClass", i); value != NULL; value = getAttrValue(entry, "objectClass", ++i))
      {
         if(_tcscmp(value, m_userClass) == 0)
         {
            newObj->m_type = LDAP_USER;
            safe_free(value);
            break;
         }
         if(_tcscmp(value, m_groupClass) == 0)
         {
            newObj->m_type = LDAP_GROUP;
            safe_free(value);
            break;
         }
         safe_free(value);
      }

      if(newObj->m_type == LDAP_DEFAULT)
      {
         DbgPrintf(4, _T("LDAPConnection::syncUsers(): %s is not a user nor a group"), dn);
#ifdef UNICODE
         free(dn);
         ldap_memfree(_dn);
#else
         ldap_memfree(dn);
#endif
         delete newObj;
         continue;
      }
      for(attribute = ldap_first_attribute(m_ldapConn, entry, &ber); attribute != NULL; attribute = ldap_next_attribute(m_ldapConn, entry, ber))
      {
         //We get values only for those attributes that are used for user/group creation
         if(strcmp(attribute, m_ldapFullNameAttr) == 0)
         {
            newObj->m_fullName = getAttrValue(entry, attribute);
         }
         else if(strcmp(attribute, m_ldapLoginNameAttr) == 0)
         {
            newObj->m_loginName = getAttrValue(entry, attribute);
         }
         else if(strcmp(attribute, m_ldapDescriptionAttr) == 0)
         {
            newObj->m_description = getAttrValue(entry, attribute);
         }
         else if(strcmp(attribute, "member") == 0)
         {
            i = 0;
            TCHAR* value = getAttrValue(entry, attribute, i);
            do
            {
               DbgPrintf(4, _T("LDAPConnection::syncUsers(): member: %s"), value);
               newObj->m_memberList->addPreallocated(value);
               value = getAttrValue(entry, attribute, ++i);
            }while(value != NULL);
         }
         ldap_memfree(attribute);
      }
      ber_free(ber, 0);

      //entry is added only if it was of a correct type
      if(newObj->m_type == LDAP_USER && newObj->m_loginName != NULL)
      {
         DbgPrintf(4, _T("LDAPConnection::syncUsers(): User added: dn: %s, login name: %s, full name: %s, description: %s"), dn, newObj->m_loginName, newObj->m_fullName, newObj->m_description);
         userEntryList->set(dn, newObj);
      }
      else if(newObj->m_type == LDAP_GROUP && newObj->m_loginName != NULL)
      {
         DbgPrintf(4, _T("LDAPConnection::syncUsers(): Group added: dn: %s, login name: %s, full name: %s, description: %s"), dn, newObj->m_loginName, newObj->m_fullName, newObj->m_description);
         groupEntryList->set(dn, newObj);
      }
      else
      {
         DbgPrintf(4, _T("LDAPConnection::syncUsers(): Unknown object is not added: dn: %s, login name: %s, full name: %s, description: %s"), dn, newObj->m_loginName, newObj->m_fullName, newObj->m_description);
         delete newObj;
      }
#ifdef UNICODE
      free(dn);
      ldap_memfree(_dn);
#else
      ldap_memfree(dn);
#endif
   }
   ldap_msgfree(searchResult);

   closeLDAPConnection();

   //compare new LDAP list with old users
   compareUserLists(userEntryList);
   compareGroupList(groupEntryList);

   delete userEntryList;
   delete groupEntryList;
}

TCHAR *LDAPConnection::getAttrValue(LDAPMessage *entry, const char *attr, int i)
{
   TCHAR *result = NULL;
   berval **values;
   values = ldap_get_values_len(m_ldapConn, entry, attr);
   if (ldap_count_values_len(values) > i)
   {
#ifdef UNICODE
      result = WideStringFromUTF8String(values[i]->bv_val);
#else
      result = strdup(values[i]->bv_val);
#endif // UNICODE
   }
   ldap_value_free_len(values);
   return result;
}

/**
 * Updates user list according to newly recievd user list
 */
void LDAPConnection::compareUserLists(StringObjectMap<Entry>* userEntryList)
{
   for(int i = 0; i < userEntryList->getSize(); i++)
   {
      UpdateLDAPUsers(userEntryList->getKeyByIndex(i), userEntryList->getValueByIndex(i));
   }
   RemoveDeletedLDAPEntry(userEntryList, m_action, true);
}

/**
 * Updates group list according to newly recievd user list
 */
void LDAPConnection::compareGroupList(StringObjectMap<Entry>* groupEntryList)
{
   for(int i = 0; i < groupEntryList->getSize(); i++)
   {
      UpdateLDAPGroups(groupEntryList->getKeyByIndex(i), groupEntryList->getValueByIndex(i));
   }
   RemoveDeletedLDAPEntry(groupEntryList, m_action, false);
}

/**
 * Coverts given parameters to correct encoding, login to ldap and close connection.
 * This function should be used to check connection. As name should be given user dn.
 */
UINT32 LDAPConnection::ldapUserLogin(const TCHAR *name, const TCHAR *password)
{
   getAllSyncParameters();
   initLDAP();
   UINT32 result;
#ifdef UNICODE
   char *utf8Name = UTF8StringFromWideString(name);
   strcpy(m_userDN, utf8Name);
   safe_free(utf8Name);
   char *utf8Password = UTF8StringFromWideString(password);
   strcpy(m_userPassword, utf8Password);
   safe_free(utf8Password);
#else
   strcpy(m_userDN, name);
   strcpy(m_userPassword, password);
#endif // UNICODE
   result = loginLDAP();
   closeLDAPConnection();
   return result;
}

/**
 * Autentificate LDAP user
 */
 UINT32 LDAPConnection::loginLDAP()
 {
   int ldap_error;
   struct berval cred;
   cred.bv_val = m_userPassword;
   cred.bv_len = strlen(m_userPassword);

   if(m_ldapConn != NULL)
   {
      ldap_error = ldap_sasl_bind_s(m_ldapConn, m_userDN, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
      if (ldap_error == LDAP_SUCCESS)
      {
         return RCC_SUCCESS;
      }
      else
      {
         TCHAR *error = getErrorString(ldap_error);
         DbgPrintf(4, _T("LDAPConnection::loginLDAP(): LDAP could not login. Error code: %s"), error);
         safe_free(error);
      }
   }
   else
   {
      return RCC_NO_LDAP_CONNECTION;
   }
   return RCC_ACCESS_DENIED;
 }

 /**
  * Converts error string and returns in right format.
  * Memory should be released by caller.
  */
TCHAR *LDAPConnection::getErrorString(int ldap_error)
{
   #ifdef UNICODE
   return WideStringFromUTF8String(ldap_err2string(ldap_error));
   #else
   return strdup(ldap_err2string(ldap_error));
   #endif // UNICODE
}

/**
 * Close LDAP connection
 */
void LDAPConnection::closeLDAPConnection()
{
   DbgPrintf(4, _T("LDAPConnection::closeLDAPConnection(): Disconnect form ldap."));
   if(m_ldapConn != NULL)
      ldap_unbind_ext(m_ldapConn, NULL, NULL);
}

/**
 * Defeult LDAPConnection class construtor
 */
LDAPConnection::LDAPConnection()
{
   m_ldapConn = NULL;
   m_action = 1;
}

/**
 * Destructor
 */
LDAPConnection::~LDAPConnection()
{
}

#else

void LDAPConnection::syncUsers()
{
    DbgPrintf(4, _T("LDAPConnection::syncUsers(): Ldap library is not declared so users woun't be synchronized."));
}

UINT32 LDAPConnection::ldapUserLogin(const TCHAR *name, const TCHAR *password)
{
   DbgPrintf(4, _T("LDAPConnection::ldapUserLogin(): Ldap library is not declared so login is not possible."));
   return RCC_INTERNAL_ERROR;
}
#endif

/**
 * Get users, according to search line & insetr in netxms DB missing
 */
THREAD_RESULT THREAD_CALL SyncLDAPUsers(void *arg)
{
   UINT32 timeInMinutes = ConfigReadInt(_T("LdapSyncInterval"), 0);
   timeInMinutes *= 60;
   while(!SleepAndCheckForShutdown(timeInMinutes))
   {
      LDAPConnection conn;
      conn.syncUsers();
   }

	return THREAD_OK;
}



/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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

#ifndef _WIN32

#define ldap_first_attributeA    ldap_first_attribute
#define ldap_next_attributeA     ldap_next_attribute
#define ldap_memfreeA            ldap_memfree
#define ldap_get_values_lenA     ldap_get_values_len

#endif

#define LDAP_DEFAULT 0
#define LDAP_USER 1
#define LDAP_GROUP 2

#ifndef LDAP_SSL_PORT
#define LDAP_SSL_PORT 636
#endif

#define LDAP_PAGE_OID "1.2.840.113556.1.4.319"

#define LDAP_DEBUG_TAG  _T("ldap")

/**
 * LDAP entry constructor
 */
Entry::Entry()
{
   m_type = LDAP_DEFAULT;
   m_loginName = NULL;
   m_fullName = NULL;
   m_description = NULL;
   m_id = NULL;
   m_memberList = new StringSet();
}

/**
 * LDAP entry destructor
 */
Entry::~Entry()
{
   MemFree(m_loginName);
   MemFree(m_fullName);
   MemFree(m_description);
   MemFree(m_id);
   delete m_memberList;
}

#if WITH_LDAP

#if !defined(_WIN32) && !HAVE_LDAP_CONTROL_CREATE && !HAVE_LDAP_CREATE_CONTROL && HAVE_NSLDAPI_BUILD_CONTROL

#if !HAVE_DECL_NSLDAPI_BUILD_CONTROL
extern "C" int nsldapi_build_control(const char *oid, BerElement *ber, int freeber, char iscritical, LDAPControl **ctrlp);
#endif

static int ldap_create_control(const char *oid, BerElement *ber, int iscritical, LDAPControl **ctrlp)
{
	return nsldapi_build_control(oid, ber, 0, iscritical, ctrlp);
}

#endif

#if !HAVE_LDAP_CREATE_PAGE_CONTROL && !defined(_WIN32)
int ldap_create_page_control(LDAP *ldap, ber_int_t pagesize,
			     struct berval *cookie, char isCritical,
			     LDAPControl **output)
{
	BerElement *ber;
	int rc;

	if (!ldap || !output)
		return LDAP_PARAM_ERROR;

	ber = ber_alloc_t(LBER_USE_DER);
	if (!ber)
		return LDAP_NO_MEMORY;

	if (ber_printf(ber, "{io}", pagesize,
			(cookie && cookie->bv_val) ? cookie->bv_val : "",
			(cookie && cookie->bv_val) ? cookie->bv_len : 0)
				== -1)
	{
		ber_free(ber, 1);
		return LDAP_ENCODING_ERROR;
	}

#if HAVE_LDAP_CONTROL_CREATE
   rc = ldap_control_create(LDAP_PAGE_OID, isCritical, cookie, 0, output);
#else
	rc = ldap_create_control(LDAP_PAGE_OID, ber, isCritical, output);
#endif

	return rc;
}
#endif /* HAVE_LDAP_CREATE_PAGE_CONTROL */

#if !HAVE_LDAP_PARSE_PAGE_CONTROL && !defined(_WIN32)
int ldap_parse_page_control(LDAP *ldap, LDAPControl **controls,
			    ber_int_t *totalcount, struct berval **cookie)
{
	BerElement *theBer;
	LDAPControl *listCtrlp;

	for(int i = 0; controls[i] != NULL; i++) {
		if (strcmp(controls[i]->ldctl_oid, LDAP_PAGE_OID) == 0) {
			listCtrlp = controls[i];

			theBer = ber_init(&listCtrlp->ldctl_value);
			if (!theBer)
				return LDAP_NO_MEMORY;

			if (ber_scanf(theBer, "{iO}", totalcount, cookie) == LBER_ERROR)
			{
				ber_free(theBer, 1);
				return LDAP_DECODING_ERROR;
			}

			ber_free(theBer, 1);
			return LDAP_SUCCESS;
		}
	}

	return LDAP_CONTROL_NOT_FOUND;
}
#endif /* HAVE_LDAP_PARSE_PAGE_CONTROL */

/**
 * Correctly formats ldap connection string (according to OS)
 */
void LDAPConnection::prepareStringForInit(LDAP_CHAR *connectionLine)
{
   LDAP_CHAR *comma;
   LDAP_CHAR *lastSlash;
   LDAP_CHAR *nearestSpace;

   comma = ldap_strchr(connectionLine, _TLDAP(','));
   while(comma != NULL)
   {
      *comma = _TLDAP(' ');
      comma = ldap_strchr(connectionLine, _TLDAP(','));
   }

   if (ldap_strstr(connectionLine, _TLDAP("ldaps://")))
   {
      m_secure = 1;
   }

   lastSlash=ldap_strchr(connectionLine, _TLDAP('/'));
   while(lastSlash != NULL)
   {
      *lastSlash = 0;
      lastSlash++;

      nearestSpace = ldap_strchr(connectionLine, _TLDAP(' '));
      if (nearestSpace == NULL)
      {
         nearestSpace = connectionLine;
      }
      else
      {
         nearestSpace++;
      }
      *nearestSpace = 0;
      ldap_strcat(connectionLine, lastSlash);

      lastSlash = ldap_strchr(connectionLine, _TLDAP('/'));
   }
}

/**
 * Init connection with LDAP(init search line, start checking thread, init check interval)
 */
void LDAPConnection::initLDAP()
{
   nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::initLDAP(): Connecting to LDAP server"));
#if HAVE_LDAP_INITIALIZE
   int errorCode = ldap_initialize(&m_ldapConn, m_connList);
   if (errorCode != LDAP_SUCCESS)
#else
   prepareStringForInit(m_connList);
   int port = m_secure ? LDAP_SSL_PORT : LDAP_PORT;
#ifdef _WIN32
   if (m_connList[0] == 0)
   {
      m_ldapConn = ldap_sslinit(NULL, port, m_secure);
   }
   else
   {
      nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::initLDAP(): servers=\"%s\" port=%d secure=%s"), m_connList, port, m_secure ? _T("yes") : _T("no"));
      m_ldapConn = ldap_sslinit(m_connList, port, m_secure);
   }
#else
   if(m_secure)
   {
#if HAVE_LDAPSSL_INIT
      ldapssl_init(m_connList, port, m_secure);
#else
      nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::initLDAP(): Your LDAP library does not support secure connection."));
#endif //HAVE_LDAPSSL_INIT
   }
   else
   {
      m_ldapConn = ldap_init(m_connList, LDAP_PORT);
   }
#endif // _WIN32

#ifdef _WIN32
   ULONG errorCode = LdapGetLastError();
#else
   int errorCode = errno;
#endif // _WIN32
   if (m_ldapConn == NULL)
#endif // HAVE_LDAP_INITIALIZE
   {
      nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::initLDAP(): LDAP session initialization failed (%s)"), getErrorString(errorCode).cstr());
      return;
   }
   //set all LDAP options
   int version = LDAP_VERSION3;
   ldap_set_option(m_ldapConn, LDAP_OPT_PROTOCOL_VERSION, &version); //default verion 2, it's recomended to use version 3
   ldap_set_option(m_ldapConn, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
   nxlog_debug_tag(LDAP_DEBUG_TAG, 6, _T("LDAPConnection::initLDAP(): LDAP session initialized"));
}

/**
 * Reads required synchronization parameters
 */
void LDAPConnection::getAllSyncParameters()
{
   TCHAR tmpPwd[MAX_PASSWORD];
   TCHAR tmpLogin[MAX_CONFIG_VALUE];
   ConfigReadStr(_T("LDAP.SyncUserPassword"), tmpPwd, MAX_PASSWORD, _T(""));
   ConfigReadStr(_T("LDAP.SyncUser"), tmpLogin, MAX_CONFIG_VALUE, _T(""));
   DecryptPassword(tmpLogin, tmpPwd, tmpPwd, MAX_PASSWORD);
   LdapConfigRead(_T("LDAP.ConnectionString"), m_connList, MAX_CONFIG_VALUE, _TLDAP(""));
   LdapConfigRead(_T("LDAP.SyncUser"), m_userDN, MAX_CONFIG_VALUE, _TLDAP(""));
   LdapConfigRead(_T("LDAP.SearchBase"), m_searchBase, MAX_CONFIG_VALUE, _TLDAP(""));
   LdapConfigRead(_T("LDAP.SearchFilter"), m_searchFilter, MAX_CONFIG_VALUE, _TLDAP("(objectClass=*)"));
   if (m_searchFilter[0] == 0)
      ldap_strcpy(m_searchFilter, _TLDAP("(objectClass=*)"));
#ifdef _WIN32
   _tcsncpy(m_userPassword, tmpPwd, MAX_PASSWORD);
#else

#ifdef UNICODE
   char *utf8Password = UTF8StringFromWideString(tmpPwd);
   strcpy(m_userPassword, utf8Password);
   MemFree(utf8Password);
#else
   char *utf8Password = UTF8StringFromMBString(tmpPwd);
   strcpy(m_userPassword, utf8Password);
   MemFree(utf8Password);
#endif // UNICODE

#endif //win32
   ConfigReadStrUTF8(_T("LDAP.UserMappingName"), m_ldapUserLoginNameAttr, MAX_CONFIG_VALUE, "");
   ConfigReadStrUTF8(_T("LDAP.GroupMappingName"), m_ldapGroupLoginNameAttr, MAX_CONFIG_VALUE, "");
   ConfigReadStrUTF8(_T("LDAP.MappingFullName"), m_ldapFullNameAttr, MAX_CONFIG_VALUE, "");
   ConfigReadStrUTF8(_T("LDAP.MappingDescription"), m_ldapDescriptionAttr, MAX_CONFIG_VALUE, "");
   ConfigReadStrUTF8(_T("LDAP.UserUniqueId"), m_ldapUsreIdAttr, MAX_CONFIG_VALUE, "");
   ConfigReadStrUTF8(_T("LDAP.GroupUniqueId"), m_ldapGroupIdAttr, MAX_CONFIG_VALUE, "");
   ConfigReadStr(_T("LDAP.GroupClass"), m_groupClass, MAX_CONFIG_VALUE, _T(""));
   ConfigReadStr(_T("LDAP.UserClass"), m_userClass, MAX_CONFIG_VALUE, _T(""));
   m_action = ConfigReadInt(_T("LDAP.UserDeleteAction"), 1); //default value - to disable user(value=1)
   m_pageSize = ConfigReadInt(_T("LDAP.PageSize"), 1000); //default value - 1000
}

/**
 * Get users, according to search line & insetr in netxms DB missing
 */
void LDAPConnection::syncUsers()
{
   getAllSyncParameters();
   initLDAP();
   UINT32 rcc = loginLDAP();
   if (rcc != RCC_SUCCESS)
   {
      nxlog_debug_tag(LDAP_DEBUG_TAG, 6, _T("LDAPConnection::syncUsers(): login failed (error %u)"), rcc);
      return;
   }

   struct ldap_timeval timeOut = { 10, 0 }; // 10 second connecion/search timeout
   delete_and_null(m_userDnEntryList);
   delete_and_null(m_userIdEntryList);
   delete_and_null(m_groupDnEntryList);
   delete_and_null(m_groupIdEntryList);
   m_userDnEntryList = new StringObjectMap<Entry>(Ownership::True); //as unique string ID is used DN
   m_userIdEntryList = new StringObjectMap<Entry>(Ownership::False); //as unique string ID is used id
   m_groupDnEntryList= new StringObjectMap<Entry>(Ownership::True); //as unique string ID is used DN
   m_groupIdEntryList= new StringObjectMap<Entry>(Ownership::False); //as unique string ID is used id

   //Parse search base string. As separater is used ';' if this symbol is not escaped
   LDAP_CHAR *tmp  = ldap_strdup(m_searchBase);
   LDAP_CHAR *separator = tmp;
   LDAP_CHAR *base = tmp;
   size_t size = ldap_strlen(tmp);
   int rc = LDAP_SUCCESS;

   while (separator != NULL)
   {
      while (true)
      {
         separator = ldap_strchr(separator, ';');
         if(separator != NULL)
         {
            if ((separator - tmp) > 0)
            {
               if(separator[-1] != '\\')
               {
                  separator[0] = 0;
               }
               else
               {
                  separator++;
                  continue;
               }
            }
            else
            {
               base++;
               separator++;
               continue;
            }
         }
         break;
      }

      nxlog_debug_tag(LDAP_DEBUG_TAG, 6, _T("LDAPConnection::syncUsers(): search base DN = ") LDAP_TFMT, base);
      LDAPMessage *searchResult;
      rc = ldap_search_ext_s(
               m_ldapConn,		// LDAP session handle
               base,	// Search Base
               LDAP_SCOPE_SUBTREE,	// Search Scope – everything below o=Acme
               m_searchFilter, // Search Filter – only inetOrgPerson objects
               NULL,	// returnAllAttributes – NULL means Yes
               0,		// attributesOnly – False means we want values
               NULL,	// Server controls – There are none
               NULL,	// Client controls – There are none
               &timeOut,	// search Timeout
               LDAP_NO_LIMIT,	// no size limit
               &searchResult);
      if (rc != LDAP_SUCCESS)
      {
         if (rc == LDAP_SIZELIMIT_EXCEEDED)
         {
            nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("LDAPConnection::syncUsers(): got LDAP_SIZELIMIT_EXCEEDED, will use paged read"));
            rc = readInPages(base);
         }
         else
         {
            nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("LDAPConnection::syncUsers(): LDAP could not get search results. Error code: %s"), getErrorString(rc).cstr());
         }
      }
      else
      {
         fillLists(searchResult);
      }
      ldap_msgfree(searchResult);

      if ((separator != NULL) && ((size_t)(separator - tmp) < size))
      {
         separator++;
         base = separator;
      }
      else
      {
         separator = NULL;
      }
   }

   MemFree(tmp);
   closeLDAPConnection();

   if (rc == LDAP_SUCCESS)
   {
      // compare new LDAP list with old users
      nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("LDAPConnection::syncUsers(): read completed, updating user database"));
      compareUserLists();
      compareGroupList();
   }
   else
   {
      nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("LDAPConnection::syncUsers(): LDAP will not sync users. Error code: %s"), getErrorString(rc).cstr());
   }

   delete_and_null(m_userDnEntryList);
   delete_and_null(m_userIdEntryList);
   delete_and_null(m_groupDnEntryList);
   delete_and_null(m_groupIdEntryList);
}

/**
 * Reads and process each page
 */
int LDAPConnection::readInPages(LDAP_CHAR *base)
{
   nxlog_debug_tag(LDAP_DEBUG_TAG, 7, _T("LDAPConnection::readInPages(): reading directory in pages"));

   LDAPControl *pageControl = NULL, *controls[2] = { NULL, NULL };
	LDAPControl **returnedControls = NULL;
#if defined(__sun) && !SUNOS_OPENLDAP
   unsigned int pageSize = m_pageSize;
   unsigned int totalCount = 0;
#elif defined(_WIN32)
   ULONG pageSize = m_pageSize;
   ULONG totalCount = 0;
#else
   ber_int_t pageSize = m_pageSize;
   ber_int_t totalCount = 0;
#endif
   char pagingCriticality = 'T';
   struct berval *cookie = NULL;

   struct ldap_timeval timeOut = { 10, 0 }; // 10 second connection/search timeout
   int rc;

   do
   {
      rc = ldap_create_page_control(m_ldapConn, pageSize, cookie, pagingCriticality, &pageControl);
      if (rc != LDAP_SUCCESS)
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("LDAPConnection::readInPages(): LDAP could not create page control. Error code: %s"), getErrorString(rc).cstr());
         break;
      }

      /* Insert the control into a list to be passed to the search. */
      controls[0] = pageControl;

      nxlog_debug_tag(LDAP_DEBUG_TAG, 6, _T("LDAPConnection::readInPages(): search base DN = ") LDAP_TFMT, base);
      LDAPMessage *searchResult;
      rc = ldap_search_ext_s(
               m_ldapConn,		// LDAP session handle
               base,	// Search Base
               LDAP_SCOPE_SUBTREE,	// Search Scope – everything below o=Acme
               m_searchFilter, // Search Filter – only inetOrgPerson objects
               NULL,	// returnAllAttributes – NULL means Yes
               0,		// attributesOnly – False means we want values
               controls,	// Server controls – Page controls
               NULL,	// Client controls – There are none
               &timeOut,	// search Timeout
               LDAP_NO_LIMIT,	// no size limit
               &searchResult);
      if ((rc != LDAP_SUCCESS) && (rc != LDAP_PARTIAL_RESULTS))
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("LDAPConnection::readInPages(): Error reading result page: %s"), getErrorString(rc).cstr());
         ldap_msgfree(searchResult);
         ldap_control_free(pageControl);
         break;
      }

      /* Parse the results to retrieve the controls being returned. */
      rc = ldap_parse_result(m_ldapConn, searchResult, NULL, NULL, NULL, NULL, &returnedControls, FALSE);
      if (rc != LDAP_SUCCESS)
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("LDAPConnection::readInPages(): Error parsing results: %s"), getErrorString(rc).cstr());
         ldap_msgfree(searchResult);
         ldap_control_free(pageControl);
         break;
      }

      fillLists(searchResult);
      ldap_msgfree(searchResult);

      /* Clear cookie */
      if (cookie != NULL)
      {
         ber_bvfree(cookie);
         cookie = NULL;
      }

      /*
       * Parse the page control returned to get the cookie and
       * determine whether there are more pages.
       */
      rc = ldap_parse_page_control(m_ldapConn, returnedControls, &totalCount, &cookie);
      if (rc != LDAP_SUCCESS)
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("LDAPConnection::readInPages(): Error parsing page control: %s"), getErrorString(rc).cstr());
         ldap_control_free(pageControl);
         break;
      }

      /* Cleanup the controls used. */
      if (returnedControls != NULL)
         ldap_controls_free(returnedControls);

      ldap_control_free(pageControl);
   } while(cookie && cookie->bv_val && strlen(cookie->bv_val));

   if (cookie != NULL)
      ber_bvfree(cookie);

   return rc;
}

/**
 * Get DN from LDAP message
 */
TCHAR *LDAPConnection::dnFromMessage(LDAPMessage *entry)
{
#ifdef _WIN32
   TCHAR *_dn = ldap_get_dn(m_ldapConn, entry);
   TCHAR *dn = MemCopyString(_dn);
#else
   char *_dn = ldap_get_dn(m_ldapConn, entry);
#ifdef UNICODE
   WCHAR *dn = WideStringFromUTF8String(_dn);
#else
   char *dn = MBStringFromUTF8String(_dn);
#endif
#endif
   ldap_memfree(_dn);
   return dn;
}

/**
 * Fills lists of users and groups from search results
 */
void LDAPConnection::fillLists(LDAPMessage *searchResult)
{
   LDAPMessage *entry;
   char *attribute;
   BerElement *ber;
	int i;
   nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::fillLists(): Found entry count: %d"), ldap_count_entries(m_ldapConn, searchResult));
   for (entry = ldap_first_entry(m_ldapConn, searchResult); entry != NULL; entry = ldap_next_entry(m_ldapConn, entry))
   {
      Entry *ldapObject = new Entry();
      TCHAR *dn = dnFromMessage(entry);
      nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::fillLists(): Found DN: %s"), dn);

      StringBuffer objectClasses;
      TCHAR *value;
      for(i = 0, value = getAttrValue(entry, "objectClass", i); value != NULL; value = getAttrValue(entry, "objectClass", ++i))
      {
         if (!objectClasses.isEmpty())
            objectClasses.append(_T(","));
         objectClasses.append(value);
         if (!_tcscmp(value, m_userClass))
         {
            ldapObject->m_type = LDAP_USER;
            MemFree(value);
            break;
         }
         if (!_tcscmp(value, m_groupClass))
         {
            ldapObject->m_type = LDAP_GROUP;
            MemFree(value);
            break;
         }
         MemFree(value);
      }

      if (ldapObject->m_type == LDAP_DEFAULT)
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::fillLists(): %s is not a user or a group (classes: %s)"), dn, objectClasses.cstr());
         MemFree(dn);
         delete ldapObject;
         continue;
      }

      for(attribute = ldap_first_attributeA(m_ldapConn, entry, &ber); attribute != NULL; attribute = ldap_next_attributeA(m_ldapConn, entry, ber))
      {
         // We get values only for those attributes that are used for user/group creation
         if (!stricmp(attribute, m_ldapFullNameAttr))
         {
            ldapObject->m_fullName = getAttrValue(entry, attribute);
         }
         if (!stricmp(attribute, m_ldapUserLoginNameAttr) && ldapObject->m_type == LDAP_USER)
         {
            ldapObject->m_loginName = getAttrValue(entry, attribute);
         }
         if (!stricmp(attribute, m_ldapGroupLoginNameAttr) && ldapObject->m_type == LDAP_GROUP)
         {
            ldapObject->m_loginName = getAttrValue(entry, attribute);
         }
         if (!stricmp(attribute, m_ldapDescriptionAttr))
         {
            ldapObject->m_description = getAttrValue(entry, attribute);
         }
         if (m_ldapUsreIdAttr[0] != 0 && !stricmp(attribute, m_ldapUsreIdAttr) && ldapObject->m_type == LDAP_USER)
         {
            ldapObject->m_id = getIdAttrValue(entry, attribute);
         }
         if (m_ldapGroupIdAttr[0] != 0 && !stricmp(attribute, m_ldapGroupIdAttr) && ldapObject->m_type == LDAP_GROUP)
         {
            ldapObject->m_id = getIdAttrValue(entry, attribute);
         }
         if (!stricmp(attribute, "member"))
         {
            i = 0;
            TCHAR *value = getAttrValue(entry, attribute, i);
            while(value != NULL)
            {
               nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::fillLists(): member: %s"), value);
               ldapObject->m_memberList->addPreallocated(value);
               value = getAttrValue(entry, attribute, ++i);
            }
         }
         if (!strncmp(attribute, "member;range=", 13))
         {
            nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::fillLists(): found member attr: %hs"), attribute);
            nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::fillLists(): there are more members, than can be provided in one request"));
            //There are more members than can be provided in one request
#if !defined(_WIN32) && defined(UNICODE)
            char *tmpDn = UTF8StringFromWideString(dn);
            updateMembers(ldapObject->m_memberList, attribute, entry, tmpDn);
            MemFree(tmpDn);
#else
            updateMembers(ldapObject->m_memberList, attribute, entry, dn);
#endif
         }
         ldap_memfreeA(attribute);
      }
      ber_free(ber, 0);

      // entry is added only if it was of a correct type
      if (ldapObject->m_type == LDAP_USER && ldapObject->m_loginName != NULL)
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::fillLists(): User added: dn: %s, login name: %s, full name: %s, description: %s"), dn, ldapObject->m_loginName, CHECK_NULL(ldapObject->m_fullName), CHECK_NULL(ldapObject->m_description));
         m_userDnEntryList->set(dn, ldapObject);
         if(m_ldapUsreIdAttr[0] != 0 && ldapObject->m_id != NULL)
            m_userIdEntryList->set(ldapObject->m_id, ldapObject);
      }
      else if (ldapObject->m_type == LDAP_GROUP && ldapObject->m_loginName != NULL)
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::fillLists(): Group added: dn: %s, login name: %s, full name: %s, description: %s"), dn, ldapObject->m_loginName, CHECK_NULL(ldapObject->m_fullName), CHECK_NULL(ldapObject->m_description));
         m_groupDnEntryList->set(dn, ldapObject);
         if (m_ldapGroupIdAttr[0] != 0 && ldapObject->m_id != NULL)
            m_groupIdEntryList->set(ldapObject->m_id, ldapObject);
      }
      else
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::fillLists(): Unknown object is not added: dn: %s, login name: %s, full name: %s, description: %s"), dn, CHECK_NULL(ldapObject->m_loginName), CHECK_NULL(ldapObject->m_fullName), CHECK_NULL(ldapObject->m_description));
         delete ldapObject;
      }
      MemFree(dn);
   }
}

/**
 * Get attribute's value
 */
TCHAR *LDAPConnection::getAttrValue(LDAPMessage *entry, const char *attr, int index)
{
   TCHAR *result = NULL;
   berval **values = ldap_get_values_lenA(m_ldapConn, entry, (char *)attr);   // cast needed for Windows LDAP library
   if (static_cast<int>(ldap_count_values_len(values)) > index)
   {
#ifdef UNICODE
      result = WideStringFromUTF8String(values[index]->bv_val);
#else
      result = MBStringFromUTF8String(values[index]->bv_val);
#endif /* UNICODE */
   }
   ldap_value_free_len(values);
   return result;
}

/**
 * Get attribute's value
 */
TCHAR *LDAPConnection::getIdAttrValue(LDAPMessage *entry, const char *attr)
{
   BYTE hash[SHA256_DIGEST_SIZE];
   BYTE tmp[1024];
   memset(tmp, 0, 1024);
   berval **values = ldap_get_values_lenA(m_ldapConn, entry, (char *)attr);   // cast needed for Windows LDAP library
   int count = (int)ldap_count_values_len(values);
   int i, pos;
   for(i = 0, pos = 0; i < count; i++)
   {
      if (pos + values[i]->bv_len > 1024)
         break;
      memcpy(tmp + pos,values[i]->bv_val, values[i]->bv_len);
      pos += values[i]->bv_len;
   }
   ldap_value_free_len(values);
   if (i == 0)
      return MemCopyString(_T(""));

   CalculateSHA256Hash(tmp, pos, hash);
   TCHAR *result = MemAllocString(SHA256_DIGEST_SIZE * 2 + 1);
   BinToStr(hash, SHA256_DIGEST_SIZE, result);
   return result;
}

/**
 * Parse range information from attribute
 */
static void ParseRange(const char *attr, int *start, int *end)
{
   *end  = -1;
   *start = -1;

   const char *tmpS = strchr(attr, '=');
   if (tmpS == NULL)
      return;

   char *tmpAttr = MemCopyStringA(tmpS + 1);
   char *tmpE = strchr(tmpAttr, '-');
   if (tmpE == NULL)
   {
      MemFree(tmpAttr);
      return;
   }
   *tmpE = 0;
   tmpE++;
   *start = atoi(tmpAttr);
   if (tmpE[0] != '*')
   {
      *end = atoi(tmpE);
   }
   MemFree(tmpAttr);
}

/**
 * Update group members
 */
void LDAPConnection::updateMembers(StringSet *memberList, const char *firstAttr, LDAPMessage *firstEntry, const LDAP_CHAR *dn)
{
   int start, end;

   // get start, end member count
   ParseRange(firstAttr, &start, &end);

   // add received members
   int i = 0;
   TCHAR *value = getAttrValue(firstEntry, firstAttr, i);
   while(value != NULL)
   {
      nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::updateMembers(): member: %s"), value);
      memberList->addPreallocated(value);
      value = getAttrValue(firstEntry, firstAttr, ++i);
   }

   LDAPMessage *searchResult;
   LDAPMessage *entry;
   char *attribute;
   BerElement *ber;
   LDAP_CHAR *requiredAttr[2];
   LDAP_CHAR memberAttr[32];
   requiredAttr[1] = NULL;

   const LDAP_CHAR *filter = _TLDAP("(objectClass=*)");

   while(true)
   {
      // request next members
      struct ldap_timeval timeOut = { 10, 0 }; // 10 second connecion/search timeout
      ldap_snprintf(memberAttr, 32, _TLDAP("member;range=%d-*"), end + 1);
      requiredAttr[0] = memberAttr;
      nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::updateMembers(): request members id ") LDAP_TFMT _T(" group: ") LDAP_TFMT, dn, memberAttr);

      int rc = ldap_search_ext_s(
               m_ldapConn,		// LDAP session handle
               (LDAP_CHAR *)dn,	// Search Base
               LDAP_SCOPE_SUBTREE,	// Search Scope – everything below o=Acme
               (LDAP_CHAR *)filter, // Search Filter – only inetOrgPerson objects
               requiredAttr,	// returnAllAttributes – NULL means Yes
               0,		// attributesOnly – False means we want values
               NULL,	// Server controls – There are none
               NULL,	// Client controls – There are none
               &timeOut,	// search Timeout
               LDAP_NO_LIMIT,	// no size limit
               &searchResult);
      if (rc != LDAP_SUCCESS)
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("LDAPConnection::updateMembers(): LDAP could not get search results. Error code: %s"), getErrorString(rc).cstr());
         ldap_msgfree(searchResult);
         break;
      }

      bool found = false;
      nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::updateMembers(): Found entry count: %d"), ldap_count_entries(m_ldapConn, searchResult));
      for(entry = ldap_first_entry(m_ldapConn, searchResult); entry != NULL; entry = ldap_next_entry(m_ldapConn, entry))
      {
         for(attribute = ldap_first_attributeA(m_ldapConn, entry, &ber); attribute != NULL; attribute = ldap_next_attributeA(m_ldapConn, entry, ber))
         {
            if (!strncmp(attribute, "member;range=", 13))
            {
               // add received members
               i = 0;
               TCHAR *value = getAttrValue(entry, attribute, i);
               if(value != NULL)
               {
                  ParseRange(attribute, &start, &end);
                  found = true;
               }

               while(value != NULL)
               {
                  nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::updateMembers(): member: %s"), value);
                  memberList->addPreallocated(value);
                  value = getAttrValue(entry, attribute, ++i);
               }
            }
            ldap_memfreeA(attribute);
         }
         ber_free(ber, 0);
      }
      ldap_msgfree(searchResult);

      if (end == -1 || !found)
         break;

      if (start == -1)
      {
         nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::updateMembers(): member start interval returned as: %d"), start);
         break;
      }
   }
}

/**
 * Update user callback
 */
static EnumerationCallbackResult UpdateUserCallback(const TCHAR *key, const Entry *value, LDAPConnection *connection)
{
   UpdateLDAPUser(key, value);
   return _CONTINUE;
}

/**
 * Updates user list according to newly recievd user list
 */
void LDAPConnection::compareUserLists()
{
   m_userDnEntryList->forEach(UpdateUserCallback, this);
   RemoveDeletedLDAPEntries(m_userDnEntryList, m_userIdEntryList, m_action, true);
}

/**
 * Update group callback
 */
static EnumerationCallbackResult UpdateGroupCallback(const TCHAR *key, const Entry *value, LDAPConnection *connection)
{
   UpdateLDAPGroup(key, value);
   return _CONTINUE;
}

/**
 * Update group callback
 */
static EnumerationCallbackResult SyncGroupMembersCallback(const TCHAR *key, const Entry *value, LDAPConnection *connection)
{
   SyncLDAPGroupMembers(key, value);
   return _CONTINUE;
}

/**
 * Updates group list according to newly recievd user list
 */
void LDAPConnection::compareGroupList()
{
   m_groupDnEntryList->forEach(UpdateGroupCallback, this);
   RemoveDeletedLDAPEntries(m_groupDnEntryList, m_groupIdEntryList, m_action, false);
   m_groupDnEntryList->forEach(SyncGroupMembersCallback, this);
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
#ifdef _WIN32
   _tcslcpy(m_userDN, name, MAX_CONFIG_VALUE);
   _tcslcpy(m_userPassword, password, MAX_PASSWORD);
#else
   WideCharToMultiByte(CP_UTF8, 0, name, -1, m_userDN, MAX_CONFIG_VALUE, NULL, NULL);
   m_userDN[MAX_CONFIG_VALUE - 1] = 0;
   WideCharToMultiByte(CP_UTF8, 0, password, -1, m_userPassword, MAX_PASSWORD, NULL, NULL);
   m_userPassword[MAX_PASSWORD - 1] = 0;
#endif
#else
   strlcpy(m_userDN, name, MAX_CONFIG_VALUE);
   strlcpy(m_userPassword, password, MAX_PASSWORD);
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
   // Prevent empty password, bind against AD will succeed with
   // empty password by default.
   if (ldap_strlen(m_userPassword) == 0)
      return RCC_ACCESS_DENIED;

   if (m_ldapConn == NULL)
      return RCC_NO_LDAP_CONNECTION;

#ifdef _WIN32
   int rc = ldap_simple_bind_s(m_ldapConn, m_userDN, m_userPassword);
#else
   struct berval cred;
   cred.bv_val = m_userPassword;
   cred.bv_len = (int)strlen(m_userPassword);
   int rc = ldap_sasl_bind_s(m_ldapConn, m_userDN, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
#endif
   if (rc == LDAP_SUCCESS)
      return RCC_SUCCESS;

   nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::loginLDAP(): cannot login to LDAP server (%s)"), getErrorString(rc).cstr());
   return RCC_ACCESS_DENIED;
}

/**
 * Close LDAP connection
 */
void LDAPConnection::closeLDAPConnection()
{
   if (m_ldapConn != NULL)
   {
      nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::closeLDAPConnection(): disconnect form LDAP server"));
#ifdef _WIN32
      ldap_unbind_s(m_ldapConn);
#else
      ldap_unbind_ext(m_ldapConn, NULL, NULL);
#endif
      m_ldapConn = NULL;
   }
   else
   {
      nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::closeLDAPConnection(): connection already closed"));
   }
}

/**
 * Default LDAPConnection class constructor
 */
LDAPConnection::LDAPConnection()
{
   m_ldapConn = NULL;
   m_action = 1;
   m_secure = 0;
   m_pageSize = 1000;
   m_userIdEntryList = NULL;
   m_userDnEntryList = NULL;
   m_groupIdEntryList = NULL;
   m_groupDnEntryList = NULL;
}

/**
 * Destructor
 */
LDAPConnection::~LDAPConnection()
{
   closeLDAPConnection();
   delete m_userIdEntryList;
   delete m_userDnEntryList;
   delete m_groupIdEntryList;
   delete m_groupDnEntryList;
}

/**
 * Converts error string and returns in right format.
 */
String LDAPConnection::getErrorString(int ldap_error)
{
#ifdef _WIN32
  return String(ldap_err2string(ldap_error));
#else
#ifdef UNICODE
  WCHAR *wcs = WideStringFromUTF8String(ldap_err2string(ldap_error));
  String s(wcs);
  MemFree(wcs);
#else
  char *mbcs = MBStringFromUTF8String(ldap_err2string(ldap_error));
  String s(mbcs);
  MemFree(mbcs);
#endif
  return s;
#endif
}

#else	/* WITH_LDAP */

/**
 * Synchronize users - stub for server without LDAP support
 */
void LDAPConnection::syncUsers()
{
    nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::syncUsers(): FAILED - server was compiled without LDAP support"));
}

/**
 * Login via LDAP - stub for server without LDAP support
 */
UINT32 LDAPConnection::ldapUserLogin(const TCHAR *name, const TCHAR *password)
{
   nxlog_debug_tag(LDAP_DEBUG_TAG, 4, _T("LDAPConnection::ldapUserLogin(): FAILED - server was compiled without LDAP support"));
   return RCC_INTERNAL_ERROR;
}

#endif

/**
 * Get users, according to search line & insetr in netxms DB missing
 */
THREAD_RESULT THREAD_CALL SyncLDAPUsers(void *arg)
{
   ThreadSetName("LDAPSync");
   UINT32 syncInterval = ConfigReadInt(_T("LDAP.SyncInterval"), 0);
   if (syncInterval == 0)
   {
      nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("SyncLDAPUsers: sync thread will not start because LDAP sync is disabled"));
      return THREAD_OK;
   }

   nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("SyncLDAPUsers: sync thread started, interval %d minutes"), syncInterval);
   syncInterval *= 60;
   while(!SleepAndCheckForShutdown(syncInterval))
   {
      LDAPConnection conn;
      conn.syncUsers();
   }
   nxlog_debug_tag(LDAP_DEBUG_TAG, 1, _T("SyncLDAPUsers: sync thread stopped"));
   return THREAD_OK;
}

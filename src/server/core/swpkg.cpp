/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: swpkg.cpp
**
**/

#include "nxcore.h"

/**
 * Comparator for package names and versions
 */
int PackageNameVersionComparator(const SoftwarePackage **p1, const SoftwarePackage **p2)
{
   int rc = _tcscmp((*p1)->getName(), (*p2)->getName());
   if (rc == 0)
      rc = _tcscmp((*p1)->getVersion(), (*p2)->getVersion());
   return rc;
}

/**
 * Comparator for package names
 */
static int PackageNameComparator(const SoftwarePackage **p1, const SoftwarePackage **p2)
{
   return _tcscmp((*p1)->getName(), (*p2)->getName());
}

/**
 * Calculate package changes
 */
ObjectArray<SoftwarePackage> *CalculatePackageChanges(ObjectArray<SoftwarePackage> *oldSet, ObjectArray<SoftwarePackage> *newSet)
{
   ObjectArray<SoftwarePackage> *changes = new ObjectArray<SoftwarePackage>(32, 32);
   for(int i = 0; i < oldSet->size(); i++)
   {
      SoftwarePackage *p = oldSet->get(i);
      SoftwarePackage *np = newSet->bsearch(p, PackageNameComparator);
      if (np == nullptr)
      {
         p->setChangeCode(CHANGE_REMOVED);
         changes->add(p);
         continue;
      }

      if (!_tcscmp(p->getVersion(), np->getVersion()))
         continue;

      if (newSet->bsearch(p, PackageNameVersionComparator) != nullptr)
         continue;

      // multiple versions of same package could be installed
      // (example is gpg-pubkey package on RedHat)
      // if this is the case consider all version changes
      // to be either install or remove
      SoftwarePackage *prev = oldSet->get(i - 1);
      SoftwarePackage *next = oldSet->get(i + 1);
      bool multipleVersions =
               ((prev != nullptr) && !_tcscmp(prev->getName(), p->getName())) ||
               ((next != nullptr) && !_tcscmp(next->getName(), p->getName()));

      if (multipleVersions)
      {
         p->setChangeCode(CHANGE_REMOVED);
      }
      else
      {
         p->setChangeCode(CHANGE_UPDATED);
         np->setChangeCode(CHANGE_UPDATED);
         changes->add(np);
      }
      changes->add(p);
   }

   // Check for new packages
   for(int i = 0; i < newSet->size(); i++)
   {
      SoftwarePackage *p = newSet->get(i);
      if (p->getChangeCode() == CHANGE_UPDATED)
         continue;   // already marked as upgrade for some existing package
      if (oldSet->bsearch(p, PackageNameVersionComparator) != NULL)
         continue;

      p->setChangeCode(CHANGE_ADDED);
      changes->add(p);
   }

   return changes;
}

/**
 * Extract value from source table cell
 */
#define EXTRACT_VALUE(name, field) \
	{ \
		if (!_tcsicmp(table.getColumnName(i), _T(name))) \
		{ \
			const TCHAR *value = table.getAsString(row, i); \
			pkg->field = MemCopyString(value); \
			continue; \
		} \
	}

/**
 * Create from agent table
 *
 * @param table table received from agent
 * @param row row number in a table
 * @return new object on success, NULL on parse error
 */
SoftwarePackage *SoftwarePackage::createFromTableRow(const Table& table, int row)
{
   SoftwarePackage *pkg = new SoftwarePackage();
   for(int i = 0; i < table.getNumColumns(); i++)
   {
      EXTRACT_VALUE("NAME", m_name);
      EXTRACT_VALUE("VERSION", m_version);
      EXTRACT_VALUE("VENDOR", m_vendor);
      EXTRACT_VALUE("URL", m_url);
      EXTRACT_VALUE("DESCRIPTION", m_description);
      EXTRACT_VALUE("UNINSTALL_KEY", m_uninstallKey);

      if (!_tcsicmp(table.getColumnName(i), _T("DATE")))
         pkg->m_date = table.getAsUInt(row, i);
   }

   if (pkg->m_name == nullptr)
   {
      delete pkg;
      return nullptr;
   }

   if ((pkg->m_version == nullptr) || (*pkg->m_version == 0))
   {
      MemFree(pkg->m_version);
      pkg->m_version = MemCopyString(_T("unset"));
   }

   return pkg;
}

/**
 * Constructor
 *
 * @param table table received from agent
 * @param row row number in a table
 */
SoftwarePackage::SoftwarePackage()
{
	m_name = nullptr;
	m_version = nullptr;
	m_vendor = nullptr;
	m_date = 0;
	m_url = nullptr;
	m_description = nullptr;
	m_uninstallKey = nullptr;
	m_changeCode = CHANGE_NONE;
}

/**
 * Constructor to load from database
 *
 * @param database query result
 * @param row row number
 */
SoftwarePackage::SoftwarePackage(DB_RESULT result, int row)
{
   m_name = DBGetField(result, row, 0, nullptr, 0);
   m_version = DBGetField(result, row, 1, nullptr, 0);
   m_vendor = DBGetField(result, row, 2, nullptr, 0);
   m_date = (time_t)DBGetFieldULong(result, row, 3);
   m_url = DBGetField(result, row, 4, nullptr, 0);
   m_description = DBGetField(result, row, 5, nullptr, 0);
   m_uninstallKey = DBGetField(result, row, 6, nullptr, 0);
   m_changeCode = CHANGE_NONE;
}

/**
 * Copy constructor
 */
SoftwarePackage::SoftwarePackage(const SoftwarePackage& src)
{
   m_name = MemCopyString(src.m_name);
   m_version = MemCopyString(src.m_version);
   m_vendor = MemCopyString(src.m_vendor);
   m_date = src.m_date;
   m_url = MemCopyString(src.m_url);
   m_description = MemCopyString(src.m_description);
   m_uninstallKey = MemCopyString(src.m_uninstallKey);
   m_changeCode = src.m_changeCode;
}

/**
 * Destructor
 */
SoftwarePackage::~SoftwarePackage()
{
	MemFree(m_name);
	MemFree(m_version);
	MemFree(m_vendor);
	MemFree(m_url);
	MemFree(m_description);
   MemFree(m_uninstallKey);
}

/**
 * Assignment operator
 */
SoftwarePackage& SoftwarePackage::operator=(const SoftwarePackage& src)
{
   MemFree(m_name);
   MemFree(m_version);
   MemFree(m_vendor);
   MemFree(m_url);
   MemFree(m_description);
   MemFree(m_uninstallKey);

   m_name = MemCopyString(src.m_name);
   m_version = MemCopyString(src.m_version);
   m_vendor = MemCopyString(src.m_vendor);
   m_date = src.m_date;
   m_url = MemCopyString(src.m_url);
   m_description = MemCopyString(src.m_description);
   m_uninstallKey = MemCopyString(src.m_uninstallKey);
   return *this;
}

/**
 * Fill NXCP message with package data
 */
void SoftwarePackage::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   uint32_t fieldId = baseId;
	msg->setField(fieldId++, CHECK_NULL_EX(m_name));
	msg->setField(fieldId++, CHECK_NULL_EX(m_version));
	msg->setField(fieldId++, CHECK_NULL_EX(m_vendor));
	msg->setField(fieldId++, static_cast<uint32_t>(m_date));
	msg->setField(fieldId++, CHECK_NULL_EX(m_url));
	msg->setField(fieldId++, CHECK_NULL_EX(m_description));
   msg->setField(fieldId++, CHECK_NULL_EX(m_uninstallKey));
}

/**
 * Serialize to JSON
 */
json_t *SoftwarePackage::toJson() const
{
   json_t *root = json_object();

   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "version", json_string_t(m_version));
   json_object_set_new(root, "vendor", json_string_t(m_vendor));
   json_object_set_new(root, "date", json_integer(static_cast<json_int_t>(m_date)));
   json_object_set_new(root, "url", json_string_t(m_url));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "uninstallKey", json_string_t(m_uninstallKey));

   return root;
}

/**
 * Save software package data to database
 */
bool SoftwarePackage::saveToDatabase(DB_STATEMENT hStmt) const
{
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC, 127);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_version, DB_BIND_STATIC, 63);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_STATIC, 63);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_date));
   DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_url, DB_BIND_STATIC, 255);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, 255);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_uninstallKey, DB_BIND_STATIC, 255);
   return DBExecute(hStmt);
}

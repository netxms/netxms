/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: images.cpp
**
**/

#include "webapi.h"

/**
 * Notify connected client sessions about an image library change.
 */
static void NotifyImageLibraryChange(const uuid& guid, bool removed)
{
   EnumerateClientSessions([guid, removed](ClientSession *session) {
      session->onLibraryImageChange(guid, removed);
   });
}

/**
 * Build JSON metadata document for image library entry.
 */
static json_t *BuildImageMetadata(const uuid& guid, const wchar_t *name, const wchar_t *category, const wchar_t *mimeType, bool isProtected)
{
   json_t *output = json_object();
   json_object_set_new(output, "guid", guid.toJson());
   json_object_set_new(output, "name", json_string_t(name));
   json_object_set_new(output, "category", json_string_t(category));
   json_object_set_new(output, "mimeType", json_string_t(mimeType));
   json_object_set_new(output, "isProtected", json_boolean(isProtected));
   return output;
}

/**
 * Handler for GET /v1/image-library
 * Returns list of all images in the library (metadata only).
 * Optional query parameter: category (filter by category)
 */
int H_ImageLibrary(Context *context)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   StringBuffer query(L"SELECT guid,name,category,mimetype,protected FROM images");
   const char *categoryFilter = context->getQueryParameter("category");
   if ((categoryFilter != nullptr) && (*categoryFilter != 0))
   {
      wchar_t categoryW[256];
      utf8_to_wchar(categoryFilter, -1, categoryW, 256);
      query.append(L" WHERE category=");
      query.append(DBPrepareString(hdb, categoryW));
   }
   query.append(L" ORDER BY name");

   DB_RESULT result = DBSelect(hdb, query);
   if (result == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   int count = DBGetNumRows(result);
   json_t *output = json_array();
   wchar_t name[256], category[256], mimeType[256];
   for (int i = 0; i < count; i++)
   {
      uuid guid = DBGetFieldGUID(result, i, 0);
      DBGetField(result, i, 1, name, 256);
      DBGetField(result, i, 2, category, 256);
      DBGetField(result, i, 3, mimeType, 256);
      bool isProtected = DBGetFieldLong(result, i, 4) != 0;
      json_array_append_new(output, BuildImageMetadata(guid, name, category, mimeType, isProtected));
   }

   DBFreeResult(result);
   DBConnectionPoolReleaseConnection(hdb);

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/image-library/:guid
 * Returns image metadata as JSON.
 */
int H_ImageLibraryDetails(Context *context)
{
   const TCHAR *guidText = context->getPlaceholderValue(L"guid");
   if (guidText == nullptr)
      return 400;

   uuid guid = uuid::parse(guidText);
   if (guid.isNull())
      return 400;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   TCHAR query[256];
   _sntprintf(query, 256, L"SELECT name,category,mimetype,protected FROM images WHERE guid='%s'", guidText);
   DB_RESULT result = DBSelect(hdb, query);
   if (result == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   if (DBGetNumRows(result) == 0)
   {
      DBFreeResult(result);
      DBConnectionPoolReleaseConnection(hdb);
      return 404;
   }

   wchar_t name[256], category[256], mimeType[256];
   DBGetField(result, 0, 0, name, 256);
   DBGetField(result, 0, 1, category, 256);
   DBGetField(result, 0, 2, mimeType, 256);
   bool isProtected = DBGetFieldLong(result, 0, 3) != 0;
   json_t *output = BuildImageMetadata(guid, name, category, mimeType, isProtected);

   DBFreeResult(result);
   DBConnectionPoolReleaseConnection(hdb);

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/image-library/:guid/data
 * Returns the raw image binary data with correct Content-Type.
 */
int H_ImageLibraryData(Context *context)
{
   const TCHAR *guidText = context->getPlaceholderValue(L"guid");
   if (guidText == nullptr)
      return 400;

   uuid guid = uuid::parse(guidText);
   if (guid.isNull())
      return 400;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT mimetype,image_data FROM images WHERE guid=?");
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);

   DB_RESULT result = DBSelectPrepared(hStmt);
   if (result == nullptr)
   {
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }
   if (DBGetNumRows(result) == 0)
   {
      DBFreeResult(result);
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      return 404;
   }

   wchar_t mimeType[64];
   DBGetField(result, 0, 0, mimeType, 64);
   size_t dataSize = 0;
   BYTE *data = DBGetFieldBinary(result, 0, 1, &dataSize);
   DBFreeResult(result);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   if ((data == nullptr) || (dataSize == 0))
   {
      MemFree(data);
      return 404;
   }

   char mimeTypeA[64];
   tchar_to_utf8(mimeType, -1, mimeTypeA, 64);
   context->setResponseData(data, dataSize, mimeTypeA);
   MemFree(data);
   return 200;
}

/**
 * Sanitize an SVG payload by staging through {datadir}/images/{guid}, running
 * SanitizeSVGFile on it, and reloading the cleaned bytes. Returns a freshly
 * allocated buffer (caller MemFree's) and writes its length to *outSize, or
 * nullptr on failure. Non-SVG inputs should not call this.
 */
static BYTE *SanitizeSVGPayload(const wchar_t *guidText, const void *data, size_t size, size_t *outSize)
{
   wchar_t staging[MAX_PATH];
   _sntprintf(staging, MAX_PATH, L"%s" DDIR_IMAGES FS_PATH_SEPARATOR L"%s", g_netxmsdDataDir, guidText);
   if (SaveFile(staging, data, size) != SaveFileStatus::SUCCESS)
      return nullptr;
   SanitizeSVGFile(staging);
   BYTE *sanitized = LoadFile(staging, outSize);
   _wremove(staging);
   return sanitized;
}

/**
 * Handler for POST /v1/image-library
 * Creates a new image library entry. The request body is the raw image bytes,
 * Content-Type header becomes the stored MIME type. Image name is passed via
 * "name" query parameter (required); "category" query parameter is optional
 * (defaults to "Default").
 */
int H_ImageLibraryCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
      return 403;

   const char *nameUtf8 = context->getQueryParameter("name");
   if ((nameUtf8 == nullptr) || (*nameUtf8 == 0))
   {
      context->setErrorResponse("Missing 'name' query parameter");
      return 400;
   }

   if (!context->hasRequestData())
   {
      context->setErrorResponse("Request body must contain image data");
      return 400;
   }

   const char *mimeTypeUtf8 = context->getRequestHeader("Content-Type");
   if ((mimeTypeUtf8 == nullptr) || (*mimeTypeUtf8 == 0))
      mimeTypeUtf8 = "image/unknown";

   const char *categoryUtf8 = context->getQueryParameter("category");
   if ((categoryUtf8 == nullptr) || (*categoryUtf8 == 0))
      categoryUtf8 = "Default";

   wchar_t name[MAX_OBJECT_NAME], category[MAX_OBJECT_NAME], mimeType[MAX_DB_STRING];
   utf8_to_wchar(nameUtf8, -1, name, MAX_OBJECT_NAME);
   utf8_to_wchar(categoryUtf8, -1, category, MAX_OBJECT_NAME);
   utf8_to_wchar(mimeTypeUtf8, -1, mimeType, MAX_DB_STRING);

   uuid guid = uuid::generate();
   wchar_t guidText[64];
   guid.toString(guidText);

   const void *imageBytes = context->getRequestData();
   size_t imageSize = context->getRequestDataSize();
   BYTE *sanitizedBuf = nullptr;
   if (!wcsicmp(mimeType, L"image/svg+xml"))
   {
      sanitizedBuf = SanitizeSVGPayload(guidText, imageBytes, imageSize, &imageSize);
      if (sanitizedBuf == nullptr)
      {
         context->setErrorResponse("Failed to sanitize image");
         return 500;
      }
      imageBytes = sanitizedBuf;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO images (guid,name,category,mimetype,protected,image_data) VALUES (?,?,?,?,0,?)");
   if (hStmt == nullptr)
   {
      MemFree(sanitizedBuf);
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, category, DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, mimeType, DB_BIND_STATIC, 32);
   DBBind(hStmt, 5, DB_SQLTYPE_TEXT, imageBytes, imageSize, (sanitizedBuf != nullptr) ? DB_BIND_DYNAMIC : DB_BIND_STATIC);
   bool dbSuccess = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   if (!dbSuccess)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   NotifyImageLibraryChange(guid, false);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Library image \"%s\" [%s] created via REST API", name, guidText);

   json_t *output = BuildImageMetadata(guid, name, category, mimeType, false);
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for PUT /v1/image-library/:guid
 * Updates an existing image. If the request body is non-empty the binary data
 * (and MIME type derived from Content-Type) are replaced. "name" and "category"
 * query parameters optionally update metadata. Protected images cannot be
 * modified.
 */
int H_ImageLibraryUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
      return 403;

   const wchar_t *guidText = context->getPlaceholderValue(L"guid");
   if (guidText == nullptr)
      return 400;

   uuid guid = uuid::parse(guidText);
   if (guid.isNull())
      return 400;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[MAX_DB_STRING];
   _sntprintf(query, MAX_DB_STRING, L"SELECT name,category,mimetype,protected FROM images WHERE guid='%s'", guidText);
   DB_RESULT result = DBSelect(hdb, query);
   if (result == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }
   if (DBGetNumRows(result) == 0)
   {
      DBFreeResult(result);
      DBConnectionPoolReleaseConnection(hdb);
      return 404;
   }

   wchar_t name[MAX_OBJECT_NAME], category[MAX_OBJECT_NAME], mimeType[MAX_DB_STRING];
   DBGetField(result, 0, 0, name, MAX_OBJECT_NAME);
   DBGetField(result, 0, 1, category, MAX_OBJECT_NAME);
   DBGetField(result, 0, 2, mimeType, MAX_DB_STRING);
   bool isProtected = DBGetFieldLong(result, 0, 3) != 0;
   DBFreeResult(result);

   if (isProtected)
   {
      DBConnectionPoolReleaseConnection(hdb);
      context->setErrorResponse("Image is protected and cannot be modified");
      return 403;
   }

   const char *newNameUtf8 = context->getQueryParameter("name");
   if ((newNameUtf8 != nullptr) && (*newNameUtf8 != 0))
      utf8_to_wchar(newNameUtf8, -1, name, MAX_OBJECT_NAME);

   const char *newCategoryUtf8 = context->getQueryParameter("category");
   if ((newCategoryUtf8 != nullptr) && (*newCategoryUtf8 != 0))
      utf8_to_wchar(newCategoryUtf8, -1, category, MAX_OBJECT_NAME);

   bool hasBody = context->hasRequestData();
   if (hasBody)
   {
      const char *newMimeUtf8 = context->getRequestHeader("Content-Type");
      if ((newMimeUtf8 != nullptr) && (*newMimeUtf8 != 0))
         utf8_to_wchar(newMimeUtf8, -1, mimeType, MAX_DB_STRING);
   }

   const void *imageBytes = nullptr;
   size_t imageSize = 0;
   BYTE *sanitizedBuf = nullptr;
   if (hasBody)
   {
      imageBytes = context->getRequestData();
      imageSize = context->getRequestDataSize();
      if (!wcsicmp(mimeType, L"image/svg+xml"))
      {
         sanitizedBuf = SanitizeSVGPayload(guidText, imageBytes, imageSize, &imageSize);
         if (sanitizedBuf == nullptr)
         {
            DBConnectionPoolReleaseConnection(hdb);
            context->setErrorResponse("Failed to sanitize image");
            return 500;
         }
         imageBytes = sanitizedBuf;
      }
   }

   DB_STATEMENT hStmt = hasBody
      ? DBPrepare(hdb, L"UPDATE images SET name=?,category=?,mimetype=?,image_data=? WHERE guid=? AND protected=0")
      : DBPrepare(hdb, L"UPDATE images SET name=?,category=?,mimetype=? WHERE guid=? AND protected=0");
   if (hStmt == nullptr)
   {
      MemFree(sanitizedBuf);
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, category, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, mimeType, DB_BIND_STATIC, 32);
   if (hasBody)
   {
      DBBind(hStmt, 4, DB_SQLTYPE_TEXT, imageBytes, imageSize, (sanitizedBuf != nullptr) ? DB_BIND_DYNAMIC : DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, guid);
   }
   else
   {
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, guid);
   }
   bool dbSuccess = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   if (!dbSuccess)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   NotifyImageLibraryChange(guid, false);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Library image \"%s\" [%s] modified via REST API", name, guidText);

   json_t *output = BuildImageMetadata(guid, name, category, mimeType, false);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/image-library/:guid
 * Removes an image library entry. Protected images cannot be deleted.
 */
int H_ImageLibraryDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
      return 403;

   const wchar_t *guidText = context->getPlaceholderValue(L"guid");
   if (guidText == nullptr)
      return 400;

   uuid guid = uuid::parse(guidText);
   if (guid.isNull())
      return 400;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[MAX_DB_STRING];
   _sntprintf(query, MAX_DB_STRING, L"SELECT name,protected FROM images WHERE guid='%s'", guidText);
   DB_RESULT result = DBSelect(hdb, query);
   if (result == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }
   if (DBGetNumRows(result) == 0)
   {
      DBFreeResult(result);
      DBConnectionPoolReleaseConnection(hdb);
      return 404;
   }

   wchar_t name[MAX_OBJECT_NAME];
   DBGetField(result, 0, 0, name, MAX_OBJECT_NAME);
   bool isProtected = DBGetFieldLong(result, 0, 1) != 0;
   DBFreeResult(result);

   if (isProtected)
   {
      DBConnectionPoolReleaseConnection(hdb);
      context->setErrorResponse("Image is protected and cannot be deleted");
      return 403;
   }

   _sntprintf(query, MAX_DB_STRING, L"DELETE FROM images WHERE guid='%s' AND protected=0", guidText);
   bool dbSuccess = DBQuery(hdb, query);
   DBConnectionPoolReleaseConnection(hdb);

   if (!dbSuccess)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   NotifyImageLibraryChange(guid, true);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Library image \"%s\" [%s] deleted via REST API", name, guidText);

   return 204;
}

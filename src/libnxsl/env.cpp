/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: env.cpp
**
**/

#include "libnxsl.h"
#include <netxms-version.h>

/**
 * Externals
 */
int F_abs(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_acos(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_asin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_assert(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_atan(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_atan2(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ceil(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_chr(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_classof(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_cos(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_cosh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_d2x(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_exit(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_exp(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_gethostbyaddr(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_gethostbyname(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_floor(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_format(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_gmtime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_index(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_inList(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_left(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_length(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_localtime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_log(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_log10(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_lower(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ltrim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_max(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_md5(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_min(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_mktime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ord(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_pow(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_print(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_println(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_random(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_replace(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_right(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_rindex(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_round(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_rtrim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_sha1(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_sha256(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_sin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_sinh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_sleep(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_sqrt(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_strftime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_substr(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_tan(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_tanh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_time(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_TIME(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_trace(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_trim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_typeof(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_upper(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_weierstrass(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_x2d(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_AddrInRange(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_AddrInSubnet(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ArrayToString(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_Base64Decode(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_Base64Encode(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ByteStream(int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM* vm);
int F_FormatMetricPrefix(int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM* vm);
int F_GeoLocation(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GetCurrentTimeMs(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GetThreadPoolNames(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_InetAddress(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_JsonArray(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_JsonObject(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_JsonParse(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MacAddress(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ReadPersistentStorage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_SecondsToUptime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_SplitString(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_Table(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_WritePersistentStorage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int F_CopyFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_CreateDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_DeleteFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_FileAccess(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_OpenFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_RemoveDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_RenameFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int S_max(const NXSL_Identifier& name, NXSL_Value *options, int argc, NXSL_Value **argv, int *selection, NXSL_VM *vm);
int S_min(const NXSL_Identifier& name, NXSL_Value *options, int argc, NXSL_Value **argv, int *selection, NXSL_VM *vm);

/**
 * Placeholder for __invoke function - will only be called if no arguments provided or via indirect call
 */
static int F_invoke(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return (argc == 0) ? NXSL_ERR_INVALID_ARGUMENT_COUNT : NXSL_ERR_TOO_MANY_NESTED_CALLS;
}

/**
 * Default built-in function list
 */
static NXSL_ExtFunction s_builtinFunctions[] =
{
   { "__invoke", F_invoke, -1 },
   { "__new@ByteStream", F_ByteStream, 0 },
   { "__new@GeoLocation", F_GeoLocation, -1 },
   { "__new@InetAddress", F_InetAddress, -1 },
   { "__new@JsonArray", F_JsonArray, -1 },
   { "__new@JsonObject", F_JsonObject, -1 },
   { "__new@MacAddress", F_MacAddress, -1 },
   { "__new@Table", F_Table, 0 },
   { "__new@TIME", F_TIME, 0 },
	{ "_exit", F_exit, -1 },
   { "abs", F_abs, 1 },
   { "asin", F_asin, 1 },
   { "acos", F_acos, 1 },
   { "assert", F_assert, -1 },
   { "atan", F_atan, 1 },
   { "atan2", F_atan2, 2 },
   { "ceil", F_ceil, 1 },
   { "chr", F_chr, 1 },
   { "classof", F_classof, 1 },
   { "cos", F_cos, 1 },
   { "cosh", F_cosh, 1 },
	{ "d2x", F_d2x, -1 },
   { "exp", F_exp, 1 },
   { "gethostbyaddr", F_gethostbyaddr, 1 },
   { "gethostbyname", F_gethostbyname, -1 },
   { "floor", F_floor, 1 },
   { "format", F_format, -1 },
   { "gmtime", F_gmtime, -1 },
   { "index", F_index, -1 },
   { "inList", F_inList, 3 },
   { "left", F_left, -1 },
   { "length", F_length, 1 },
   { "localtime", F_localtime, -1 },
   { "log", F_log, 1 },
   { "log10", F_log10, 1 },
   { "lower", F_lower, 1 },
	{ "ltrim", F_ltrim, 1 },
   { "max", F_max, -1 },
   { "md5", F_md5, 1 },
   { "min", F_min, -1 },
   { "mktime", F_mktime, 1 },
   { "ord", F_ord, 1 },
   { "pow", F_pow, 2 },
   { "print", F_print, -1 },
   { "println", F_println, -1 },
   { "random", F_random, 2 },
   { "replace", F_replace, 3 },
   { "right", F_right, -1 },
   { "rindex", F_rindex, -1 },
   { "round", F_round, -1 },
	{ "rtrim", F_rtrim, 1 },
	{ "sha1", F_sha1, 1 },
	{ "sha256", F_sha256, 1 },
   { "sin", F_sin, 1 },
   { "sinh", F_sinh, 1 },
	{ "sleep", F_sleep, 1 },
   { "sqrt", F_sqrt, 1 },
	{ "strftime", F_strftime, -1 },
	{ "substr", F_substr, -1 },
   { "tan", F_tan, 1 },
   { "tanh", F_tanh, 1 },
	{ "time", F_time, 0 },
   { "trace", F_trace, 2 },
	{ "trim", F_trim, 1 },
   { "typeof", F_typeof, 1 },
   { "upper", F_upper, 1 },
   { "weierstrass", F_weierstrass, 3 },
   { "x2d", F_x2d, 1 },
   { "AddrInRange", F_AddrInRange, 3 },
   { "AddrInSubnet", F_AddrInSubnet, 3 },
   { "ArrayToString", F_ArrayToString, 2 },
   { "Base64Decode", F_Base64Decode, -1 },
   { "Base64Encode", F_Base64Encode, -1 },
   { "FormatMetricPrefix", F_FormatMetricPrefix, -1 },
   { "GetCurrentTimeMs", F_GetCurrentTimeMs, 0 },
   { "GetThreadPoolNames", F_GetThreadPoolNames, 0 },
   { "JsonParse", F_JsonParse, 1 },
   { "ReadPersistentStorage", F_ReadPersistentStorage, 1 },
	{ "SecondsToUptime", F_SecondsToUptime, 1 },
   { "SplitString", F_SplitString, 2 },
   { "WritePersistentStorage", F_WritePersistentStorage, 2 }
};

/**
 * I/O and file management functions
 */
static NXSL_ExtFunction s_ioFunctions[] =
{
   { "CopyFile", F_CopyFile, 2 },
   { "CreateDirectory", F_CreateDirectory, 1 },
   { "DeleteFile", F_DeleteFile, 1 },
   { "FileAccess", F_FileAccess, 2 },
   { "OpenFile", F_OpenFile, -1 },
   { "RemoveDirectory", F_RemoveDirectory, 1 },
   { "RenameFile", F_RenameFile, 2 }
};

/**
 * Default built-in selector list
 */
static NXSL_ExtSelector s_builtinSelectors[] =
{
   { "max", S_max },
   { "min", S_min }
};

/**
 * Constructor
 */
NXSL_Environment::NXSL_Environment() : m_metadata(1024)
{
   m_functions = createFunctionListRef(s_builtinFunctions, sizeof(s_builtinFunctions) / sizeof(NXSL_ExtFunction));
   m_selectors = createSelectorListRef(s_builtinSelectors, sizeof(s_builtinSelectors) / sizeof(NXSL_ExtSelector));
   m_library = nullptr;
}

/**
 * Destructor
 */
NXSL_Environment::~NXSL_Environment()
{
}

/**
 * Find function by name
 */
const NXSL_ExtFunction *NXSL_Environment::findFunction(const NXSL_Identifier& name) const
{
   for(NXSL_EnvironmentListRef<NXSL_ExtFunction> *list = m_functions; list != nullptr; list = list->next)
   {
      for(size_t i = 0; i < list->count; i++)
         if (!strcmp(list->elements[i].m_name, name.value))
            return &list->elements[i];
   }
   return nullptr;
}

/**
 * Get all available functions
 */
StringSet *NXSL_Environment::getAllFunctions() const
{
   StringSet *functions = new StringSet();
   for(NXSL_EnvironmentListRef<NXSL_ExtFunction> *list = m_functions; list != nullptr; list = list->next)
   {
      for(size_t i = 0; i < list->count; i++)
      {
#ifdef UNICODE
         functions->addPreallocated(WideStringFromUTF8String(list->elements[i].m_name));
#else
         functions->add(list->elements[i].m_name);
#endif
      }
   }
   return functions;
}

/**
 * Register function set
 */
void NXSL_Environment::registerFunctionSet(size_t count, const NXSL_ExtFunction *list)
{
   NXSL_EnvironmentListRef<NXSL_ExtFunction> *ref = createFunctionListRef(list, count);
   ref->next = m_functions;
   m_functions = ref;
}

/**
 * Register I/O and file management functions
 */
void NXSL_Environment::registerIOFunctions()
{
   registerFunctionSet(sizeof(s_ioFunctions) / sizeof(NXSL_ExtFunction), s_ioFunctions);
}

/**
 * Find selector by name
 */
const NXSL_ExtSelector *NXSL_Environment::findSelector(const NXSL_Identifier& name) const
{
   for(NXSL_EnvironmentListRef<NXSL_ExtSelector> *list = m_selectors; list != nullptr; list = list->next)
   {
      for(size_t i = 0; i < list->count; i++)
         if (!strcmp(list->elements[i].m_name, name.value))
            return &list->elements[i];
   }
   return nullptr;
}

/**
 * Register selector set
 */
void NXSL_Environment::registerSelectorSet(size_t count, const NXSL_ExtSelector *list)
{
   NXSL_EnvironmentListRef<NXSL_ExtSelector> *ref = createSelectorListRef(list, count);
   ref->next = m_selectors;
   m_selectors = ref;
}

/**
 * Load module into VM
 */
bool NXSL_Environment::loadModule(NXSL_VM *vm, const NXSL_ModuleImport *importInfo)
{
   bool success = false;

   // First, try to find module in library
   if (m_library != nullptr)
   {
      // Wildcard import
      if (importInfo->flags & MODULE_IMPORT_WILDCARD)
      {
         success = true;
         m_library->forEach(
            [vm, importInfo, &success] (const NXSL_LibraryScript *e) -> void
            {
               if (success && e->isValid() && MatchString(importInfo->name, e->getName(), false))
               {
                  NXSL_ModuleImport module;
                  _tcslcpy(module.name, e->getName(), MAX_IDENTIFIER_LENGTH);
                  module.lineNumber = importInfo->lineNumber;
                  module.flags = importInfo->flags & ~MODULE_IMPORT_WILDCARD;
                  success = vm->loadModule(e->getProgram(), &module);
               }
            });
      }
      else
      {
         NXSL_Program *libraryModule = m_library->findNxslProgram(importInfo->name);
         if (libraryModule != nullptr)
         {
            success = vm->loadModule(libraryModule, importInfo);
         }
      }
   }

   // If failed, try to load it from file
   if (!success && !(importInfo->flags & MODULE_IMPORT_WILDCARD))
   {
      TCHAR fileName[MAX_PATH];
      _sntprintf(fileName, MAX_PATH, _T("%s.nxsl"), importInfo->name);
      TCHAR *source = NXSLLoadFile(fileName);
      if (source != nullptr)
      {
         NXSL_Program *libraryModule = NXSLCompile(source, nullptr, 0, nullptr, this);
         if (libraryModule != nullptr)
         {
            success = vm->loadModule(libraryModule, importInfo);
            delete libraryModule;
         }
         MemFree(source);
      }
   }

   return success || (importInfo->flags & MODULE_IMPORT_OPTIONAL);
}

/**
 * Write trace message
 * Default implementation does nothing
 */
void NXSL_Environment::trace(int level, const TCHAR *text)
{
}

/**
 * Print text to standard output
 * Default implementation writes value as string to stdout, if it is set
 */
void NXSL_Environment::print(const TCHAR *text)
{
   if (text != nullptr)
	{
      WriteToTerminal(text);
	}
   else
	{
      WriteToTerminal(_T("(null)"));
	}
}

/**
 * Additional VM configuration
 */
void NXSL_Environment::configureVM(NXSL_VM *vm)
{
}

/**
 * Get constant value known to this environment
 */
NXSL_Value *NXSL_Environment::getConstantValue(const NXSL_Identifier& name, NXSL_ValueManager *vm)
{
   if (name.value[0] == 'M')
   {
      NXSL_ENV_CONSTANT("MacAddressNotation::BYTEPAIR_COLON_SEPARATED", static_cast<int32_t>(MacAddressNotation::BYTEPAIR_COLON_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::BYTEPAIR_DOT_SEPARATED", static_cast<int32_t>(MacAddressNotation::BYTEPAIR_DOT_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::COLON_SEPARATED", static_cast<int32_t>(MacAddressNotation::COLON_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::DECIMAL_DOT_SEPARATED", static_cast<int32_t>(MacAddressNotation::DECIMAL_DOT_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::DOT_SEPARATED", static_cast<int32_t>(MacAddressNotation::DOT_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::FLAT_STRING", static_cast<int32_t>(MacAddressNotation::FLAT_STRING));
      NXSL_ENV_CONSTANT("MacAddressNotation::HYPHEN_SEPARATED", static_cast<int32_t>(MacAddressNotation::HYPHEN_SEPARATED));
   }

   if (name.value[0] == 'N')
   {
      NXSL_ENV_CONSTANT("NXSL::BuildTag", NETXMS_BUILD_TAG);
#if WORDS_BIGENDIAN
      NXSL_ENV_CONSTANT("NXSL::SystemIsBigEndian", 1);
#else
      NXSL_ENV_CONSTANT("NXSL::SystemIsBigEndian", 0);
#endif
      NXSL_ENV_CONSTANT("NXSL::Version", NETXMS_VERSION_STRING);
   }

   if (name.value[0] == 'S')
   {
      NXSL_ENV_CONSTANT("SeekOrigin::Begin", SEEK_SET);
      NXSL_ENV_CONSTANT("SeekOrigin::Current", SEEK_CUR);
      NXSL_ENV_CONSTANT("SeekOrigin::End", SEEK_END);
   }

   return nullptr;
}

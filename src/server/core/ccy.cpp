/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: ccy.cpp
**
**/

#include "nxcore.h"
#include <math.h>

/**
 * Country information
 */
struct COUNTRY_INFO
{
   TCHAR numericCode[4];
   TCHAR alphaCode[3];
   TCHAR alpha3Code[4];
   TCHAR *name;
};

/**
 * Currency information
 */
struct CURRENCY_INFO
{
   TCHAR numericCode[4];
   TCHAR alphaCode[4];
   TCHAR *description;
   int exponent;
   double divisor;
};

/**
 * Country list
 */
static int s_countryListSize = 0;
static COUNTRY_INFO *s_countryList = NULL;

/**
 * Currency list
 */
static int s_currencyListSize = 0;
static CURRENCY_INFO *s_currencyList = NULL;

/**
 * Initialize country list
 */
void InitCountryList()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT numeric_code,alpha_code,alpha3_code,name FROM country_codes"));
   if (hResult != NULL)
   {
      s_countryListSize = DBGetNumRows(hResult);
      if (s_countryListSize > 0)
      {
         s_countryList = (COUNTRY_INFO *)malloc(sizeof(COUNTRY_INFO) * s_countryListSize);
         for(int i = 0; i < s_countryListSize; i++)
         {
            DBGetField(hResult, i, 0, s_countryList[i].numericCode, 4);
            DBGetField(hResult, i, 1, s_countryList[i].alphaCode, 3);
            DBGetField(hResult, i, 2, s_countryList[i].alpha3Code, 4);
            s_countryList[i].name = DBGetField(hResult, i, 3, NULL, 0);
         }
      }
      DBFreeResult(hResult);
      DbgPrintf(4, _T("%d country codes loaded"), s_countryListSize);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get country alpha code from numeric code
 */
const TCHAR NXCORE_EXPORTABLE *CountryAlphaCode(const TCHAR *code)
{
   for(int i = 0; i < s_countryListSize; i++)
   {
      if (!_tcscmp(s_countryList[i].numericCode, code) || !_tcsicmp(s_countryList[i].alpha3Code, code))
         return s_countryList[i].alphaCode;
   }
   return NULL;
}

/**
 * Get country name from code
 */
const TCHAR NXCORE_EXPORTABLE *CountryName(const TCHAR *code)
{
   for(int i = 0; i < s_countryListSize; i++)
   {
      if (!_tcscmp(s_countryList[i].numericCode, code) || !_tcsicmp(s_countryList[i].alphaCode, code) || !_tcsicmp(s_countryList[i].alpha3Code, code))
         return s_countryList[i].name;
   }
   return NULL;
}

/**
 * Initialize currency list
 */
void InitCurrencyList()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT numeric_code,alpha_code,description,exponent FROM currency_codes"));
   if (hResult != NULL)
   {
      s_currencyListSize = DBGetNumRows(hResult);
      if (s_currencyListSize > 0)
      {
         s_currencyList = (CURRENCY_INFO *)malloc(sizeof(CURRENCY_INFO) * s_currencyListSize);
         for(int i = 0; i < s_currencyListSize; i++)
         {
            DBGetField(hResult, i, 0, s_currencyList[i].numericCode, 4);
            DBGetField(hResult, i, 1, s_currencyList[i].alphaCode, 4);
            s_currencyList[i].description = DBGetField(hResult, i, 2, NULL, 0);
            s_currencyList[i].exponent = DBGetFieldLong(hResult, i, 3);
            s_currencyList[i].divisor = pow(10.0, s_currencyList[i].exponent);
         }
      }
      DBFreeResult(hResult);
      DbgPrintf(4, _T("%d currency codes loaded"), s_currencyListSize);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Resolve currency code
 */
const TCHAR NXCORE_EXPORTABLE *CurrencyAlphaCode(const TCHAR *numericCode)
{
   for(int i = 0; i < s_currencyListSize; i++)
   {
      if (!_tcscmp(s_currencyList[i].numericCode, numericCode))
         return s_currencyList[i].alphaCode;
   }
   return NULL;
}

/**
 * Get currency exponent
 */
int NXCORE_EXPORTABLE CurrencyExponent(const TCHAR *code)
{
   for(int i = 0; i < s_currencyListSize; i++)
   {
      if (!_tcsicmp(s_currencyList[i].alphaCode, code) || !_tcscmp(s_currencyList[i].numericCode, code))
         return s_currencyList[i].exponent;
   }
   return 0;
}

/**
 * Get currency name
 */
const TCHAR NXCORE_EXPORTABLE *CurrencyName(const TCHAR *code)
{
   for(int i = 0; i < s_currencyListSize; i++)
   {
      if (!_tcsicmp(s_currencyList[i].alphaCode, code) || !_tcscmp(s_currencyList[i].numericCode, code))
         return s_currencyList[i].description;
   }
   return NULL;
}

/*
** NetXMS - Network Management System
** Copyright (C) 2021 Raden Solutions
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
** File: search_query.cpp
**
**/

#include "nxcore.h"

/**
 * Constructor for search string
 */
SearchQuery::SearchQuery(const String &searchString)
{
   m_excludedTexts = nullptr;
   m_includedTexts = nullptr;
   m_excludedAttributes = nullptr;
   m_includedAttributes = nullptr;

   const TCHAR *input = searchString;
   size_t inputSize = searchString.length();
   size_t position = 0, start = 0;
   int state = 0;
   bool negateNext = false;
   StringSet *attributeValues;
   while(position <= inputSize)
   {
      TCHAR ch = (position < inputSize) ? input[position] : _T(' ');
      switch(state)
      {
         case 0: // whitespace
            if (ch == _T('"'))
            {
               state = 3;
               start = position + 1;
            }
            else if (ch != _T(' '))
            {
               state = 1;
               start = position;
            }
            break;
         case 1: // text started
            if (ch == _T(' '))
            {
               String text = String(input + start, position - start);
               if (!_tcsicmp(text, _T("not")))
               {
                  negateNext = true;
               }
               else
               {
                  if (negateNext)
                  {
                     if (m_excludedTexts == nullptr)
                        m_excludedTexts = new StringSet();
                     m_excludedTexts->add(text);
                     negateNext = false;
                  }
                  else
                  {
                     if (m_includedTexts == nullptr)
                        m_includedTexts = new StringSet();
                     m_includedTexts->add(text);
                  }
               }
               state = 0;
            }
            else if (ch == ':') // Start of attribute
            {
               StringObjectMap<StringSet> *attributes;
               if ((input[start] == '-') || negateNext)
               {
                  if (m_excludedAttributes == nullptr)
                     m_excludedAttributes = new StringObjectMap<StringSet>(Ownership::True);
                  attributes = m_excludedAttributes;
                  if (input[start] == '-')
                     start++;
                  negateNext = false;
               }
               else
               {
                  if (m_includedAttributes == nullptr)
                     m_includedAttributes = new StringObjectMap<StringSet>(Ownership::True);
                  attributes = m_includedAttributes;
               }

               String name = String(input + start, position - start);
               attributeValues = attributes->get(name);
               if (attributeValues == nullptr)
               {
                  attributeValues = new StringSet();
                  attributes->set(name, attributeValues);
               }

               start = position + 1;
               state = 2;
            }
            break;
         case 2: // attribute values
            if ((ch == _T(' ')) || (ch == _T(',')))
            {
               String v = String(input + start, position - start);
               if (!v.isEmpty())
                  attributeValues->add(v);
               if (ch == _T(','))
               {
                  start = position + 1;
               }
               else
               {
                  state = 0; // Whitespace, end of current element
               }
            }
            break;
         case 3: // quoted text
            if (ch == _T('"'))
            {
               if (negateNext)
               {
                  if (m_excludedTexts == nullptr)
                     m_excludedTexts = new StringSet();
                  String v = String(input + start, position - start);
                  m_excludedTexts->add(v);
                  negateNext = false;
               }
               else
               {
                  if (m_includedTexts == nullptr)
                     m_includedTexts = new StringSet();
                  String v = String(input + start, position - start);
                  m_includedTexts->add(v);
               }
               state = 0;
            }
            break;
      }
      position++;
   }
}

/**
 * Search query destructor
 */
SearchQuery::~SearchQuery()
{
   delete m_excludedTexts;
   delete m_includedTexts;
   delete m_excludedAttributes;
   delete m_includedAttributes;
}

/**
 * Match required attribute
 */
bool SearchQuery::match(const SearchAttributeProvider &provider) const
{
   if (m_excludedTexts != nullptr)
   {
      for (const TCHAR *s : *m_excludedTexts)
      {
         if (_tcsistr(provider.getText(), s) != nullptr)
         {
            return false;
         }
      }
   }
   if (m_includedTexts != nullptr)
   {
      for (const TCHAR *s : *m_includedTexts)
      {
         if (_tcsistr(provider.getText(), s) == nullptr)
         {
            return false;
         }
      }
   }

   if (m_excludedAttributes != nullptr)
   {
      for (KeyValuePair<StringSet> *attr : *m_excludedAttributes)
      {
         SharedString text = provider.getAttribute(attr->key);
         if (!text.isNull())
         {
            for (const TCHAR *s : *(attr->value))
            {
               if (_tcsistr(text, s) != nullptr)
               {
                  return false;
               }
            }
         }
      }
   }

   if (m_includedAttributes != nullptr)
   {
      for (KeyValuePair<StringSet> *attr : *m_includedAttributes)
      {
         SharedString text = provider.getAttribute(attr->key);
         if (!text.isNull())
         {
            for (const TCHAR *s : *(attr->value))
            {
               if (_tcsistr(text, s) == nullptr)
               {
                  return false;
               }
            }
         }
      }
   }

   return true;
}

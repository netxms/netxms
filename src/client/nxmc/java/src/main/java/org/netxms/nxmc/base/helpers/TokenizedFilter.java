/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.base.helpers;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Predicate;

/**
 * Filter string split into tokens. Tokens are separated by whitespace; text enclosed in double quotes forms single token (exact
 * phrase). Element passes the filter if each token matches at least one of element's fields. Matching is case-insensitive. Filter
 * without tokens matches everything.
 */
public final class TokenizedFilter
{
   private final String filterString;
   private final String[] tokens;

   /**
    * Create tokenized filter from filter string.
    *
    * @param filterString filter string (can be null)
    */
   public TokenizedFilter(String filterString)
   {
      this.filterString = (filterString != null) ? filterString : "";
      this.tokens = tokenize(this.filterString.toLowerCase());
   }

   /**
    * Split filter string into tokens. Double quote without matching closing quote is treated as literal character.
    *
    * @param s filter string
    * @return list of tokens
    */
   private static String[] tokenize(String s)
   {
      List<String> tokens = new ArrayList<>();
      StringBuilder token = new StringBuilder();
      for(int i = 0; i < s.length(); i++)
      {
         char ch = s.charAt(i);
         if (ch == '"')
         {
            int end = s.indexOf('"', i + 1);
            if (end != -1)
            {
               token.append(s, i + 1, end);
               i = end;
            }
            else
            {
               token.append(ch);
            }
         }
         else if (Character.isWhitespace(ch))
         {
            if (token.length() > 0)
            {
               tokens.add(token.toString());
               token.setLength(0);
            }
         }
         else
         {
            token.append(ch);
         }
      }
      if (token.length() > 0)
         tokens.add(token.toString());
      return tokens.toArray(new String[tokens.size()]);
   }

   /**
    * Check if filter is empty (has no tokens and so matches everything).
    *
    * @return true if filter is empty
    */
   public boolean isEmpty()
   {
      return tokens.length == 0;
   }

   /**
    * Get original filter string.
    *
    * @return original filter string
    */
   public String getFilterString()
   {
      return filterString;
   }

   /**
    * Match filter against set of fields. Each token should be contained in at least one field (different tokens can match
    * different fields). Null fields are ignored.
    *
    * @param fields field values
    * @return true if all tokens matched
    */
   public boolean matches(String... fields)
   {
      if (tokens.length == 0)
         return true;

      String[] values = new String[fields.length];
      for(int i = 0; i < fields.length; i++)
         values[i] = (fields[i] != null) ? fields[i].toLowerCase() : null;

      for(String token : tokens)
      {
         boolean found = false;
         for(String v : values)
         {
            if ((v != null) && v.contains(token))
            {
               found = true;
               break;
            }
         }
         if (!found)
            return false;
      }
      return true;
   }

   /**
    * Match filter using custom token matcher. Intended for elements where collecting all field values up front is expensive.
    *
    * @param tokenMatcher predicate that receives single lowercase token and returns true if element matches it
    * @return true if all tokens matched
    */
   public boolean matchesEachToken(Predicate<String> tokenMatcher)
   {
      for(String token : tokens)
      {
         if (!tokenMatcher.test(token))
            return false;
      }
      return true;
   }
}

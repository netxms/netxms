/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.base;

/**
 * Glob matcher
 */
public class Glob
{
	/**
	 * Match string against glob - implementation. Ported from C code in libnetxms.
	 * 
	 * @param pattern glob
	 * @param string string to match
	 * @return true if string matches glob
	 */
	private static boolean matchInternal(char[] pattern, char[] string)
	{
		int mptr = 0;
		int sptr = 0;
		while(mptr < pattern.length)
		{
			switch(pattern[mptr])
			{
				case '?':
					if (sptr >= string.length)
						return false;
					sptr++;
					mptr++;
					break;
				case '*':
					while((mptr < pattern.length) && (pattern[mptr] == '*'))
						mptr++;
					if (mptr == pattern.length)
						return true;	// * at the end matches everything
					while((mptr < pattern.length) && (pattern[mptr] == '?'))	// Handle "*?" case
					{
						if (sptr >= string.length)
							return false;
						sptr++;
						mptr++;
					}
					
					int bptr = mptr;	// Text block begins here
	            while((mptr < pattern.length) && (pattern[mptr] != '?') && (pattern[mptr] != '*'))
	               mptr++;     // Find the end of text block
	            // Try to find rightmost matching block
	            int eptr = -1;
	            boolean finishScan = false;
	            do
	            {
	               while(true)
	               {
	                  while((sptr < string.length) && (string[sptr] != pattern[bptr]))
	                     sptr++;
	                  if ((string.length - sptr) < (mptr - bptr))
	                  {
	                     if (eptr == -1)
	                     {
	                        return false;  // Length of remained text less than remaining pattern
	                     }
	                     else
	                     {
	                        sptr = eptr;   // Revert back to last match
	                        finishScan = true;
	                        break;
	                     }
	                  }
	                  
	                  boolean matched = true;
	                  for(int i = 0; i < (mptr - bptr); i++)
	                  {
	                  	if (pattern[bptr + i] != string[sptr + i])
	                  	{
	                  		matched = false;
	                  		break;
	                  	}
	                  }
	                  if (matched)
	                     break;
	                  sptr++;
	               }
	               if (!finishScan)
	               {
	                  sptr += (mptr - bptr);   // Increment SPtr because we already match current fragment
	                  eptr = sptr;   // Remember current point
	               }
	            }
	            while(!finishScan);
					break;
				default:
					if (pattern[mptr] != string[sptr])
						return false;
					sptr++;
					mptr++;
					break;
			}
		}

		return sptr == string.length;
	}

	/**
	 * Match string against glob
	 * @param pattern glob
	 * @param string string to match
	 * @return true if string matches glob
	 */
	public static boolean match(String pattern, String string)
	{
		if (string.length() == 0)
			return pattern.equals("*");
		return matchInternal(pattern.toCharArray(), string.toCharArray());
	}

	/**
	 * Match string against glob ignoring characters case
	 * 
	 * @param pattern glob
	 * @param string string to match
	 * @return true if string matches glob
	 */
	public static boolean matchIgnoreCase(String pattern, String string)
	{
		if (string.length() == 0)
			return pattern.equals("*");
		return matchInternal(pattern.toUpperCase().toCharArray(), string.toUpperCase().toCharArray());
	}
}

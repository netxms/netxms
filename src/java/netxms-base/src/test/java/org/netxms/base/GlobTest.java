/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import junit.framework.TestCase;

/**
 * Glob matcher test
 */
public class GlobTest extends TestCase
{
	public void testGlob()
	{
		assertTrue(Glob.match("a*", "alpha"));
		assertTrue(!Glob.match("a*", "Alpha"));
		assertTrue(Glob.matchIgnoreCase("a*", "Alpha"));
		assertTrue(!Glob.match("a*b", "alpha"));
		assertTrue(Glob.match("a?01", "ab01"));
		assertTrue(!Glob.match("a?01", "abc01"));
		assertTrue(!Glob.match("c*", ""));
		assertTrue(Glob.match("*", ""));
		assertTrue(Glob.match("*bc*f", "abcdef"));
	}
}

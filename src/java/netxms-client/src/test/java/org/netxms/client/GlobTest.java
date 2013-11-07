/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.client;

import org.netxms.base.Glob;

import junit.framework.TestCase;

/**
 * tests for Glob class
 *
 */
public class GlobTest extends TestCase
{
	public void testGlobMatch()
	{
		assertTrue(Glob.match("abc*", "abc123"));
		assertTrue(Glob.match("abc?def", "abc1def"));
		assertFalse(Glob.match("abc?def", "abcdef"));
		assertTrue(Glob.match("abc*def", "abcdef"));
		assertTrue(Glob.match("abc*?def", "abcxdef"));
		assertTrue(Glob.match("*test", "retest"));
		assertFalse(Glob.match("*test", "retested"));
	}
}

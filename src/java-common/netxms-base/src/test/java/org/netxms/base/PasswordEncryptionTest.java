/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
 * Test password encryption/decryption
 */
public class PasswordEncryptionTest extends TestCase
{
   private static final String LOGIN = "user1";
   private static final String PASSWORD = "alphaChannel12";
   
   public void testPasswordEncryption() throws Exception
   {
      String encrypted = EncryptedPassword.encrypt(LOGIN, PASSWORD);
      assertNotNull(encrypted);
      String decrypted = EncryptedPassword.decrypt(LOGIN, encrypted);
      assertEquals(decrypted, PASSWORD);
      decrypted = EncryptedPassword.decrypt(LOGIN, PASSWORD);
      assertEquals(decrypted, PASSWORD);
   }
}

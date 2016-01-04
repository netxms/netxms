/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import javax.xml.bind.DatatypeConverter;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

/**
 * Utility class to encrypt and decrypt passwords in same way as nxcencpasswd do.
 * Please note that this functionality does not provide any protection for the passwords -
 * it's only allows to store passwords in configuration files in a not-immediately-readable form,
 * as required by some security standards.
 */
public class EncryptedPassword
{
   /**
    * Encrypt password for given login.
    * 
    * @param login login name
    * @param password plain text password
    * @return password obfuscated wiht ICE algorithm
    * @throws NoSuchAlgorithmException
    * @throws UnsupportedEncodingException
    */
   public static String encrypt(String login, String password) throws NoSuchAlgorithmException, UnsupportedEncodingException
   {
      byte[] key = MessageDigest.getInstance("MD5").digest(login.getBytes("UTF-8"));
      byte[] encrypted = IceCrypto.encrypt(Arrays.copyOf(password.getBytes("UTF-8"), 32), key);
      return DatatypeConverter.printBase64Binary(encrypted);
   }
   
   /**
    * @param login login name
    * @param password ICE-obfuscated password
    * @return clear-text password
    * @throws NoSuchAlgorithmException
    * @throws IOException
    */
   public static String decrypt(String login, String password) throws NoSuchAlgorithmException, IllegalArgumentException, IOException
   {
      byte[] key = MessageDigest.getInstance("MD5").digest(login.getBytes("UTF-8"));
      byte[] encrypted = DatatypeConverter.parseBase64Binary(password);
      byte[] plainText = IceCrypto.decrypt(encrypted, key);
      int i;
      for(i = 0; i < plainText.length; i++)
         if (plainText[i] == 0)
            break;
      return new String(Arrays.copyOf(plainText, i), "UTF-8");
   }
}

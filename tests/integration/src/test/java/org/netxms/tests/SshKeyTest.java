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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SshKeyPair;
import org.netxms.client.constants.RCC;

/**
 * Tests for SSH key import through client library
 */
public class SshKeyTest extends AbstractSessionTest
{
   /**
    * Ed25519 key generated with ssh-keygen -C "" and the first two fields of ssh-keygen -y output for it
    */
   private static final String PRIVATE_KEY =
      "-----BEGIN OPENSSH PRIVATE KEY-----\n" +
      "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW\n" +
      "QyNTUxOQAAACCOOEUiqU27Ixb0PRXsvAVN2vDzfDmPT5Kdo5epGX1ZrQAAAIhQOEmyUDhJ\n" +
      "sgAAAAtzc2gtZWQyNTUxOQAAACCOOEUiqU27Ixb0PRXsvAVN2vDzfDmPT5Kdo5epGX1ZrQ\n" +
      "AAAEBIrGkjUkDgzigS8krFT+NAXAqoO5+BxCYS7EIn+oV6co44RSKpTbsjFvQ9Fey8BU3a\n" +
      "8PN8OY9Pkp2jl6kZfVmtAAAAAAECAwQF\n" +
      "-----END OPENSSH PRIVATE KEY-----\n";
   private static final String PUBLIC_KEY = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAII44RSKpTbsjFvQ9Fey8BU3a8PN8OY9Pkp2jl6kZfVmt";

   /**
    * Import with private key only stores public key derived by server
    */
   @Test
   public void testImportDerivesPublicKey() throws Exception
   {
      final NXCSession session = connectAndLogin();
      try
      {
         final String name = "sshkey-test-" + System.currentTimeMillis();
         SshKeyPair key = new SshKeyPair();
         key.setName(name);
         key.setPrivateKey(PRIVATE_KEY);
         session.updateSshKey(key);

         SshKeyPair imported = null;
         for(SshKeyPair k : session.getSshKeys(true))
         {
            if (k.getName().equals(name))
               imported = k;
         }
         assertNotNull(imported);
         session.deleteSshKey(imported.getId(), true);
         assertEquals(PUBLIC_KEY + " " + name, imported.getPublicKey());
      }
      finally
      {
         session.disconnect();
      }
   }

   /**
    * Import of key in unsupported format is rejected with INVALID_ARGUMENT and creates no key
    */
   @Test
   public void testInvalidKeyRejected() throws Exception
   {
      final NXCSession session = connectAndLogin();
      try
      {
         final String name = "sshkey-test-invalid-" + System.currentTimeMillis();
         SshKeyPair key = new SshKeyPair();
         key.setName(name);
         key.setPrivateKey("-----BEGIN RSA PRIVATE KEY-----\nAAAA\n-----END RSA PRIVATE KEY-----\n");
         NXCException e = assertThrows(NXCException.class, () -> session.updateSshKey(key));
         assertEquals(RCC.INVALID_ARGUMENT, e.getErrorCode());
         assertFalse(session.getSshKeys(false).stream().anyMatch(k -> k.getName().equals(name)));
      }
      finally
      {
         session.disconnect();
      }
   }
}

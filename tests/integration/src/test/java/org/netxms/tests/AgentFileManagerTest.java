/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
 * <p/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.netxms.client.AgentFileData;
import org.netxms.client.AgentFileFingerprint;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Node;
import org.netxms.client.server.AgentFile;
import org.netxms.utilities.TestHelper;
 
/**
  * Agent file manager functionality
  */
public class AgentFileManagerTest extends AbstractSessionTest
{
   @Test
   public void testGetLastValues() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      Node testNode = TestHelper.findManagementServer(session);

      List<AgentFile> fileList = session.listAgentFiles(null, "/", testNode.getObjectId());
      AgentFile tmpAgentFolder = null;
      for(int i = 0; i < fileList.size(); i++)
      {
         AgentFile file = fileList.get(i);
         if (file.getName().equals("/tmp"))
         {
            tmpAgentFolder = file;
            break;
         }
      }
      assertNotNull(tmpAgentFolder);

      List<AgentFile> tmpChildrenList = session.listAgentFiles(tmpAgentFolder, tmpAgentFolder.getFullName(), testNode.getObjectId());
      assertFalse(tmpChildrenList.isEmpty());

      File localFile = new File(this.getClass().getResource("/test.txt").getPath());
      session.uploadLocalFileToAgent(testNode.getObjectId(), localFile, "/tmp/test.txt", true, null);
      tmpChildrenList = session.listAgentFiles(tmpAgentFolder, tmpAgentFolder.getFullName(), testNode.getObjectId());
      AgentFile testAgentFile = null;
      for(AgentFile fileForTest : tmpChildrenList)
      {
         if (fileForTest.getName().equalsIgnoreCase(localFile.getName()))
         {
            testAgentFile = fileForTest;
            break;
         }
      }
      assertNotNull(testAgentFile);

      AgentFileData agentFile = session.downloadFileFromAgent(testNode.getObjectId(), "/tmp/test.txt", 1000, false, null);
      BufferedReader agentFileReader = new BufferedReader(new FileReader(agentFile.getFile()));
      try (BufferedReader fileReader = new BufferedReader(new FileReader(localFile)))
      {
         assertEquals(fileReader.readLine(), agentFileReader.readLine());
      }

      session.renameAgentFile(testNode.getObjectId(), testAgentFile.getFilePath(), "/tmp/anotherName2.txt", false);
      tmpChildrenList = session.listAgentFiles(tmpAgentFolder, tmpAgentFolder.getFullName(), testNode.getObjectId());
      for(AgentFile fileForTest : tmpChildrenList)
      {
         if (fileForTest.getName().equals("anotherName2.txt"))
         {
            testAgentFile = fileForTest;
            break;
         }
      }
      assertEquals(testAgentFile.getName(), "anotherName2.txt");

      AgentFileFingerprint testFingerprint = session.getAgentFileFingerprint(testNode.getObjectId(), testAgentFile.getFullName());
      byte[] bytes = testFingerprint.getSHA256();
      StringBuilder hexString = new StringBuilder();
      for(byte b : bytes)
      {
         hexString.append(String.format("%02x", b));
      }
      String agentFileFingerprint = hexString.toString();

      MessageDigest digest = MessageDigest.getInstance("SHA-256");
      FileInputStream fis = new FileInputStream(localFile.getPath());
      DigestInputStream dis = new DigestInputStream(fis, digest);
      byte[] buffer = new byte[8192];
      while(dis.read(buffer) != -1)
      {
      }
      byte[] hash = digest.digest();

      StringBuilder hexString2 = new StringBuilder();
      for(byte b : hash)
      {
         hexString2.append(String.format("%02x", b));
      }
      String localFileFingerPrint = hexString2.toString();

      assertEquals(localFileFingerPrint, agentFileFingerprint);

      session.deleteAgentFile(testNode.getObjectId(), "/tmp/anotherName2.txt");
      boolean found = false;
      tmpChildrenList = session.listAgentFiles(tmpAgentFolder, tmpAgentFolder.getFullName(), testNode.getObjectId());
      for(AgentFile fileForTest : tmpChildrenList)
      {
         if (fileForTest.getName().equals("anotherName2.txt"))
         {
            found = true;
            break;
         }
      }
      assertFalse(found);
      
      session.disconnect();
   }
}

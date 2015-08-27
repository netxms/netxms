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
package org.netxms.client;

import java.io.FileReader;
import java.io.IOException;
import org.netxms.client.server.ServerFile;

/**
 * Test functionality related to server file store
 */
public class ServerFilesTest extends AbstractSessionTest
{
   public void testFileList() throws Exception
   {
      final NXCSession session = connect();

      ServerFile[] files = session.listServerFiles();
      for(ServerFile f : files)
         System.out.println(f.getName() + " size=" + f.getSize() + " modified: " + f.getModifyicationTime().toString());

      session.disconnect();
   }

   public void testAgentFileDownload() throws Exception
   {
      final NXCSession session = connect();

      AgentFileData file = session.downloadFileFromAgent(TestConstants.LOCAL_NODE_ID, TestConstants.FILE_NAME, TestConstants.FILE_OFFSET, true);
      // check that server returned file with correct size (offset should be less than size of file)
      //assertEquals(file.length(), TestConstants.FileOfset);
      String content = null;
      try
      {
         FileReader reader = new FileReader(file.getFile());
         char[] chars = new char[(int)file.getFile().length()];
         reader.read(chars);
         content = new String(chars);
         reader.close();
      }
      catch(IOException e)
      {
         e.printStackTrace();
      }
      System.out.println("Downloaded content is: \n" + content);
      System.out.println("*** Downloaded file: " + file.getFile().getAbsolutePath());
      
      // get tails of provided file
      int i = 3;
      while(i > 0)
      {
         System.out.println("Tail content: \n" + session.waitForFileTail(file.getId(), 30000));
         i--;
      }
      session.cancelFileMonitoring(TestConstants.LOCAL_NODE_ID, file.getId());
      System.out.println("Monitoring have been canceled.\n");
      session.disconnect();
   }
}

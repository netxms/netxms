/**
 * 
 */
package org.netxms.server.ucc.drivers;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import org.netxms.bridge.Platform;
import org.netxms.server.ucc.UserCommunicationChannel;

/**
 * User communication channel that writes messages to file
 */
public class FileChannel extends UserCommunicationChannel
{
   private static final String DEBUG_TAG = "ucc.channel.file";
   
   private File file;
   
   /**
    * @param id
    * @param config
    */
   public FileChannel(String id)
   {
      super(id);
   }
   
   /* (non-Javadoc)
    * @see org.netxms.server.ucc.UserCommunicationChannel#initialize()
    */
   @Override
   public void initialize() throws Exception
   {
      super.initialize();
      file = new File(readConfigurationAsString("FileName", "messages.out"));
   }

   /* (non-Javadoc)
    * @see org.netxms.server.ucc.UserCommunicationChannel#getDriverName()
    */
   @Override
   public String getDriverName()
   {
      return "File";
   }

   /* (non-Javadoc)
    * @see org.netxms.server.ucc.UserCommunicationChannel#send(java.lang.String, java.lang.String, java.lang.String)
    */
   @Override
   public void send(String recipient, String subject, String text) throws Exception
   {
      BufferedWriter out = new BufferedWriter(new FileWriter(file, true));
      try
      {
         StringBuilder sb = new StringBuilder();
         sb.append(recipient);
         sb.append(" - ");
         if (subject != null)
         {
            sb.append('"');
            sb.append(subject);
            sb.append("\" - ");
         }
         sb.append(text);
         out.write(sb.toString());
         out.newLine();
      }
      catch(Exception e)
      {
         Platform.writeDebugLog(DEBUG_TAG, 4, "Exception in file channel " + getId() + " (" + e.getClass().getName() + ")");
         Platform.writeDebugLog(DEBUG_TAG, 4, "   ", e);
      }
      finally
      {
         out.close();
      }
   }
}

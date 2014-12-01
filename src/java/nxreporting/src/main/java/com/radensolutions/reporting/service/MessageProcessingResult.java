/**
 * 
 */
package com.radensolutions.reporting.service;

import java.io.File;
import org.netxms.base.NXCPMessage;

/**
 * Message processing result
 */
public class MessageProcessingResult
{
   public NXCPMessage response;
   public File file;
   
   /**
    * @param response
    * @param file
    */
   public MessageProcessingResult(NXCPMessage response, File file)
   {
      this.response = response;
      this.file = file;
   }

   /**
    * @param response
    */
   public MessageProcessingResult(NXCPMessage response)
   {
      this.response = response;
   }
}

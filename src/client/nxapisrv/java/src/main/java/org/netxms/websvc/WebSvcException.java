/**
 * 
 */
package org.netxms.websvc;

import org.netxms.client.NXCException;
import org.netxms.client.constants.RCC;

/**
 * NetXMS web service specific exception
 */
public class WebSvcException extends Exception
{
   private static final long serialVersionUID = 1L;
   
   private int errorCode;

   /**
    * @param errorCode
    */
   public WebSvcException(int errorCode)
   {
      super(WebSvcStatusService.getMessageFromRCC(errorCode));
      this.errorCode = errorCode;
   }

   /**
    * @param e
    */
   public WebSvcException(Throwable e)
   {
      super(e.getMessage(), e);
      errorCode = (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : RCC.INTERNAL_ERROR;
   }

   /**
    * @param message
    * @param e
    */
   public WebSvcException(String message, Throwable e)
   {
      super(message, e);
      errorCode = RCC.INTERNAL_ERROR;
   }

   /**
    * @return the errorCode
    */
   public int getErrorCode()
   {
      return errorCode;
   }
}

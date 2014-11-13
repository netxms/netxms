package org.netxms.certificate.manager.exception;

public class SignatureImpossibleException extends Exception
{
   private static final long serialVersionUID = 1L;

   public SignatureImpossibleException(String message)
   {
      super(message);
   }
}

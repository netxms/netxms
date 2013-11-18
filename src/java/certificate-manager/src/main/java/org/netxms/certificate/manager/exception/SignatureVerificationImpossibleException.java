package org.netxms.certificate.manager.exception;

public class SignatureVerificationImpossibleException extends Exception
{
   private static final long serialVersionUID = 1L;

   public SignatureVerificationImpossibleException()
   {
      super("Can't establish verification process!");
   }
}

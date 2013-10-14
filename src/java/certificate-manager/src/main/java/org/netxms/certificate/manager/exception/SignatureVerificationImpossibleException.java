package org.netxms.certificate.manager.exception;

public class SignatureVerificationImpossibleException extends Exception
{
   public SignatureVerificationImpossibleException()
   {
      super("Can't establish verification process!");
   }
}

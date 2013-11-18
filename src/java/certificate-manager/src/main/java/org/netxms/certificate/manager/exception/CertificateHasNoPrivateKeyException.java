package org.netxms.certificate.manager.exception;

public class CertificateHasNoPrivateKeyException extends Exception
{
   private static final long serialVersionUID = 1L;

   public CertificateHasNoPrivateKeyException()
   {
      super("The certificate doesn't have a private key attached to it!");
   }
}

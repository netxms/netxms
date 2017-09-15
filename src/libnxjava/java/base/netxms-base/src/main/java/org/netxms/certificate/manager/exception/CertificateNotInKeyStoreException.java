package org.netxms.certificate.manager.exception;

public class CertificateNotInKeyStoreException extends Exception
{
   private static final long serialVersionUID = 1L;

   public CertificateNotInKeyStoreException()
   {
      super("The certificate could not be found in the keystore!");
   }
}

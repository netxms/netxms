package org.netxms.certificate.manager.exception;

public class CertificateNotInKeyStoreException extends Exception
{
   public CertificateNotInKeyStoreException()
   {
      super("The certificate could not be found in the keystore!");
   }
}

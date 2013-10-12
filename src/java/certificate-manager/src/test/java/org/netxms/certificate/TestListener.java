package org.netxms.certificate;

import org.netxms.certificate.manager.CertificateManagerProviderRequestListener;

public class TestListener implements CertificateManagerProviderRequestListener
{
   @Override
   public String keyStorePasswordRequested()
   {
      return "test1337";
   }

   @Override
   public String keyStoreLocationRequested()
   {
      return "src/test/resource/keystore.p12";
   }
}

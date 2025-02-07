package org.netxms.certificate;

import org.netxms.certificate.manager.CertificateManagerRequestListener;

public class TestListener implements CertificateManagerRequestListener
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

   @Override
   public String keyStoreEntryPasswordRequested()
   {
      return "test1337";
   }
}

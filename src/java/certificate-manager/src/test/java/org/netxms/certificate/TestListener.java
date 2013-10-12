package org.netxms.certificate;

import org.netxms.certificate.loader.KeyStoreRequestListener;

public class TestListener implements KeyStoreRequestListener
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

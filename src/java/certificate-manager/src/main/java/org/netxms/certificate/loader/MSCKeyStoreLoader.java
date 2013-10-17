package org.netxms.certificate.loader;

import org.netxms.certificate.loader.exception.KeyStoreLoaderException;

import java.security.KeyStore;

public class MSCKeyStoreLoader implements KeyStoreLoader
{
   @Override
   public KeyStore loadKeyStore() throws KeyStoreLoaderException
   {
      KeyStore ks;

      try
      {
         ks = KeyStore.getInstance("Windows-MY", "SunMSCAPI");
         ks.load(null, null);
      }
      catch(Exception e)
      {
         throw new KeyStoreLoaderException(e.getMessage());
      }

      return ks;
   }

   @Override
   public void setKeyStoreRequestListener(KeyStoreRequestListener listener)
   {
      // Not needed
   }
}

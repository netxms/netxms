package org.netxms.certificate.loader;

import org.netxms.certificate.loader.exception.KeyStoreLoaderException;

import java.io.FileInputStream;
import java.security.KeyStore;

public class PKCS12KeyStoreLoader implements KeyStoreLoader
{
   private KeyStoreRequestListener listener;

   @Override
   public KeyStore loadKeyStore() throws KeyStoreLoaderException
   {
      if (listener == null)
      {
         throw new KeyStoreLoaderException("KeyStoreRequestListener not set!");
      }

      KeyStore ks;

      try
      {
         ks = KeyStore.getInstance("PKCS12");

         String ksLocation = listener.keyStoreLocationRequested();

         // TODO: JAVA7 try-with-resources
         FileInputStream fis = new FileInputStream(ksLocation);
         try
         {
            String keyStorePassword = listener.keyStorePasswordRequested();
            ks.load(fis, keyStorePassword.toCharArray());
         }
         finally
         {
            fis.close();
         }
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
      this.listener = listener;
   }
}

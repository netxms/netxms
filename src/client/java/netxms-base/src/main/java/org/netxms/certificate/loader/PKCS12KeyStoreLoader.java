package org.netxms.certificate.loader;

import org.netxms.certificate.loader.exception.KeyStoreLoaderException;

import java.io.FileInputStream;
import java.io.IOException;
import java.security.KeyStore;

public class PKCS12KeyStoreLoader implements KeyStoreLoader
{
   private KeyStoreRequestListener listener;

   /* (non-Javadoc)
    * @see org.netxms.certificate.loader.KeyStoreLoader#loadKeyStore()
    */
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

         if (ksLocation == null)
         {
            return null;
         }

         // TODO: JAVA7 try-with-resources
         FileInputStream fis = new FileInputStream(ksLocation);
         try
         {
            try
            {
               ks.load(fis, new char[0]);
            }
            catch(IOException ioe)
            {
               fis.close();
               fis = new FileInputStream(ksLocation);

               String keyStorePassword = listener.keyStorePasswordRequested();

               if (keyStorePassword == null || keyStorePassword.isEmpty()) return null;

               ks.load(fis, keyStorePassword.toCharArray());
            }
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

   /* (non-Javadoc)
    * @see org.netxms.certificate.loader.KeyStoreLoader#setKeyStoreRequestListener(org.netxms.certificate.loader.KeyStoreRequestListener)
    */
   @Override
   public void setKeyStoreRequestListener(KeyStoreRequestListener listener)
   {
      this.listener = listener;
   }
}

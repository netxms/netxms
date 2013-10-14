package org.netxms.certificate.loader;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.cert.CertificateException;

public class PKCS12KeyStoreLoader implements KeyStoreLoader
{
   private final KeyStoreRequestListener listener;

   public PKCS12KeyStoreLoader(KeyStoreRequestListener listener)
   {
      this.listener = listener;
   }

   @Override
   public KeyStore loadKeyStore()
      throws CertificateException, NoSuchAlgorithmException, NoSuchProviderException, KeyStoreException, IOException
   {
      KeyStore ks = KeyStore.getInstance("PKCS12");
      String ksLocation = listener.keyStoreLocationRequested();
      String keyStorePassword = listener.keyStorePasswordRequested();

      // TODO: JAVA7 try-with-resources
      FileInputStream fis;
      try
      {
         fis = new FileInputStream(ksLocation);
         try
         {
            ks.load(fis, keyStorePassword.toCharArray());
         }
         finally
         {
            fis.close();
         }
      }
      catch(FileNotFoundException fnfe)
      {
         System.out.println(fnfe.getMessage());
      }

      return ks;
   }
}

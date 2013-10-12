package org.netxms.certificate.loader;

import java.io.IOException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.cert.CertificateException;

public class MSCKeyStoreLoader implements KeyStoreLoader
{
   @Override
   public KeyStore loadKeyStore()
      throws CertificateException, NoSuchAlgorithmException, NoSuchProviderException, KeyStoreException, IOException
   {
      KeyStore ks = KeyStore.getInstance("Windows-MY", "SunMSCAPI");
      //ks = KeyStore.getInstance("Windows-ROOT", "SunMSCAPI");
      ks.load(null, null);

      return ks;
   }
}

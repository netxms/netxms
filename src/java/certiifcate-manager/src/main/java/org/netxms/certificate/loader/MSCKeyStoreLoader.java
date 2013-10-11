package org.netxms.certificate.loader;

import org.netxms.certificate.Certificate;

import java.io.IOException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.cert.CertificateException;

public class MSCKeyStoreLoader implements KeyStoreLoader
{
   @Override
   public Certificate[] retrieveCertificates()
   {
      return new Certificate[0];
   }

   protected KeyStore getMscKeyStore()
      throws NoSuchProviderException, KeyStoreException, CertificateException, NoSuchAlgorithmException, IOException
   {

      KeyStore ks = KeyStore.getInstance("Windows-MY", "SunMSCAPI");
      ks.load(null, null);

      return ks;
   }
}

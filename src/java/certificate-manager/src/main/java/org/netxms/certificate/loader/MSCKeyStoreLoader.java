package org.netxms.certificate.loader;

import java.io.IOException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.util.List;

public class MSCKeyStoreLoader implements KeyStoreLoader
{
   @Override
   public List<Certificate> retrieveCertificates()
   {
      return null;
   }

   protected KeyStore getMscKeyStore()
      throws NoSuchProviderException, KeyStoreException, CertificateException, NoSuchAlgorithmException, IOException
   {

      KeyStore ks = KeyStore.getInstance("Windows-MY", "SunMSCAPI");
      ks.load(null, null);

      return ks;
   }
}

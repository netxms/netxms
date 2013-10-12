package org.netxms.certificate.loader;

import java.io.IOException;
import java.security.*;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public class MSCKeyStoreLoader implements KeyStoreLoader
{
   @Override
   public KeyStore loadKeyStore()
      throws CertificateException, NoSuchAlgorithmException, NoSuchProviderException, KeyStoreException, IOException
   {
      KeyStore ks = null;

      ks = KeyStore.getInstance("Windows-MY", "SunMSCAPI");
      ks.load(null, null);

      return ks;
   }
}

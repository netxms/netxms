package org.netxms.certificate.manager;

import java.security.KeyStore;
import java.security.cert.Certificate;
import java.util.List;

public class CertificateManager
{
   private final KeyStore keyStore;
   private final List<Certificate> certs;

   CertificateManager(KeyStore keyStore, List<Certificate> certs)
   {
      this.keyStore = keyStore;
      this.certs = certs;
   }

   public List<Certificate> getCerts()
   {
      return certs;
   }

   public boolean hasNoCertificates()
   {
      return certs.size() == 0;
   }
}

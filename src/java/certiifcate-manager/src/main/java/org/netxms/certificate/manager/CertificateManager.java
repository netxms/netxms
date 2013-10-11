package org.netxms.certificate.manager;

import org.netxms.certificate.Certificate;

public class CertificateManager
{
   private final Certificate[] certs;

   CertificateManager(Certificate[] certs)
   {
      this.certs = certs;
   }

   public Certificate[] getCerts()
   {
      return certs;
   }

   public boolean hasNoCertificates()
   {
      return certs.length == 0;
   }
}

package org.netxms.certificate.manager;

import java.security.PrivateKey;
import java.security.cert.Certificate;

public class CertificateEntry
{
   private final Certificate cert;
   private final PrivateKey pk;

   public CertificateEntry(Certificate certificate, PrivateKey privateKey) {
      this.cert = certificate;
      this.pk = privateKey;
   }

   public Certificate getCert()
   {
      return cert;
   }

   public PrivateKey getPk() {
      return pk;
   }
}

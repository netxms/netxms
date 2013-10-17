package org.netxms.certificate.manager;

import org.netxms.certificate.loader.KeyStoreLoader;
import org.netxms.certificate.loader.MSCKeyStoreLoader;
import org.netxms.certificate.loader.PKCS12KeyStoreLoader;

public class CertificateManagerProvider
{
   private static volatile CertificateManager manager;

   private CertificateManagerProvider()
   {
   }

   public static synchronized CertificateManager provideCertificateManager()
   {
      if (manager != null) return manager;
      manager = new CertificateManagerProvider().createCertificateManager();
      return manager;
   }

   protected CertificateManager createCertificateManager()
   {
      final String os = System.getProperty("os.name");
      final KeyStoreLoader loader;

      if (os.startsWith("Windows"))
      {
         loader = new MSCKeyStoreLoader();
      }
      else
      {
         loader = new PKCS12KeyStoreLoader();
      }

      return new CertificateManager(loader);
   }


   public static synchronized void dispose()
   {
      if (manager == null) return;

      manager.setEntryListener(null);
      manager = null;
   }
}

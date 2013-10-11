package org.netxms.certificate;

import org.netxms.certificate.manager.CertificateManager;
import org.netxms.certificate.manager.CertificateManagerProvider;
import org.netxms.certificate.manager.CertificateManagerProviderRequestListener;

public class Main
{
   public static void main(String[] args)
   {
      CertificateManager mgr = CertificateManagerProvider
         .provideCertificateManager(new CertificateManagerProviderRequestListener()
         {
            @Override
            public String keyStoreLocationRequested()
            {
               return null;
            }

            @Override
            public String keyStorePasswordRequested()
            {
               return null;
            }
         });

      if (!mgr.hasNoCertificates())
      {
         System.out.println(mgr.getCerts()[0].getCommonName());
      }
      else
      {
         System.out.println("No certs!");
      }
   }
}

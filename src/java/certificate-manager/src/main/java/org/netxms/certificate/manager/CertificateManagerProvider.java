package org.netxms.certificate.manager;

import org.netxms.certificate.loader.KeyStoreLoader;
import org.netxms.certificate.loader.KeyStoreRequestListener;
import org.netxms.certificate.loader.MSCKeyStoreLoader;
import org.netxms.certificate.loader.PKCS12KeyStoreLoader;

import java.io.IOException;
import java.security.*;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public class CertificateManagerProvider
{
   private static volatile CertificateManager manager;
   private final KeyStoreRequestListener listener;

   private CertificateManagerProvider(
      KeyStoreRequestListener listener)
   {
      this.listener = listener;
   }

   public static synchronized CertificateManager provideCertificateManager(
      KeyStoreRequestListener listener)
   {
      if (manager != null) return manager;
      manager = new CertificateManagerProvider(listener).createCertificateManager();
      return manager;
   }

   protected CertificateManager createCertificateManager()
   {
      final String os = System.getProperty("os.name");
      final KeyStoreLoader loader;
      List<CertificateEntry> certs;

      if (os.startsWith("Windows"))
      {
         loader = new MSCKeyStoreLoader();
      }
      else
      {
         loader = new PKCS12KeyStoreLoader(listener);
      }

      try
      {
         KeyStore ks = loader.loadKeyStore();
         certs = getCertsFromKeyStore(ks);
      }
      catch(KeyStoreException e)
      {
         e.printStackTrace();
         certs = new ArrayList<CertificateEntry>(0);
      }
      catch(CertificateException e)
      {
         e.printStackTrace();
         certs = new ArrayList<CertificateEntry>(0);
      }
      catch(NoSuchAlgorithmException e)
      {
         e.printStackTrace();
         certs = new ArrayList<CertificateEntry>(0);
      }
      catch(IOException e)
      {
         e.printStackTrace();
         certs = new ArrayList<CertificateEntry>(0);
      }
      catch (NoSuchProviderException e)
      {
         e.printStackTrace();
         certs = new ArrayList<CertificateEntry>(0);
      }
      catch (UnrecoverableEntryException e)
      {
         e.printStackTrace();
         certs = new ArrayList<CertificateEntry>(0);
      }

      return new CertificateManager(certs);
   }

   protected List<CertificateEntry> getCertsFromKeyStore(KeyStore ks)
      throws KeyStoreException, UnrecoverableEntryException, NoSuchAlgorithmException
   {
      if (ks.size() == 0)
      {
         return new ArrayList<CertificateEntry>(0);
      }

      List<CertificateEntry> certs = new ArrayList<CertificateEntry>();
      Enumeration<String> aliases = ks.aliases();
      KeyStore.ProtectionParameter protParam = new KeyStore.PasswordProtection(null);

      while(aliases.hasMoreElements())
      {
         String alias = aliases.nextElement();
            //X509Certificate x509Cert = (X509Certificate) ks.getCertificate(alias);
            //Principal subjectField = x509Cert.getSubjectDN();

            //Subject subject = SubjectParser.parseSubject(subjectField.toString());
         if(ks.isKeyEntry(alias))
         {
            Certificate cert = ks.getCertificate(alias);
            KeyStore.PrivateKeyEntry pkEntry = (KeyStore.PrivateKeyEntry) ks.getEntry(alias, protParam);
            PrivateKey pk = pkEntry.getPrivateKey();

            certs.add(new CertificateEntry(cert,pk));
         }
      }

      return certs;
   }

   public static synchronized void dispose()
   {
      manager = null;
   }
}

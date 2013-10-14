package org.netxms.certificate.manager;

import org.netxms.certificate.manager.exception.CertificateHasNoPrivateKeyException;
import org.netxms.certificate.manager.exception.CertificateNotInKeyStoreException;
import org.netxms.certificate.manager.exception.SignatureImpossibleException;
import org.netxms.certificate.manager.exception.SignatureVerificationImpossibleException;
import org.netxms.certificate.request.KeyStoreEntryPasswordRequestListener;

import java.security.*;
import java.security.cert.Certificate;

public class CertificateManager
{
   private final KeyStore keyStore;
   private final Certificate[] certs;
   private KeyStoreEntryPasswordRequestListener listener;

   CertificateManager(KeyStore keyStore, Certificate[] certs)
   {
      this.keyStore = keyStore;
      this.certs = certs;
   }

   public void setListener(KeyStoreEntryPasswordRequestListener listener)
   {
      this.listener = listener;
   }

   public Certificate[] getCerts()
   {
      return certs;
   }

   public boolean hasNoCertificates()
   {
      return certs.length == 0;
   }

   public byte[] sign(Certificate cert, byte[] challenge) throws SignatureImpossibleException
   {
      byte[] signedChallenge;

      try
      {
         PrivateKey pk = getPrivateKey(cert);
         Signature signature = Signature.getInstance("SHA1withRSA");
         signature.initSign(pk);
         signature.update(challenge);
         signedChallenge = signature.sign();
      }
      catch(Exception e)
      {
         throw new SignatureImpossibleException(e.getMessage());
      }

      return signedChallenge;
   }

   public boolean verify(Certificate cert, byte[] original, byte[] signed)
      throws SignatureVerificationImpossibleException
   {
      boolean verified;

      try
      {
         Signature verifier = Signature.getInstance("SHA1withRSA");
         verifier.initVerify(cert);
         verifier.update(original);

         verified = verifier.verify(signed);
      }
      catch(Exception e)
      {
         throw new SignatureVerificationImpossibleException();
      }

      return verified;
   }

   protected PrivateKey getPrivateKey(Certificate cert)
      throws KeyStoreException, CertificateNotInKeyStoreException, CertificateHasNoPrivateKeyException,
      NoSuchAlgorithmException, UnrecoverableEntryException
   {
      String alias = keyStore.getCertificateAlias(cert);

      if (alias == null) throw new CertificateNotInKeyStoreException();

      KeyStore.PrivateKeyEntry pkEntry;

      try
      {
         pkEntry = (KeyStore.PrivateKeyEntry) keyStore.getEntry(alias, null);
      }
      catch(UnrecoverableEntryException uee)
      {
         String password = getEntryPassword();
         KeyStore.ProtectionParameter parameter = new KeyStore.PasswordProtection(password.toCharArray());

         pkEntry = (KeyStore.PrivateKeyEntry) keyStore.getEntry(alias, parameter);
      }

      PrivateKey pk = pkEntry.getPrivateKey();

      if (pk == null) throw new CertificateHasNoPrivateKeyException();

      return pk;
   }

   protected String getEntryPassword()
   {
      if (listener == null) return "";

      return listener.keyStoreEntryPasswordRequested();
   }
}

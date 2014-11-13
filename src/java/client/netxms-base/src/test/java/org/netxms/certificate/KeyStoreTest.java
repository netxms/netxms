package org.netxms.certificate;

import org.junit.Before;
import org.junit.Test;

import java.io.FileInputStream;
import java.io.IOException;
import java.security.KeyStore;
import java.security.Signature;
import java.security.cert.Certificate;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.not;
import static org.junit.Assert.assertTrue;

public class KeyStoreTest
{
   private final TestListener testListener = new TestListener();
   private final String password = testListener.keyStorePasswordRequested();
   private final String location = testListener.keyStoreLocationRequested();
   private KeyStore keyStore;
   private Certificate cert;
   private KeyStore.PrivateKeyEntry pkEntry;

   @Before
   public void setUp() throws Exception
   {
      keyStore = KeyStore.getInstance("PKCS12");

      FileInputStream fis = new FileInputStream(location);

      try
      {
         keyStore.load(fis, password.toCharArray());
      }
      finally
      {
         fis.close();
      }

      cert = keyStore.getCertificate("John Doe");
      pkEntry = (KeyStore.PrivateKeyEntry) keyStore
         .getEntry("John Doe", new KeyStore.PasswordProtection(password.toCharArray()));
   }

   @Test
   public void testKeyStore_NotEmpty() throws Exception
   {
      int numCerts = keyStore.size();

      assertThat(numCerts, greaterThan(0));
   }

   @Test
   public void testKeyStore_TestKeys()
   {
      String pubString = cert.getPublicKey().toString();
      String pkString = pkEntry.getPrivateKey().toString();

      assertThat(pubString, not(pkString));
   }

   @Test
   public void testKeyStore_TestSign() throws Exception
   {
      byte[] challenge = "I challenge you to sign this!".getBytes();

      Signature sig = Signature.getInstance("SHA1withRSA");
      sig.initSign(pkEntry.getPrivateKey());
      sig.update(challenge);

      Signature verifier = Signature.getInstance("SHA1withRSA");
      verifier.initVerify(cert);
      verifier.update(challenge);

      byte[] signedChallenge = sig.sign();

      assertTrue(verifier.verify(signedChallenge));
   }

   @Test(expected = IOException.class)
   public void testKeyStore_ThrowsIOExcepitonOnEmptyPassword() throws Exception
   {
      keyStore = KeyStore.getInstance("PKCS12");

      FileInputStream fis = new FileInputStream(location);

      try
      {
         keyStore.load(fis, new char[0]);
      }
      finally
      {
         fis.close();
      }
   }
}

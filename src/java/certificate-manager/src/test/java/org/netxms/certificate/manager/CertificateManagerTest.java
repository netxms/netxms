package org.netxms.certificate.manager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.netxms.certificate.TestListener;

import java.security.cert.Certificate;
import java.util.List;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.Assert.assertFalse;

public class CertificateManagerTest
{
   private CertificateManager manager;
   private final TestListener listener = new TestListener();

   @Before
   public void setUp() throws Exception
   {
      manager = CertificateManagerProvider.provideCertificateManager(listener);
   }

   @After
   public void tearDown() throws Exception
   {
      CertificateManagerProvider.dispose();
   }

   @Test
   public void testHasNoCertificates()
   {
      boolean hasNoCertificates = manager.hasNoCertificates();

      assertFalse(hasNoCertificates);
   }

   @Test
   public void testGetCerts_HasExpectedNumberOfCerts()
   {
      List<Certificate> certs = manager.getCerts();
      int numCerts = certs.size();

      assertThat(numCerts, equalTo(1));
   }
}

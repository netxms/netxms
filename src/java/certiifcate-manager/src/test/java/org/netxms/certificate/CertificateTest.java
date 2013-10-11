package org.netxms.certificate;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.netxms.certificate.manager.CertificateManager;
import org.netxms.certificate.manager.CertificateManagerProvider;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.notNullValue;
import static org.hamcrest.MatcherAssert.assertThat;

public class CertificateTest
{
   private CertificateManager manager;
   private Certificate cert;
   private final TestListener listener = new TestListener();

   @Before
   public void setUp()
   {
      manager = CertificateManagerProvider.provideCertificateManager(listener);
      cert = manager.getCerts().get(0);
   }

   @After
   public void tearDown()
   {
      CertificateManagerProvider.dispose();
   }

   @Test
   public void testGetCommonName()
   {
      String commonName = cert.getCommonName();

      assertThat(commonName, equalTo("John Doe"));
   }

   @Test
   public void testGetOrganization()
   {
      String organization = cert.getOrganization();

      assertThat(organization, equalTo("Raden Solutions"));
   }

   @Test
   public void testGetState()
   {
      String state = cert.getState();

      assertThat(state, equalTo("Riga"));
   }

   @Test
   public void testGetCountry()
   {
      String country = cert.getCountry();

      assertThat(country, equalTo("LV"));
   }

   @Test
   public void testGetPrivateKey_ReturnsSomething() throws Exception
   {
      Object o = cert.getPrivateKey();

      assertThat(o, notNullValue());
   }
}

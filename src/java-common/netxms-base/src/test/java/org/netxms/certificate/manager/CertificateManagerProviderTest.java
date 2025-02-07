package org.netxms.certificate.manager;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.notNullValue;
import static org.hamcrest.MatcherAssert.assertThat;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

public class CertificateManagerProviderTest
{
   private CertificateManager manager;

   @BeforeEach
   public void setUp() throws Exception
   {
      manager = CertificateManagerProvider.provideCertificateManager();
   }

   @AfterEach
   public void tearDown()
   {
      CertificateManagerProvider.dispose();
   }

   @Test
   public void testProvideCertificateManager_ProvidesManager()
   {
      assertThat(manager, notNullValue());
   }

   @Test
   public void testProvideCertificateManager_SameInstance()
   {
      CertificateManager mgr2 = CertificateManagerProvider.provideCertificateManager();
      assertThat(manager, equalTo(mgr2));
   }
}

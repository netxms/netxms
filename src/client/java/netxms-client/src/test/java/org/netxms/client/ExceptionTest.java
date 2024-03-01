/**
 * 
 */
package org.netxms.client;

import java.util.Locale;
import org.junit.jupiter.api.Test;
import org.netxms.client.constants.RCC;

/**
 * Client library exception tests
 */
public class ExceptionTest
{
   @Test
   public void testLocalizedMessages()
   {
      NXCException e = new NXCException(RCC.ACCOUNT_DISABLED, "test");
      testLocale("en", e);
      testLocale("ru", e);
      testLocale("es", e);
      testLocale("fr", e);

      // test for non-existing error message
      e = new NXCException(235);
      testLocale("en", e);
      testLocale("ru", e);
   }
   
   private void testLocale(String lang, NXCException e)
   {
      Locale l = new Locale(lang);
      Locale.setDefault(l);
      System.out.println(lang + ": " + e.getLocalizedMessage());
   }
}

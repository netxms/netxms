/**
 * 
 */
package org.netxms.client;

import java.util.Locale;
import org.netxms.client.constants.RCC;
import junit.framework.TestCase;

/**
 * Client library exception tests
 */
public class ExceptionTest extends TestCase
{
   public void testLocalizedMessages()
   {
      NXCException e = new NXCException(RCC.ACCOUNT_DISABLED, "test");
      testLocale("en", e);
      testLocale("ru", e);
      testLocale("es", e);
      testLocale("fr", e);
   }
   
   private void testLocale(String lang, NXCException e)
   {
      Locale l = new Locale(lang);
      Locale.setDefault(l);
      System.out.println(lang + ": " + e.getLocalizedMessage());
   }
}

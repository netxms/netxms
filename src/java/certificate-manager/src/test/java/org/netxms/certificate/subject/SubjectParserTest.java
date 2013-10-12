package org.netxms.certificate.subject;

import org.junit.Before;
import org.junit.Test;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.MatcherAssert.assertThat;

public class SubjectParserTest
{
   private final String testString = "CN=John Doe, O=Raden Solutions, ST=Riga, C=LV";
   private Subject subject;

   @Before
   public void setUp() throws Exception
   {
      subject = SubjectParser.parseSubject(testString);
   }

   @Test
   public void testParseSubject_ParsesNameCorrectly() throws Exception
   {
      String cn = subject.getCommonName();

      assertThat(cn, equalTo("John Doe"));
   }

   @Test
   public void testParseSubject_ParsesOrganizatonCorrectly() throws Exception
   {
      String o = subject.getOrganization();

      assertThat(o, equalTo("Raden Solutions"));
   }

   @Test
   public void testParseSubject_ParsesStateCorrectly() throws Exception
   {
      String st = subject.getState();

      assertThat(st, equalTo("Riga"));
   }

   @Test
   public void testParseSubject_ParsesCountryCorrectly() throws Exception
   {
      String cn = subject.getCountry();

      assertThat(cn, equalTo("LV"));
   }
}

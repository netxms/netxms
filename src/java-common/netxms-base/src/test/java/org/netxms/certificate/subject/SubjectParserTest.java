package org.netxms.certificate.subject;

import org.junit.Before;
import org.junit.Test;

import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

public class SubjectParserTest
{
   private final String subjectString = "cN=Rage Cage, O=YMCA, st=Kentucky, c=US";
   private Subject subj;

   @Before
   public void setUp() throws Exception
   {
      subj = SubjectParser.parseSubject(subjectString);
   }

   @Test
   public void testParseSubject_testCN() throws Exception
   {
      String cn = subj.getCommonName();

      assertThat(cn, equalTo("Rage Cage"));
   }

   @Test
   public void testParseSubject_testO() throws Exception
   {
      String o = subj.getOrganization();

      assertThat(o, equalTo("YMCA"));
   }

   @Test
   public void testParseSubject_testST() throws Exception
   {
      String st = subj.getState();

      assertThat(st, equalTo("Kentucky"));
   }

   @Test
   public void testParseSubject_testC() throws Exception
   {
      String c = subj.getCountry();

      assertThat(c, equalTo("US"));
   }
}

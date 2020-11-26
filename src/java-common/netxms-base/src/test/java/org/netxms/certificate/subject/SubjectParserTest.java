package org.netxms.certificate.subject;

import org.junit.Before;
import org.junit.Test;
import junit.framework.TestCase;

public class SubjectParserTest extends TestCase
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
      assertTrue(cn.equals("Rage Cage"));
   }

   @Test
   public void testParseSubject_testO() throws Exception
   {
      String o = subj.getOrganization();
      assertTrue(o.equals("YMCA"));
   }

   @Test
   public void testParseSubject_testST() throws Exception
   {
      String st = subj.getState();
      assertTrue(st.equals("Kentucky"));
   }

   @Test
   public void testParseSubject_testC() throws Exception
   {
      String c = subj.getCountry();
      assertTrue(c.equals("US"));
   }
}

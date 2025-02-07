package org.netxms.certificate.subject;

import static org.junit.jupiter.api.Assertions.assertTrue;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

public class SubjectParserTest
{
   private final String subjectString = "cN=Rage Cage, O=YMCA, st=Kentucky, c=US";
   private Subject subj;

   @BeforeEach
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

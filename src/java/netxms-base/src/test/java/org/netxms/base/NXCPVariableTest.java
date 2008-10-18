package org.netxms.base;

import junit.framework.TestCase;

public class NXCPVariableTest extends TestCase 
{
    public void testStringConstruction() 
    {
        final NXCPVariable variable = new NXCPVariable(1, "Sample String");

        assertEquals(1, variable.getVariableId());
        assertEquals(NXCPVariable.TYPE_STRING, variable.getVariableType());
        assertEquals("Sample String", variable.getAsString());
    }

    public void testInt32Construction() 
    {
       final NXCPVariable variable = new NXCPVariable(1, NXCPVariable.TYPE_INTEGER, (long)17);

       assertEquals(1, variable.getVariableId());
       assertEquals(NXCPVariable.TYPE_INTEGER, variable.getVariableType());
       assertEquals(17, variable.getAsInteger().intValue());
   }
}

package org.netxms.base;

import junit.framework.TestCase;

public class NXCPVariableTest extends TestCase {

    public void testStringConstruction() {
        final NXCPVariable variable = new NXCPVariable(1, "Sample String");

        assertEquals(1, variable.getVariableId());
        assertEquals(NXCPVariable.TYPE_STRING, variable.getVariableType());
        assertEquals("Sample String", variable.getAsString());
    }

}

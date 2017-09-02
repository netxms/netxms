package org.netxms.ui.eclipse.datacollection;

import org.eclipse.core.expressions.PropertyTester;
import org.netxms.client.datacollection.DciSummaryTable;

public class DciSummaryTablePropertyTester extends PropertyTester
{
   /* (non-Javadoc)
    * @see org.eclipse.core.expressions.IPropertyTester#test(java.lang.Object, java.lang.String, java.lang.Object[], java.lang.Object)
    */
   @Override
   public boolean test(Object receiver, String property, Object[] args, Object expectedValue)
   {
      if (!(receiver instanceof DciSummaryTable))
         return false;
      
      if (property.equals("isTableSource"))
         return ((DciSummaryTable)receiver).isTableSoure();
      
      return false;
   }
}

package org.netxms.client.objects.interfaces;

import org.netxms.client.objects.AbstractNode;

/**
 * Common methods for all classes hat are node children
 */
public interface NodeChild
{

   /**
    * Get parent node object.
    * 
    * @return parent node object or null if it is not exist or inaccessible
    */
   public AbstractNode getParentNode();
}

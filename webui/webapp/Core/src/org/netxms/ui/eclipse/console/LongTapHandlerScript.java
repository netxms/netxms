/**
 * 
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.rap.ui.resources.IResource;

/**
 * Resource for long tap handler JavaScript
 */
public class LongTapHandlerScript implements IResource
{
   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.resources.IResource#getLoader()
    */
   @Override
   public ClassLoader getLoader()
   {
      return getClass().getClassLoader();
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.resources.IResource#getLocation()
    */
   @Override
   public String getLocation()
   {
      return "js/longpress.js";
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.resources.IResource#isJSLibrary()
    */
   @Override
   public boolean isJSLibrary()
   {
      return true;
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.resources.IResource#isExternal()
    */
   @Override
   public boolean isExternal()
   {
      return false;
   }
}

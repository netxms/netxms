/**
 * 
 */
package org.netxms.ui.eclipse.console.perspectives;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

/**
 * Switcher perspective
 */
public class SwitcherPerspective implements IPerspectiveFactory
{
   /**
    * @see org.eclipse.ui.IPerspectiveFactory#createInitialLayout(org.eclipse.ui.IPageLayout)
    */
   @Override
   public void createInitialLayout(IPageLayout layout)
   {
      layout.setEditorAreaVisible(false);
   }
}

/**
 * 
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.internal.presentations.defaultpresentation.DefaultTabFolder;
import org.eclipse.ui.internal.presentations.util.TabbedStackPresentation;
import org.eclipse.ui.presentations.IStackPresentationSite;
import org.eclipse.ui.presentations.StackPresentation;
import org.eclipse.ui.presentations.WorkbenchPresentationFactory;

/**
 * Custom presentation factory
 */
@SuppressWarnings("restriction")
public class ApplicationPresentationFactory extends WorkbenchPresentationFactory
{
   /* (non-Javadoc)
    * @see org.eclipse.ui.presentations.WorkbenchPresentationFactory#createViewPresentation(org.eclipse.swt.widgets.Composite, org.eclipse.ui.presentations.IStackPresentationSite)
    */
   @Override
   public StackPresentation createViewPresentation(Composite parent, IStackPresentationSite site)
   {
      TabbedStackPresentation p = (TabbedStackPresentation)super.createViewPresentation(parent, site);
      ((DefaultTabFolder)p.getTabFolder()).setUnselectedCloseVisible(true);
      return p;
   }
}

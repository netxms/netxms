/**
 * 
 */
package org.netxms.webui.mobile.widgets;

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;

/**
 * @author Victor
 *
 */
public class NavigationPanel extends Composite
{

   /**
    * @param parent
    * @param style
    */
   public NavigationPanel(Composite parent, int style)
   {
      super(parent, style);
      setBackground(new Color(getDisplay(), 200, 0, 0));
   }

}

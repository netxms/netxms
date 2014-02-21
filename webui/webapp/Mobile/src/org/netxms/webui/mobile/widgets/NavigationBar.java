/**
 * 
 */
package org.netxms.webui.mobile.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;

/**
 * Navigation bar
 */
public class NavigationBar extends Composite
{
   /**
    * @param parent
    */
   public NavigationBar(Composite parent)
   {
      super(parent, SWT.NONE);
      setBackground(new Color(parent.getDisplay(), 203, 184, 154));
      setForeground(new Color(parent.getDisplay(), 88, 96, 109));
      
      GridLayout layout = new GridLayout();
      setLayout(layout);
      
      addElement("Alarms");
      addElement("Dashboard");
   }

   private void addElement(String name)
   {
      Label l = new Label(this, SWT.NONE);
      l.setText(name);
      l.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
   }
}

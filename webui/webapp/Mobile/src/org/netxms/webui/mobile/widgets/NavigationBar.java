/**
 * 
 */
package org.netxms.webui.mobile.widgets;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.action.IAction;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;

/**
 * Navigation bar
 */
public class NavigationBar extends Composite
{
   private Map<String, ActionElement> elements = new HashMap<String, ActionElement>();
   
   /**
    * @param parent
    */
   public NavigationBar(Composite parent)
   {
      super(parent, SWT.NONE);
      setBackground(new Color(parent.getDisplay(), 0, 0, 0));
      setForeground(new Color(parent.getDisplay(), 255, 255, 255));
      
      GridLayout layout = new GridLayout();
      setLayout(layout);
      
      //addAction(action);
   }
   
   public void addAction(IAction action)
   {
      ActionElement e = elements.get(action.getId());
      if (e != null)
      {
         e.update(action);
      }
      else
      {
         elements.put(action.getId(), new ActionElement(this, action));
      }
      layout(true, true);
   }
   
   private class ActionElement extends CLabel
   {
      private IAction action;
      private Image image = null;
      
      ActionElement(Composite parent, IAction action)
      {
         super(parent, SWT.NONE);
         update(action);
      }
      
      void update(IAction action)
      {
         this.action = action;
         if (action.getImageDescriptor() != null)
         {
            image = action.getImageDescriptor().createImage();
            setImage(image);
         }
         else
         {
            setText(action.getText());
            setImage(null);
         }
      }
   }
}

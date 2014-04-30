/**
 * 
 */
package org.netxms.webui.mobile.widgets;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.webui.mobile.Activator;

/**
 * Navigation bar
 */
public class NavigationBar extends Composite
{
   private static final RGB COLOR_BACKGROUND = new RGB(0, 0, 0);
   private static final RGB COLOR_FOREGROUND = new RGB(255, 255, 255);
   
   private Map<String, ActionElement> elements = new HashMap<String, ActionElement>();
   private ColorCache colors;
   
   /**
    * @param parent
    */
   public NavigationBar(Composite parent)
   {
      super(parent, SWT.NONE);
      
      colors = new ColorCache(this);
      
      setBackground(colors.create(COLOR_BACKGROUND));
      setForeground(colors.create(COLOR_FOREGROUND));
      
      GridLayout layout = new GridLayout();
      setLayout(layout);
      
      addAction(new Action("☰") {
         @Override
         public void run()
         {
         }
      });
   }
   
   /**
    * Add action to navigation bar
    * 
    * @param action
    */
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
   
   /**
    * Action element
    */
   private class ActionElement extends CLabel
   {
      private IAction action;
      private Image image = null;
      
      ActionElement(Composite parent, IAction action)
      {
         super(parent, SWT.NONE);
         setBackground(colors.create(COLOR_BACKGROUND));
         setForeground(colors.create(COLOR_FOREGROUND));
         setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
         update(action);
         addMouseListener(new MouseListener() {
            @Override
            public void mouseUp(MouseEvent e)
            {
               ActionElement.this.action.run();
            }
            
            @Override
            public void mouseDown(MouseEvent e)
            {
            }
            
            @Override
            public void mouseDoubleClick(MouseEvent e)
            {
            }
         });
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

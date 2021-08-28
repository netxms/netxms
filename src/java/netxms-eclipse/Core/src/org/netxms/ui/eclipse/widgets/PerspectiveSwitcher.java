/**
 * 
 */
package org.netxms.ui.eclipse.widgets;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.IPerspectiveDescriptor;
import org.eclipse.ui.IPerspectiveListener;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PlatformUI;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Custom perspective switcher
 */
public class PerspectiveSwitcher extends Composite
{
   private IWorkbenchWindow window;
   private Label currentPerspective;
   private ToolBar toolbar;
   private List<String> allowedPerspectives;

   /**
    * @param window
    */
   public PerspectiveSwitcher(IWorkbenchWindow window)
   {
      super((Composite)window.getShell().getChildren()[1], SWT.NONE);
      this.window = window;

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 10;
      setLayout(layout);

      currentPerspective = new Label(this, SWT.RIGHT);
      currentPerspective.setFont(JFaceResources.getBannerFont());
      IPerspectiveDescriptor p = window.getActivePage().getPerspective();
      if (p != null)
         currentPerspective.setText(p.getLabel());

      String v = ConsoleSharedData.getSession().getClientConfigurationHint("AllowedPerspectives");
      if ((v != null) && !v.isEmpty())
      {
         String[] parts = v.split(",");
         allowedPerspectives = new ArrayList<String>(parts.length);
         for(String s : parts)
         {
            if (!s.isBlank())
               allowedPerspectives.add(s.trim().toLowerCase());
         }
      }
      else
      {
         allowedPerspectives = null;
      }

      toolbar = new ToolBar(this, SWT.FLAT);
      buildToolbar();

      window.addPerspectiveListener(new IPerspectiveListener() {
         @Override
         public void perspectiveChanged(IWorkbenchPage page, IPerspectiveDescriptor perspective, String changeId)
         {
         }

         @Override
         public void perspectiveActivated(IWorkbenchPage page, IPerspectiveDescriptor perspective)
         {
            if ((allowedPerspectives == null) || allowedPerspectives.contains(perspective.getLabel().toLowerCase()))
            {
               boolean found = false;
               for(ToolItem i : toolbar.getItems())
               {
                  if (perspective.getId().equals(i.getData("perspective")))
                  {
                     found = true;
                     break;
                  }
               }
               if (!found)
               {
                  for(ToolItem i : toolbar.getItems())
                     i.dispose();
                  buildToolbar();
               }
            }

            currentPerspective.setText(perspective.getLabel());
            window.getShell().layout(true, true);
         }
      });
   }

   /**
    * Build toolbar
    */
   private void buildToolbar()
   {
      IPerspectiveDescriptor[] perspectives = PlatformUI.getWorkbench().getPerspectiveRegistry().getPerspectives();
      if (allowedPerspectives != null)
      {
         Arrays.sort(perspectives, new Comparator<IPerspectiveDescriptor>() {
            @Override
            public int compare(IPerspectiveDescriptor p1, IPerspectiveDescriptor p2)
            {
               return allowedPerspectives.indexOf(p1.getLabel().toLowerCase()) - allowedPerspectives.indexOf(p2.getLabel().toLowerCase());
            }
         });
      }

      for(final IPerspectiveDescriptor p : perspectives)
      {
         if ((allowedPerspectives != null) && !allowedPerspectives.contains(p.getLabel().toLowerCase()))
            continue;

         ToolItem item = new ToolItem(toolbar, SWT.PUSH);
         item.setImage(p.getImageDescriptor().createImage());
         item.setText(p.getLabel());
         item.setToolTipText(p.getLabel());
         item.setData("perspective", p.getId());
         item.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               window.getActivePage().setPerspective(p);
            }
         });
      }
   }
}

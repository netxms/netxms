/**
 * 
 */
package org.netxms.webui.mobile.pages;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.webui.mobile.pages.helpers.ObjectListLabelProvider;

/**
 * @author Victor
 *
 */
public class ObjectBrowser extends AbstractPage
{
   private long rootObjectId;
   private TableViewer viewer;
   
   public ObjectBrowser(long rootObjectId)
   {
      this.rootObjectId = rootObjectId;
   }

   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.pages.AbstractPage#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Composite createContent(Composite parent)
   {
      viewer = new TableViewer(parent, SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ObjectListLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((AbstractObject)e1).getObjectName().compareToIgnoreCase(((AbstractObject)e2).getObjectName());
         }
      });
      
      viewer.addPostSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            AbstractObject o = (AbstractObject)((IStructuredSelection)viewer.getSelection()).getFirstElement();
            handleSelection(o);
         }
      });

      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      viewer.setInput(session.findObjectById(rootObjectId).getChildsAsArray());
      setTitle(session.getObjectName(rootObjectId)); 
      
      return null;
   }

   /**
    * @param object
    */
   private void handleSelection(AbstractObject object)
   {
      if (object instanceof Container)
      {
         getPageManager().openPage(new ObjectBrowser(object.getObjectId()));
      }
      else if (object instanceof Dashboard)
      {
         getPageManager().openPage(new DashboardPage(object.getObjectId()));
      }
   }
}

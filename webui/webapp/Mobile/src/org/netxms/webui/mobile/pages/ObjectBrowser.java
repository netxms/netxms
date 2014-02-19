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
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.webui.mobile.pages.helpers.ObjectListLabelProvider;
import com.eclipsesource.tabris.ui.PageData;

/**
 * Object browser
 */
public class ObjectBrowser extends BasePage
{
   private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   private long rootObjectId = 2;
   private TableViewer viewer;
   
   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.pages.BasePage#createPageContent(org.eclipse.swt.widgets.Composite, com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   public void createPageContent(Composite parent, PageData pageData)
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
      
      Long id = pageData.get("rootObject", Long.class);
      rootObjectId = (id != null) ? id : 2; 
      viewer.setInput(session.findObjectById(rootObjectId).getChildsAsArray());
   }

   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.pages.BasePage#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return session.getObjectName(rootObjectId);
   }
   
   /**
    * @param object
    */
   private void handleSelection(AbstractObject object)
   {
      if (object instanceof Dashboard)
      {
         PageData data = new PageData();
         data.set("object", object.getObjectId());
         openPage("page.dashboard", data);
      }
      else
      {
         PageData data = new PageData();
         data.set("rootObject", object.getObjectId());
         openPage("page.objectBrowser", data);
      }
   }
}

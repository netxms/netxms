package org.netxms.ui.eclipse.perfview.actions;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.perfview.views.StatusIndicatorView;

public class ShowStatusIndicator implements IObjectActionDelegate {

   private IWorkbenchWindow window;
   private Object[] currentSelection = null;
   private long uniqueId = 0;

   @Override
   public void run(IAction action) {
      if (currentSelection != null) {
         String id = Long.toString(uniqueId++);
         for (int i = 0; i < currentSelection.length; i++) {
            long dciId = 0, nodeId = 0;
            int source = 0, dataType = 0;
            String name = null, description = null;

            try {
               if (currentSelection[i] instanceof DciValue) {
                  dciId = ((DciValue) currentSelection[i]).getId();
                  nodeId = ((DciValue) currentSelection[i]).getNodeId();
                  source = ((DciValue) currentSelection[i]).getSource();
                  dataType = ((DciValue) currentSelection[i]).getDataType();
                  name = URLEncoder.encode(((DciValue) currentSelection[i]).getName(), "UTF-8");
                  description = URLEncoder.encode(((DciValue) currentSelection[i]).getDescription(), "UTF-8");
               } else if (currentSelection[i] instanceof DataCollectionItem) {
                  dciId = ((DataCollectionItem) currentSelection[i]).getId();
                  nodeId = ((DataCollectionItem) currentSelection[i]).getNodeId();
                  source = ((DataCollectionItem) currentSelection[i]).getOrigin();
                  dataType = ((DataCollectionItem) currentSelection[i]).getDataType();
                  name = URLEncoder.encode(((DataCollectionItem) currentSelection[i]).getName(), "UTF-8");
                  description = URLEncoder.encode(((DataCollectionItem) currentSelection[i]).getDescription(), "UTF-8");
               }
            } catch (UnsupportedEncodingException e) {
               description = "<description unavailable>";
            }

            id += "&" + Long.toString(nodeId) + "@" + Long.toString(dciId) + "@" + Integer.toString(source) + "@"
                  + Integer.toString(dataType) + "@" + name + "@" + description;
         }

         try {
            window.getActivePage().showView(StatusIndicatorView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
         } catch (PartInitException e) {
            MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
         } catch (IllegalArgumentException e) {
            MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
         }
      }
   }

   @Override
   public void selectionChanged(IAction action, ISelection selection) {
      if ((selection instanceof IStructuredSelection) && !selection.isEmpty()) {
         Object element = ((IStructuredSelection) selection).getFirstElement();
         if ((element instanceof DciValue) || (element instanceof DataCollectionItem)) {
            currentSelection = ((IStructuredSelection) selection).toArray();
         } else {
            currentSelection = null;
         }
      } else {
         currentSelection = null;
      }

      action.setEnabled((currentSelection != null) && (currentSelection.length > 0));
   }

   @Override
   public void setActivePart(IAction action, IWorkbenchPart targetPart) {
      window = targetPart.getSite().getWorkbenchWindow();
   }

}

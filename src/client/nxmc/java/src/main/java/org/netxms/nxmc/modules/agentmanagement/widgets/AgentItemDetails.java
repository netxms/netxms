/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.agentmanagement.widgets;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.Table;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.AgentExplorer;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.AgentDataTreeNode;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.AgentDataTreeNode.NodeType;
import org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer;
import org.netxms.nxmc.modules.datacollection.widgets.TableValueViewer;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Widget showing details of selected agent data item
 */
public class AgentItemDetails extends Composite
{
   private I18n i18n = LocalizationHelper.getI18n(AgentItemDetails.class);

   private AgentExplorer view;
   private LabeledText name;
   private LabeledText type;
   private LabeledText description;
   private LabeledText arguments;
   private Composite valueContainer;
   private Text valueMetric;
   private TableViewer valueList;
   private AgentTableValueViewer valueTable;
   private Control currentValueControl;
   private Button queryButton;
   private AgentDataTreeNode currentNode;
   private NodeType currentNodeType;

   /**
    * Create agent item details widget.
    *
    * @param parent parent composite
    * @param style SWT style
    * @param view parent view
    */
   public AgentItemDetails(Composite parent, int style, AgentExplorer view)
   {
      super(parent, style);
      this.view = view;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = 8;
      setLayout(layout);

      Composite infoContainer = new Composite(this, SWT.NONE);
      infoContainer.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      GridLayout infoLayout = new GridLayout();
      infoLayout.marginWidth = 0;
      infoLayout.marginHeight = 0;
      infoLayout.numColumns = 2;
      infoLayout.makeColumnsEqualWidth = true;
      infoContainer.setLayout(infoLayout);

      name = new LabeledText(infoContainer, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      name.setLabel(i18n.tr("Name"));
      name.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      type = new LabeledText(infoContainer, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      type.setLabel(i18n.tr("Type"));
      type.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      description = new LabeledText(infoContainer, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      description.setLabel(i18n.tr("Description"));
      description.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      Composite queryContainer = new Composite(this, SWT.NONE);
      queryContainer.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      GridLayout queryLayout = new GridLayout();
      queryLayout.numColumns = 2;
      queryLayout.marginWidth = 0;
      queryLayout.marginHeight = 0;
      queryContainer.setLayout(queryLayout);

      queryButton = new Button(queryContainer, SWT.PUSH);
      queryButton.setText(i18n.tr("&Query"));
      GridData gd = new GridData(SWT.LEFT, SWT.BOTTOM, false, false);
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      queryButton.setLayoutData(gd);
      queryButton.setEnabled(false);
      queryButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (currentNode != null)
            {
               view.queryCurrentNode(arguments.getText().trim());
            }
         }
      });

      arguments = new LabeledText(queryContainer, SWT.NONE);
      arguments.setLabel(i18n.tr("Arguments"));
      arguments.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      valueContainer = new Composite(this, SWT.NONE);
      valueContainer.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true, 2, 1));
      GridLayout valueLayout = new GridLayout();
      valueLayout.marginWidth = 0;
      valueLayout.marginHeight = 0;
      valueContainer.setLayout(valueLayout);
   }

   /**
    * Set the currently displayed node.
    *
    * @param node tree node to display
    */
   public void setNode(AgentDataTreeNode node)
   {
      currentNode = node;

      if (node == null)
      {
         name.setText("");
         type.setText("");
         description.setText("");
         queryButton.setEnabled(false);
         currentNodeType = null;
         if (currentValueControl != null)
         {
            currentValueControl.dispose();
            currentValueControl = null;
         }
         return;
      }

      name.setText(node.getName() != null ? node.getName() : node.getDisplayName());
      type.setText(node.getDataType() != null ? node.getDataType() : node.getType().name());
      description.setText(node.getDescription() != null ? node.getDescription() : "");
      queryButton.setEnabled(node.isLeaf());

      if (currentNodeType != node.getType())
      {
         // Dispose previous value control
         if (currentValueControl != null)
         {
            currentValueControl.dispose();
            currentValueControl = null;
         }

         // Create new value control
         switch(node.getType())
         {
            case PARAMETER:
               valueMetric = new Text(valueContainer, SWT.BORDER | SWT.READ_ONLY);
               valueMetric.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
               currentValueControl = valueMetric;
               break;
            case LIST:
               valueList = new TableViewer(valueContainer, SWT.BORDER | SWT.FULL_SELECTION);
               valueList.setContentProvider(new ArrayContentProvider());
               valueList.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
               currentValueControl = valueList.getTable();
               break;
            case TABLE:
               valueTable = new AgentTableValueViewer(valueContainer, SWT.BORDER, view);
               valueTable.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
               currentValueControl = valueTable;
               break;
            default:
               break;
         }
         currentNodeType = node.getType();
         valueContainer.layout();
      }
   }

   /**
    * Set query result.
    *
    * @param value query value
    * @param status query status
    */
   public void setQueryResult(Object value, String status)
   {
      switch(currentNodeType)
      {
         case PARAMETER:
            valueMetric.setText(value != null ? value.toString() : null);
            break;
         case LIST:
            System.out.println("AgentItemDetails: setQueryResult for LIST type is called: " + value);
            valueList.setInput(value);
            break;
         case TABLE:
            valueTable.setData((Table)value);
            valueTable.refresh(null);
            break;
         default:
            break;
      }
   }

   /**
    * Table value viewer for agent data table
    */
   private static class AgentTableValueViewer extends BaseTableValueViewer
   {
      private Table data;

      public AgentTableValueViewer(Composite parent, int style, AgentExplorer view)
      {
         super(parent, style, view, null, false);
      }

      /**
       * Set data to display.
       *
       * @param data data to display
       */
      public void setData(Table data)
      {
         this.data = data;
      }

      /**
       * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#setupLocalization()
       */
      @Override
      protected void setupLocalization()
      {
      }

      /**
       * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#readData()
       */
      @Override
      protected Table readData() throws Exception
      {
         return data;
      }

      /**
       * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#getReadJobName()
       */
      @Override
      protected String getReadJobName()
      {
         return "Reading table";
      }

      /**
       * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#getReadJobErrorMessage()
       */
      @Override
      protected String getReadJobErrorMessage()
      {
         return "Cannot read table";
      }
   }
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.DashboardComposite;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementLayout;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.FontTools;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * Base class for all dashboard elements
 */
public class ElementWidget extends DashboardComposite implements ControlListener, MessageAreaHolder
{
   private static final Logger logger = LoggerFactory.getLogger(ElementWidget.class);

	protected DashboardElement element;
   protected AbstractDashboardView view;

   private DashboardControl dbc;
   private Label title;
   private Composite mainArea;
   private Composite content;
   private Font titleFont;
	private DashboardElementLayout layout;
	private boolean editMode = false;
	private EditPaneWidget editPane = null;
   private MessageArea messageArea;
   private FilterText filterText = null;
   private boolean hasFilter = false;

   /**
    * @param parent
    * @param style
    * @param element
    * @param view
    */
   protected ElementWidget(DashboardControl parent, int style, DashboardElement element, AbstractDashboardView view)
	{
		super(parent, style);
		dbc = parent;
		this.element = element;
      this.view = view;
      setupElement();
	}

   /**
    * @param parent
    * @param element
    * @param view
    */
   protected ElementWidget(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
		super(parent, SWT.BORDER);
		dbc = parent;
		this.element = element;
      this.view = view;
      setupElement();
	}

   /**
    * @param parent
    * @param element
    * @param view
    * @param hasFilter
    */
   protected ElementWidget(DashboardControl parent, DashboardElement element, AbstractDashboardView view, boolean hasFilter)
   {
      super(parent, SWT.BORDER);
      dbc = parent;
      this.element = element;
      this.view = view;
      this.hasFilter = hasFilter;
      setupElement();
   }

   /**
    * Setup this element
    */
   private void setupElement()
   {
      parseLayout(element.getLayout());
      addControlListener(this);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 4;
      setLayout(layout);

      mainArea = new Composite(this, SWT.NONE);
      mainArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      mainArea.setLayout(new FormLayout());
      mainArea.setBackground(getBackground());

      messageArea = new MessageArea(mainArea, SWT.NONE);
      if (hasFilter)
      {
         filterText = new FilterText(mainArea, SWT.NONE, null, false, false);
         filterText.addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               onFilterModify();
            }
         });
      }

      content = new Composite(mainArea, SWT.NONE) {
         @Override
         public Point computeSize(int wHint, int hHint, boolean changed)
         {
            Point size = super.computeSize(wHint, hHint, changed);
            if (hHint == SWT.DEFAULT)
            {
               int mh = getPreferredHeight();
               if (mh > size.y)
                  size.y = mh;
            }
            return size;
         }
      };
      content.setLayout(new FillLayout());
      content.setBackground(getBackground());

      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      messageArea.setLayoutData(fd);

      if (hasFilter)
      {
         fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(messageArea);
         fd.right = new FormAttachment(100, 0);
         filterText.setLayoutData(fd);

         fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(filterText);
         fd.right = new FormAttachment(100, 0);
         fd.bottom = new FormAttachment(100, 0);
         content.setLayoutData(fd);
      }
      else
      {
         fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(messageArea);
         fd.right = new FormAttachment(100, 0);
         fd.bottom = new FormAttachment(100, 0);
         content.setLayoutData(fd);
      }

      addDisposeListener((e) -> {
         if (titleFont != null)
            titleFont.dispose();
      });
   }

   /**
    * Handler for filter modification
    */
   protected void onFilterModify()
   {
   }

   /**
    * Get content area for placing element's widgets.
    *
    * @return content area for placing element's widgets
    */
   protected Composite getContentArea()
   {
      return content;
   }

   /**
    * Get content area for placing element's widgets.
    *
    * @return content area for placing element's widgets
    */
   protected Composite getMainArea()
   {
      return mainArea;
   }

   /**
    * Get preferred component height - used when component is not required to fill all available space.
    *
    * @return preferred component height
    */
   protected int getPreferredHeight()
   {
      return 300;
   }

   /**
    * Set element's title.
    *
    * @param text title text
    * @param backgroundColor background color or null
    * @param foregroundColor foreground color or null
    * @param textSizeAdjustment text size adjustment
    * @param fontName font name or null
    */
   protected void setTitle(String text, RGB backgroundColor, RGB foregroundColor, int textSizeAdjustment, String fontName)
   {
      if (title != null)
         title.dispose();

      title = new Label(this, SWT.CENTER);
      title.setText(text.replace("&", "&&"));

      if (backgroundColor != null)
      {
         title.setBackground(colors.create(backgroundColor));
         setBackground(title.getBackground());
      }
      else
      {
         title.setBackground(getBackground());
      }

      if (foregroundColor != null)
         title.setForeground(colors.create(foregroundColor));

      if (titleFont != null)
         titleFont.dispose();
      if ((fontName != null) && !fontName.isEmpty())
      {
         Font systemFont = JFaceResources.getBannerFont();
         titleFont = FontTools.createFont(new String[] { fontName }, JFaceResources.getBannerFont(), textSizeAdjustment, systemFont.getFontData()[0].getStyle());
         title.setFont((titleFont != null) ? titleFont : systemFont);
      }
      else if (textSizeAdjustment != 0)
      {
         titleFont = FontTools.createAdjustedFont(JFaceResources.getBannerFont(), textSizeAdjustment);
         title.setFont(titleFont);
      }
      else
      {
         titleFont = null;
         title.setFont(JFaceResources.getBannerFont());
      }

      GridData gd = new GridData(SWT.CENTER, SWT.CENTER, true, false);
      gd.verticalIndent = 4;
      title.setLayoutData(gd);
      title.moveAbove(null);
      layout(true, true);
   }

   /**
    * Process settings common for all elements.
    *
    * @param config dashboard element configuration
    */
   protected void processCommonSettings(DashboardElementConfig config)
   {
      if (!config.getTitle().isEmpty())
         setTitle(config.getTitle(), ColorConverter.parseColorDefinition(config.getTitleBackground()), ColorConverter.parseColorDefinition(config.getTitleForeground()), config.getTitleFontSize(), config.getTitleFontName());
   }

	/**
	 * @param xml
	 */
	private void parseLayout(String xml)
	{
		try
		{
         layout = new Gson().fromJson(xml, DashboardElementLayout.class);
		}
		catch(Exception e)
		{
         logger.error("Cannot parse dashboard element layout", e);
			layout = new DashboardElementLayout();
		}
	}

	/**
	 * @return the layout
	 */
	public DashboardElementLayout getElementLayout()
	{
		return layout;
	}

	/**
	 * @return the editMode
	 */
	public boolean isEditMode()
	{
		return editMode;
	}

	/**
	 * @param editMode the editMode to set
	 */
	public void setEditMode(boolean editMode)
	{
		this.editMode = editMode;
		if (editMode)
      {
			editPane = new EditPaneWidget(this, dbc, element);
			editPane.setLocation(0,  0);
			editPane.setSize(getSize());
			editPane.moveAbove(null);
		}
      else if (editPane != null)
		{
         editPane.dispose();
         editPane = null;
		}
	}

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   protected void enableFilter(boolean enable)
   {
      if (!hasFilter)
         return;

      filterText.setVisible(enable);
      FormData fd = (FormData)content.getLayoutData();
      fd.top = new FormAttachment(enable ? filterText : messageArea);
      mainArea.layout(true, true);
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText("");
         onFilterModify();
      }
   }

   /**
    * Get filter text.
    *
    * @return filter text
    */
   protected String getFilterText()
   {
      return hasFilter ? filterText.getText() : "";
   }

   /**
    * @see org.eclipse.swt.events.ControlListener#controlMoved(org.eclipse.swt.events.ControlEvent)
    */
	@Override
	public void controlMoved(ControlEvent e)
	{
	}

   /**
    * @see org.eclipse.swt.events.ControlListener#controlResized(org.eclipse.swt.events.ControlEvent)
    */
	@Override
	public void controlResized(ControlEvent e)
	{
		if (editPane != null)
		{
			editPane.setLocation(0,  0);
			editPane.setSize(getSize());
			editPane.moveAbove(null);
		}
	}

	/**
	 * Request layout run for entire dashboard. Can be called by subclasses when complete dashboard layout re-run is needed. 
	 */
	protected void requestDashboardLayout()
	{
      dbc.requestDashboardLayout();
	}

   /**
    * Get ID of owning dashboard object.
    *
    * @return ID of owning dashboard object
    */
   protected long getDashboardObjectId()
   {
      return dbc.getDashboardObject().getObjectId();
   }

   /**
    * Get context for owning dashboard.
    *
    * @return context for owning dashboard (can be null)
    */
   protected AbstractObject getContext()
   {
      return dbc.getContext();
   }

   /**
    * Check if dashboard is in narrow screen mode.
    *
    * @return true if dashboard is in narrow screen mode
    */
   protected boolean isNarrowScreenMode()
   {
      return dbc.isNarrowScreenMode();
   }

   /**
    * Get ID of context object.
    *
    * @return ID of context object or 0 if context is not set
    */
   protected long getContextObjectId()
   {
      AbstractObject object = dbc.getContext();
      return (object != null) ? object.getObjectId() : 0;
   }

   /**
    * Get effective object ID - if suppliet object ID is a context placeholder, returns ID of current context object, otherwise
    * supplied ID itself.
    * 
    * @param objectId object ID to check
    * @return supplied object ID or context object ID if supplied ID is context placeholder
    */
   protected long getEffectiveObjectId(long objectId)
   {
      return (objectId == AbstractObject.CONTEXT) ? getContextObjectId() : objectId;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String)
    */
   @Override
   public int addMessage(int level, String text)
   {
      return messageArea.addMessage(level, text);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String, boolean)
    */
   @Override
   public int addMessage(int level, String text, boolean sticky)
   {
      return messageArea.addMessage(level, text, sticky);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String, boolean, java.lang.String, java.lang.Runnable)
    */
   @Override
   public int addMessage(int level, String text, boolean sticky, String buttonText, Runnable action)
   {
      return messageArea.addMessage(level, text, sticky, buttonText, action);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#deleteMessage(int)
    */
   @Override
   public void deleteMessage(int id)
   {
      messageArea.deleteMessage(id);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#clearMessages()
    */
   @Override
   public void clearMessages()
   {
      messageArea.clearMessages();
   }
}

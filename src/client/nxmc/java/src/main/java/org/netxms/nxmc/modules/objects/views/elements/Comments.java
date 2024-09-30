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
package org.netxms.nxmc.modules.objects.views.elements;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.MarkdownViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectOverviewView;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * Show object's comments
 */
public class Comments extends OverviewPageElement
{
   private final I18n i18n = LocalizationHelper.getI18n(Comments.class);

   private MarkdownViewer markdownViewer;
   private Text textViewer;
   private Control currentViewer = null;
   private Composite content;
   private boolean markdownMode = false;
   private boolean showEmptyComments;

	/**
	 * The constructor
	 * 
	 * @param parent
	 * @param anchor
	 * @param objectView
	 */
   public Comments(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
		super(parent, anchor, objectView);
      showEmptyComments = !Registry.getSession().getClientConfigurationHintAsBoolean("ObjectOverview.ShowCommentsOnlyIfPresent", true);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createClientArea(Composite parent)
	{
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FillLayout());
      onObjectChange();
      return content;
	}

   /**
    * Create viewer depending on mode
    * 
    * @param markdownMode true if viewer should be for markdown
    */
   private void createViewer(boolean markdownMode)
   {
      if ((currentViewer != null) && !currentViewer.isDisposed())
      {
         if (this.markdownMode == markdownMode)
            return; // Already in correct mode
         currentViewer.dispose();
      }

      if (markdownMode)
      {
         markdownViewer = new MarkdownViewer(content, SWT.NONE);
         markdownViewer.setRenderCompletionHandler(() -> {
            ObjectView view = getObjectView();
            if (view != null)
               ((ObjectOverviewView)view).requestElementLayout();
         });
         currentViewer = markdownViewer;
      }
      else
      {
         textViewer = new Text(content, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
         textViewer.setFont(JFaceResources.getDialogFont());
         currentViewer = textViewer;
      }
      currentViewer.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      this.markdownMode = markdownMode;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("Comments");
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#onObjectChange()
    */
	@Override
	protected void onObjectChange()
	{
      if (getObject() == null)
         return;

      String comments = getObject().getComments();
      if (comments.startsWith(AbstractObject.MARKDOWN_COMMENTS_INDICATOR))
      {
         createViewer(true);
         markdownViewer.setText(comments.substring(AbstractObject.MARKDOWN_COMMENTS_INDICATOR.length()));
      }
      else
      {
         createViewer(false);
         textViewer.setText(comments);
      }
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      if (showEmptyComments)
         return true;
      String comments = object.getComments();
      return !comments.isBlank() && !(comments.startsWith(AbstractObject.MARKDOWN_COMMENTS_INDICATOR) && comments.substring(AbstractObject.MARKDOWN_COMMENTS_INDICATOR.length()).isBlank());
   }
}

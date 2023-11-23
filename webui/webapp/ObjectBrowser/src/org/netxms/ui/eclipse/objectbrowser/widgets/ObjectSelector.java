/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Object selector
 */
public class ObjectSelector extends AbstractSelector
{
	private long objectId = 0;
	private AbstractObject object = null;
	private Set<Class<? extends AbstractObject>> objectClassSet = new HashSet<Class<? extends AbstractObject>>();
	private Set<Integer> classFilter = null;
   private String emptySelectionName = Messages.get().ObjectSelector_None;
   private String contextSelectionName = "<context>";
   private WorkbenchLabelProvider labelProvider = new WorkbenchLabelProvider();

   /**
    * @param parent parent composite
    * @param style control style
    * @param options selector option
    */
   public ObjectSelector(Composite parent, int style, int options)
   {
      super(parent, style, options);
      setText(emptySelectionName);
      objectClassSet.add(Node.class);
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            labelProvider.dispose();
         }
      });
   }

   /**
    * @param parent parent composite
    * @param style control style
    * @param showClearButton true to show "Clear" button
    * @param showContextButton true to show "Context" button
    */
   public ObjectSelector(Composite parent, int style, boolean showClearButton, boolean showContextButton)
   {
      this(parent, style, (showClearButton ? SHOW_CLEAR_BUTTON : 0) | (showContextButton ? SHOW_CONTEXT_BUTTON : 0));
   }

   /**
    * @param parent parent composite
    * @param style control style
    * @param showClearButton true to show "Clear" button
    */
   public ObjectSelector(Composite parent, int style, boolean showClearButton)
   {
      this(parent, style, showClearButton, false);
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
    */
	@Override
	protected void selectionButtonHandler()
	{
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), classFilter);
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
         AbstractObject[] objects = (objectClassSet != null) ? dlg.getSelectedObjects(objectClassSet) : dlg.getSelectedObjects().toArray(AbstractObject[]::new);
			if (objects.length > 0)
			{
				objectId = objects[0].getObjectId();
				object = objects[0];
            setText(objects[0].getNameWithAlias());
            setImage(labelProvider.getImage(object));
			}
			else
			{
				objectId = 0;
				object = null;
				setText(emptySelectionName);
            setImage(null);
			}
			fireModifyListeners();
		}
	}

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#contextButtonHandler()
    */
   @Override
   protected void contextButtonHandler()
   {
      objectId = AbstractObject.CONTEXT;
      object = null;
      setText(contextSelectionName);
      setImage(null);
      fireModifyListeners();
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
    */
	@Override
	protected void clearButtonHandler()
	{
		objectId = 0;
      object = null;
		setText(emptySelectionName);
      setImage(null);
		fireModifyListeners();
	}

	/**
    * Get ID of selected object.
    *
    * @return ID of selected object
    */
	public long getObjectId()
	{
		return objectId;
	}
	
   /**
    * Get selected object.
    *
    * @return selected object
    */
	public AbstractObject getObject()
	{
	   return object;
	}

	/**
	 * @return the object name
	 */
	public String getObjectName()
	{
		return getText();
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setObjectId(long objectId)
	{
		this.objectId = objectId;
		if (objectId == 0)
		{
         setText(emptySelectionName);
         setImage(null);
		}
      else if (objectId == AbstractObject.CONTEXT)
      {
         setText(contextSelectionName);
         setImage(null);
      }
		else
		{
			object = ConsoleSharedData.getSession().findObjectById(objectId);
			setText((object != null) ? object.getObjectName() : ("<" + Long.toString(objectId) + ">")); //$NON-NLS-1$ //$NON-NLS-2$
         setImage((object != null) ? labelProvider.getImage(object) : SharedIcons.IMG_UNKNOWN_OBJECT);
		}
	}

	/**
	 * @return the objectClass
	 */
	public Class<? extends AbstractObject> getObjectClass()
	{
		return objectClassSet.iterator().next();
	}

	/**
	 * @param objectClass the objectClass to set
	 */
	public void setObjectClass(Class<? extends AbstractObject> objectClass)
	{
	   objectClassSet.clear();
		objectClassSet.add(objectClass);
	}

	/**
	 * Set object Class set
	 * 
	 * @param filterSet object class set
	 */
   public void setObjectClass(Set<Class<? extends AbstractObject>> filterSet)
   {
      objectClassSet = filterSet;
   }

   /**
    * Remove class filter for object selection (object of any class could be selected).
    */
   public void removeSelectionClassFilter()
   {
      objectClassSet = null;
   }

	/**
	 * @return the emptySelectionName
	 */
	public String getEmptySelectionName()
	{
		return emptySelectionName;
	}

	/**
	 * @param emptySelectionName the emptySelectionName to set
	 */
	public void setEmptySelectionName(String emptySelectionName)
	{
		this.emptySelectionName = emptySelectionName;
      if (objectId == 0)
         setText(emptySelectionName);
	}

	/**
    * @return the contextSelectionName
    */
   public String getContextSelectionName()
   {
      return contextSelectionName;
   }

   /**
    * @param contextSelectionName the contextSelectionName to set
    */
   public void setContextSelectionName(String contextSelectionName)
   {
      this.contextSelectionName = contextSelectionName;
   }

   /**
    * @return the classFilter
    */
	public Set<Integer> getClassFilter()
	{
		return classFilter;
	}

	/**
	 * @param classFilter the classFilter to set
	 */
	public void setClassFilter(Set<Integer> classFilter)
	{
		this.classFilter = classFilter;
	}
}

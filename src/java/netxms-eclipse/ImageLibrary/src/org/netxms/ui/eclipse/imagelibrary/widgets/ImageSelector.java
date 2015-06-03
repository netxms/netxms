/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.imagelibrary.widgets;

import java.util.UUID;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.NXCommon;
import org.netxms.client.LibraryImage;
import org.netxms.ui.eclipse.imagelibrary.Messages;
import org.netxms.ui.eclipse.imagelibrary.dialogs.ImageSelectionDialog;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageUpdateListener;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Image selector
 */
public class ImageSelector extends AbstractSelector implements ImageUpdateListener
{
	private UUID imageGuid = NXCommon.EMPTY_GUID;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ImageSelector(Composite parent, int style)
	{
		super(parent, style, SHOW_CLEAR_BUTTON);
		ImageProvider.getInstance().addUpdateListener(this);
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				ImageProvider.getInstance().removeUpdateListener(ImageSelector.this);
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		ImageSelectionDialog dlg = new ImageSelectionDialog(getShell());
		if (dlg.open() == Window.OK)
		{
			LibraryImage image = dlg.getLibraryImage();
			if (image != null)
			{
				setText(image.getName());
				setImage(dlg.getImage());
				imageGuid = dlg.getGuid();
			}
			else
			{
				setText(Messages.get().ImageSelector_Default);
				setImage(null);
				imageGuid = NXCommon.EMPTY_GUID;
			}
			getParent().layout();
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
	 */
	@Override
	protected void clearButtonHandler()
	{
		setText(Messages.get().ImageSelector_Default);
		setImage(null);
		imageGuid = NXCommon.EMPTY_GUID;
		getParent().layout();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getSelectionButtonToolTip()
	 */
	@Override
	protected String getSelectionButtonToolTip()
	{
		return Messages.get().ImageSelector_SelectImage;
	}

	/**
	 * @return the imageGuid
	 */
	public UUID getImageGuid()
	{
		return imageGuid;
	}

	/**
	 * Set image GUID
	 * 
	 * @param imageGuid image GUID
	 * @param redoLayout if set to true, control will update it's layout
	 */
	public void setImageGuid(UUID imageGuid, boolean redoLayout)
	{
		this.imageGuid = imageGuid;
		if (imageGuid.equals(NXCommon.EMPTY_GUID))
		{
			setText(Messages.get().ImageSelector_Default);
			setImage(null);
		}
		else
		{
			LibraryImage image = ImageProvider.getInstance().getLibraryImageObject(imageGuid);
			if (image != null)
			{
				setText(image.getName());
				setImage(ImageProvider.getInstance().getImage(imageGuid));
			}
			else
			{
				setText("<?>" + imageGuid.toString()); //$NON-NLS-1$
				setImage(null);
			}
		}
		
		if (redoLayout)
			getParent().layout();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.imagelibrary.shared.ImageUpdateListener#imageUpdated(java.util.UUID)
	 */
	@Override
	public void imageUpdated(UUID guid)
	{
		if (guid.equals(imageGuid))
		{
		   getDisplay().asyncExec(new Runnable() {
            
            @Override
            public void run()
            {
               setImage(ImageProvider.getInstance().getImage(imageGuid));
               getParent().layout();
            }
         });
		}
	}
}

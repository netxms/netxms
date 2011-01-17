package org.netxms.ui.eclipse.imagelibrary.views;

import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;

public class ImageLibrary extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.view.imagelibrary";

	@Override
	public void createPartControl(Composite parent)
	{
		FormLayout formLayout = new FormLayout();
		parent.setLayout(formLayout);

		createMenu();
		createPopupMenu();
	}

	@Override
	public void setFocus()
	{
	}

	private void createPopupMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});
	}

	private void createMenu()
	{
	}

	protected void fillContextMenu(IMenuManager mgr)
	{
	}

}

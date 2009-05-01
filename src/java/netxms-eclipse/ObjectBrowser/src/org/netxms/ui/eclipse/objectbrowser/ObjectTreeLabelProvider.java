package org.netxms.ui.eclipse.objectbrowser;

import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.*;

public class ObjectTreeLabelProvider extends LabelProvider
{
	// Object images
	private Image imageNetwork;
	private Image imageSubnet;
	private Image imageNode;
	private Image imageInterface;
	private Image imageServiceRoot;
	private Image imageContainer;
	private Image imageCondition;
	private Image imageTemplate;
	private Image imageTemplateGroup;
	private Image imageTemplateRoot;
	
	/**
	 * Default constructor 
	 */
	public ObjectTreeLabelProvider()
	{
		super();
		imageNetwork = Activator.getImageDescriptor("icons/network.png").createImage();
		imageServiceRoot = Activator.getImageDescriptor("icons/service_root.png").createImage();
		imageContainer = Activator.getImageDescriptor("icons/container.png").createImage();
		imageSubnet = Activator.getImageDescriptor("icons/subnet.png").createImage();
		imageNode = Activator.getImageDescriptor("icons/node.png").createImage();
		imageInterface = Activator.getImageDescriptor("icons/interface.png").createImage();
		imageCondition = Activator.getImageDescriptor("icons/condition.png").createImage();
		imageTemplate = Activator.getImageDescriptor("icons/template.png").createImage();
		imageTemplateGroup = Activator.getImageDescriptor("icons/template_group.png").createImage();
		imageTemplateRoot = Activator.getImageDescriptor("icons/template_root.png").createImage();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
	 */
	@Override
	public Image getImage(Object element)
	{
		if (element == null)
			return null;
		
		switch(((NXCObject)element).getObjectClass())
		{
			case NXCObject.OBJECT_NETWORK:
				return imageNetwork;
			case NXCObject.OBJECT_SERVICEROOT:
				return imageServiceRoot;
			case NXCObject.OBJECT_CONTAINER:
				return imageContainer;
			case NXCObject.OBJECT_SUBNET:
				return imageSubnet;
			case NXCObject.OBJECT_NODE:
				return imageNode;
			case NXCObject.OBJECT_INTERFACE:
				return imageInterface;
			case NXCObject.OBJECT_CONDITION:
				return imageCondition;
			case NXCObject.OBJECT_TEMPLATEROOT:
				return imageTemplateRoot;
			case NXCObject.OBJECT_TEMPLATEGROUP:
				return imageTemplateGroup;
			case NXCObject.OBJECT_TEMPLATE:
				return imageTemplate;
			default:
				return super.getImage(element);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
	 */
	@Override
	public String getText(Object element)
	{
		return (element != null) ? ((NXCObject)element).getObjectName() : "<null>";
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		imageNetwork.dispose();
		imageServiceRoot.dispose();
		imageContainer.dispose();
		imageSubnet.dispose();
		imageNode.dispose();
		imageInterface.dispose();
		imageCondition.dispose();
		imageTemplateRoot.dispose();
		imageTemplateGroup.dispose();
		imageTemplate.dispose();
		super.dispose();
	}
}

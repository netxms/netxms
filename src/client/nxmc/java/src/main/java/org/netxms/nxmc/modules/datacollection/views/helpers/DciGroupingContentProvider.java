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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import java.util.IdentityHashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Content provider for data collection views that groups elements (DCI values or data collection objects) according to selected
 * grouping mode. Elements without value for the grouping attribute (for example DCIs without tag when grouping by tag) are placed
 * into fallback group.
 */
public class DciGroupingContentProvider implements ITreeContentProvider
{
   /**
    * Resolver for data collection objects referenced by displayed elements (used to find instance discovery prototypes).
    */
   public interface DciResolver
   {
      /**
       * Find data collection object.
       *
       * @param ownerId DCI owner ID
       * @param dciId DCI ID
       * @return data collection object or null if not available
       */
      public DataCollectionObject find(long ownerId, long dciId);
   }

   private final I18n i18n = LocalizationHelper.getI18n(DciGroupingContentProvider.class);

   private NXCSession session = Registry.getSession();
   private DciGroupingMode mode = DciGroupingMode.NONE;
   private DciResolver resolver;
   private Map<Object, DciGroup> parents = new IdentityHashMap<>();

   /**
    * Create content provider.
    *
    * @param resolver resolver for referenced data collection objects (may be null)
    */
   public DciGroupingContentProvider(DciResolver resolver)
   {
      this.resolver = resolver;
   }

   /**
    * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
    */
   @Override
   public Object[] getElements(Object inputElement)
   {
      parents.clear();
      if (!(inputElement instanceof Object[]))
         return new Object[0];

      Object[] input = (Object[])inputElement;
      if (mode == DciGroupingMode.NONE)
         return input;

      Map<DciGroup, DciGroup> groups = new LinkedHashMap<>();
      for(Object e : input)
      {
         DciGroup group = createGroup(e);
         DciGroup existing = groups.get(group);
         if (existing == null)
         {
            groups.put(group, group);
            existing = group;
         }
         existing.add(e);
         parents.put(e, existing);
      }
      return groups.keySet().toArray();
   }

   /**
    * Create group descriptor for given element according to current grouping mode.
    *
    * @param e element
    * @return group descriptor
    */
   private DciGroup createGroup(Object e)
   {
      switch(mode)
      {
         case INSTANCE_DISCOVERY:
            if (!isInstance(e))
               return new DciGroup(i18n.tr("(no instance discovery)"));
            long ownerId = getOwnerId(e);
            long prototypeId = getTemplateItemId(e);
            DataCollectionObject prototype = (resolver != null) ? resolver.find(ownerId, prototypeId) : null;
            return new DciGroup(Long.toString(ownerId) + "/" + Long.toString(prototypeId), (prototype != null) ? prototype.getDescription() : "[" + prototypeId + "]");
         case ORIGIN:
            DataOrigin origin = (e instanceof DciValue) ? ((DciValue)e).getSource() : ((DataCollectionObject)e).getOrigin();
            return new DciGroup(origin, DataCollectionDisplayInfo.getOriginName(origin));
         case RELATED_OBJECT:
            long relatedObjectId = (e instanceof DciValue) ? ((DciValue)e).getRelatedObject() : ((DataCollectionObject)e).getRelatedObject();
            return (relatedObjectId != 0) ? new DciGroup(relatedObjectId, getObjectName(relatedObjectId)) : new DciGroup(i18n.tr("(no related object)"));
         case STATUS:
            DataCollectionObjectStatus status = (e instanceof DciValue) ? ((DciValue)e).getStatus() : ((DataCollectionObject)e).getStatus();
            return new DciGroup(status, DataCollectionDisplayInfo.getStatusName(status));
         case TAG:
            String tag = (e instanceof DciValue) ? ((DciValue)e).getUserTag() : ((DataCollectionObject)e).getUserTag();
            return ((tag != null) && !tag.isEmpty()) ? new DciGroup(tag, tag) : new DciGroup(i18n.tr("(untagged)"));
         case TEMPLATE:
            long templateId = getSourceTemplateId(e);
            return (templateId != 0) ? new DciGroup(templateId, getObjectName(templateId)) : new DciGroup(i18n.tr("(no template)"));
         default:
            throw new IllegalStateException("Unexpected grouping mode " + mode);
      }
   }

   /**
    * Get ID of DCI owner.
    */
   private static long getOwnerId(Object e)
   {
      return (e instanceof DciValue) ? ((DciValue)e).getNodeId() : ((DataCollectionObject)e).getNodeId();
   }

   /**
    * Get template ID of given element.
    */
   private static long getTemplateId(Object e)
   {
      return (e instanceof DciValue) ? ((DciValue)e).getTemplateId() : ((DataCollectionObject)e).getTemplateId();
   }

   /**
    * Get template item ID of given element.
    */
   private static long getTemplateItemId(Object e)
   {
      return (e instanceof DciValue) ? ((DciValue)e).getTemplateDciId() : ((DataCollectionObject)e).getTemplateItemId();
   }

   /**
    * Check if given element was created by instance discovery.
    */
   private static boolean isInstance(Object e)
   {
      long templateId = getTemplateId(e);
      return (templateId != 0) && (templateId == getOwnerId(e)) && (getTemplateItemId(e) != 0);
   }

   /**
    * Get ID of template (or cluster) given element originates from. For DCIs created by instance discovery template of the
    * prototype DCI is used.
    */
   private long getSourceTemplateId(Object e)
   {
      if (!isInstance(e))
         return getTemplateId(e);
      if (resolver == null)
         return 0;
      DataCollectionObject prototype = resolver.find(getOwnerId(e), getTemplateItemId(e));
      return (prototype != null) ? prototype.getTemplateId() : 0;
   }

   /**
    * Get object name for group display.
    */
   private String getObjectName(long objectId)
   {
      AbstractObject object = session.findObjectById(objectId);
      return (object != null) ? object.getObjectName() : "[" + objectId + "]";
   }

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
    */
   @Override
   public Object[] getChildren(Object parentElement)
   {
      return (parentElement instanceof DciGroup) ? ((DciGroup)parentElement).getElements() : new Object[0];
   }

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
    */
   @Override
   public Object getParent(Object element)
   {
      return parents.get(element);
   }

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
    */
   @Override
   public boolean hasChildren(Object element)
   {
      return element instanceof DciGroup;
   }

   /**
    * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
   {
      parents.clear();
   }

   /**
    * @see org.eclipse.jface.viewers.IContentProvider#dispose()
    */
   @Override
   public void dispose()
   {
   }

   /**
    * Get current grouping mode.
    *
    * @return current grouping mode
    */
   public DciGroupingMode getMode()
   {
      return mode;
   }

   /**
    * Set grouping mode. Caller is responsible for refreshing the viewer.
    *
    * @param mode new grouping mode
    */
   public void setMode(DciGroupingMode mode)
   {
      this.mode = mode;
   }
}

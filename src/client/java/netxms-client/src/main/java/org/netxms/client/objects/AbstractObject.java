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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.netxms.base.GeoLocation;
import org.netxms.base.Logger;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;
import org.netxms.base.PostalAddress;
import org.netxms.base.annotations.Internal;
import org.netxms.client.AccessListElement;
import org.netxms.client.ModuleDataProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectUrl;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.services.ServiceManager;

/**
 * Abstract base class for all NetXMS objects (both built-in and provided by extensions)
 */
public abstract class AbstractObject
{
	/** Entire network */
	public static final int NETWORK = 1;
	/** Infrastructure Services */
	public static final int SERVICEROOT = 2;
	/** Templates */
	public static final int TEMPLATEROOT = 3;
	/** Default zone */
	public static final int ZONE0 = 4;
	/** Configuration Policies */
	public static final int POLICYROOT = 5;
	/** Network Maps */
	public static final int NETWORKMAPROOT = 6;
	/** Dashboards */
	public static final int DASHBOARDROOT = 7;
	/** Reports */
	public static final int REPORTROOT = 8;
	/** Business Services */
	public static final int BUSINESSSERVICEROOT = 9;
	
	// Object classes
	public static final int OBJECT_GENERIC = 0;
	public static final int OBJECT_SUBNET = 1;
	public static final int OBJECT_NODE = 2;
	public static final int OBJECT_INTERFACE = 3;
	public static final int OBJECT_NETWORK = 4;
	public static final int OBJECT_CONTAINER = 5;
	public static final int OBJECT_ZONE = 6;
	public static final int OBJECT_SERVICEROOT = 7;
	public static final int OBJECT_TEMPLATE = 8;
	public static final int OBJECT_TEMPLATEGROUP = 9;
	public static final int OBJECT_TEMPLATEROOT = 10;
	public static final int OBJECT_NETWORKSERVICE = 11;
	public static final int OBJECT_VPNCONNECTOR = 12;
	public static final int OBJECT_CONDITION = 13;
	public static final int OBJECT_CLUSTER = 14;
	public static final int OBJECT_POLICYGROUP = 15;
	public static final int OBJECT_POLICYROOT = 16;
	public static final int OBJECT_AGENTPOLICY = 17;
	public static final int OBJECT_AGENTPOLICY_CONFIG = 18;
	public static final int OBJECT_NETWORKMAPROOT = 19;
	public static final int OBJECT_NETWORKMAPGROUP = 20;
	public static final int OBJECT_NETWORKMAP = 21;
	public static final int OBJECT_DASHBOARDROOT = 22;
	public static final int OBJECT_DASHBOARD = 23;
	public static final int OBJECT_BUSINESSSERVICEROOT = 27;
	public static final int OBJECT_BUSINESSSERVICE = 28;
	public static final int OBJECT_NODELINK = 29;
	public static final int OBJECT_SLMCHECK = 30;
	public static final int OBJECT_MOBILEDEVICE = 31;
	public static final int OBJECT_RACK = 32;
	public static final int OBJECT_ACCESSPOINT = 33;
	public static final int OBJECT_AGENTPOLICY_LOGPARSER = 34;
   public static final int OBJECT_CHASSIS = 35;
   public static final int OBJECT_DASHBOARDGROUP = 36;
   public static final int OBJECT_SENSOR = 37;
	public static final int OBJECT_CUSTOM = 10000;
	
	// Status calculation methods
	public static final int CALCULATE_DEFAULT = 0;
	public static final int CALCULATE_MOST_CRITICAL = 1;
	public static final int CALCULATE_SINGLE_THRESHOLD = 2;
	public static final int CALCULATE_MULTIPLE_THRESHOLDS = 3;
	
	// Status propagation methods
	public static final int PROPAGATE_DEFAULT = 0;
	public static final int PROPAGATE_UNCHANGED = 1;
	public static final int PROPAGATE_FIXED = 2;
	public static final int PROPAGATE_RELATIVE = 3;
	public static final int PROPAGATE_TRANSLATED = 4;
	
	@Internal protected NXCSession session = null;
	protected long objectId = 0;
	protected UUID guid;
	protected String objectName;
	protected int objectClass;
	protected ObjectStatus status = ObjectStatus.UNKNOWN;
	protected boolean isDeleted = false;
	protected boolean inMaintenanceMode = false;
	protected long primaryZoneProxyId = 0;
   protected long backupZoneProxyId = 0;
	protected String comments;
	protected GeoLocation geolocation;
	protected PostalAddress postalAddress;
	protected UUID image;
	protected long drillDownObjectId;
	protected final HashSet<Long> trustedNodes = new HashSet<Long>(0);
	protected boolean inheritAccessRights = true;
	protected HashSet<AccessListElement> accessList = new HashSet<AccessListElement>(0);
	protected int statusCalculationMethod;
	protected int statusPropagationMethod;
	protected ObjectStatus fixedPropagatedStatus;
	protected int statusShift;
	protected ObjectStatus[] statusTransformation;
	protected int statusSingleThreshold;
	protected int[] statusThresholds;
	protected final HashSet<Long> parents = new HashSet<Long>(0);
	protected final HashSet<Long> children = new HashSet<Long>(0);
	protected final List<Long> dashboards = new ArrayList<Long>(0);
	protected final Map<String, String> customAttributes = new HashMap<String, String>(0);
	protected final List<ObjectUrl> urls = new ArrayList<ObjectUrl>(0);
   protected final List<Long> responsibleUsers = new ArrayList<Long>(0);
	protected Map<String, Object> moduleData = null;
	
	@Internal private int effectiveRights = 0;
	@Internal private boolean effectiveRightsCached = false;

	/**
	 * Create dummy object of GENERIC class
	 * 
	 * @param id object ID to set
	 * @param session associated session
	 */
	protected AbstractObject(final long id, final NXCSession session)
	{
		objectId = id;
		this.session = session;
		guid = UUID.randomUUID();
		objectName = "unknown";
		objectClass = OBJECT_GENERIC;
		comments = "";
		geolocation = new GeoLocation(false);
		postalAddress = new PostalAddress();
		image = NXCommon.EMPTY_GUID;

		statusCalculationMethod = CALCULATE_DEFAULT;
		statusPropagationMethod = PROPAGATE_DEFAULT;
		fixedPropagatedStatus = ObjectStatus.NORMAL;
		statusShift = 0;
		statusTransformation = new ObjectStatus[4];
		statusTransformation[0] = ObjectStatus.WARNING;
		statusTransformation[1] = ObjectStatus.MINOR;
		statusTransformation[2] = ObjectStatus.MAJOR;
		statusTransformation[3] = ObjectStatus.CRITICAL;
		statusSingleThreshold = 75;
		statusThresholds = new int[4];
		statusThresholds[0] = 75;
		statusThresholds[1] = 75;
		statusThresholds[2] = 75;
		statusThresholds[3] = 75;
	}
	
	/**
	 * Create object from NXCP message
	 * @param msg Message to create object from
	 * @param session Associated client session
	 */
	public AbstractObject(final NXCPMessage msg, final NXCSession session)
	{
		int i, count;
		long id, id2;
		
		this.session = session;
		
		objectId = msg.getFieldAsInt32(NXCPCodes.VID_OBJECT_ID);
		guid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
		objectName = msg.getFieldAsString(NXCPCodes.VID_OBJECT_NAME);
		objectClass = msg.getFieldAsInt32(NXCPCodes.VID_OBJECT_CLASS);
		isDeleted = msg.getFieldAsBoolean(NXCPCodes.VID_IS_DELETED);
		status = ObjectStatus.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_OBJECT_STATUS));
		inMaintenanceMode = msg.getFieldAsBoolean(NXCPCodes.VID_MAINTENANCE_MODE);
		primaryZoneProxyId = msg.getFieldAsInt64(NXCPCodes.VID_PRIMARY_ZONE_PROXY_ID);
      backupZoneProxyId = msg.getFieldAsInt64(NXCPCodes.VID_BACKUP_ZONE_PROXY_ID);
		comments = msg.getFieldAsString(NXCPCodes.VID_COMMENTS);
		geolocation = new GeoLocation(msg);
		postalAddress = new PostalAddress(msg);
		image = msg.getFieldAsUUID(NXCPCodes.VID_IMAGE);
		drillDownObjectId = msg.getFieldAsInt64(NXCPCodes.VID_DRILL_DOWN_OBJECT_ID);
		if (image == null)
			image = NXCommon.EMPTY_GUID;
		
		statusCalculationMethod = msg.getFieldAsInt32(NXCPCodes.VID_STATUS_CALCULATION_ALG);
		statusPropagationMethod = msg.getFieldAsInt32(NXCPCodes.VID_STATUS_PROPAGATION_ALG);
		fixedPropagatedStatus = ObjectStatus.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_FIXED_STATUS));
		statusShift = msg.getFieldAsInt32(NXCPCodes.VID_STATUS_SHIFT);
		statusTransformation = new ObjectStatus[4];
		statusTransformation[0] = ObjectStatus.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_STATUS_TRANSLATION_1));
		statusTransformation[1] = ObjectStatus.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_STATUS_TRANSLATION_2));
		statusTransformation[2] = ObjectStatus.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_STATUS_TRANSLATION_3));
		statusTransformation[3] = ObjectStatus.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_STATUS_TRANSLATION_4));
		statusSingleThreshold = msg.getFieldAsInt32(NXCPCodes.VID_STATUS_SINGLE_THRESHOLD);
		statusThresholds = new int[4];
		statusThresholds[0] = msg.getFieldAsInt32(NXCPCodes.VID_STATUS_THRESHOLD_1);
		statusThresholds[1] = msg.getFieldAsInt32(NXCPCodes.VID_STATUS_THRESHOLD_2);
		statusThresholds[2] = msg.getFieldAsInt32(NXCPCodes.VID_STATUS_THRESHOLD_3);
		statusThresholds[3] = msg.getFieldAsInt32(NXCPCodes.VID_STATUS_THRESHOLD_4);
		
		// Status shift can be negative, but all int16 values read from message
		// as unsigned, so we need to convert shift value
		if (statusShift > 32767)
			statusShift = statusShift - 65536;
		
		// Parents
		count = msg.getFieldAsInt32(NXCPCodes.VID_PARENT_CNT);
		for(i = 0, id = NXCPCodes.VID_PARENT_ID_BASE; i < count; i++, id++)
		{
			parents.add(msg.getFieldAsInt64(id));
		}

		// Children
		count = msg.getFieldAsInt32(NXCPCodes.VID_CHILD_CNT);
		for(i = 0, id = NXCPCodes.VID_CHILD_ID_BASE; i < count; i++, id++)
		{
			children.add(msg.getFieldAsInt64(id));
		}
		
		// Trusted nodes
		count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_TRUSTED_NODES);
		if (count > 0)
		{
			Long[] nodes = msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_TRUSTED_NODES);
			trustedNodes.addAll(Arrays.asList(nodes));
		}
		
		// Dashboards
		Long[] d = msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_DASHBOARDS);
		if ((d != null) && (d.length > 0))
		   dashboards.addAll(Arrays.asList(d));
		
		// Custom attributes
		count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_CUSTOM_ATTRIBUTES);
		for(i = 0, id = NXCPCodes.VID_CUSTOM_ATTRIBUTES_BASE; i < count; i++, id += 2)
		{
			customAttributes.put(msg.getFieldAsString(id), msg.getFieldAsString(id + 1));
		}
		
		// URLs
      count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_URLS);
      for(i = 0, id = NXCPCodes.VID_URL_LIST_BASE; i < count; i++, id += 10)
      {
         urls.add(new ObjectUrl(msg, id));
      }
      
		// Access list
		inheritAccessRights = msg.getFieldAsBoolean(NXCPCodes.VID_INHERIT_RIGHTS);
		count = msg.getFieldAsInt32(NXCPCodes.VID_ACL_SIZE);
		for(i = 0, id = NXCPCodes.VID_ACL_USER_BASE, id2 = NXCPCodes.VID_ACL_RIGHTS_BASE; i < count; i++, id++, id2++)
		{
			accessList.add(new AccessListElement(msg.getFieldAsInt64(id), msg.getFieldAsInt32(id2)));
		}
		
		// Module-specific data
		count = msg.getFieldAsInt32(NXCPCodes.VID_MODULE_DATA_COUNT);
		if (count > 0)
		{
		   moduleData = new HashMap<String, Object>(count);
		   for(i = 0, id = NXCPCodes.VID_MODULE_DATA_BASE; i < count; i++, id += 0x100000)
		   {
		      String module = msg.getFieldAsString(id);
		      ModuleDataProvider p = (ModuleDataProvider)ServiceManager.getServiceHandler(module, ModuleDataProvider.class);
		      if (p != null)
		      {
		         moduleData.put(module, p.createModuleData(msg, id + 1));
		      }
		      else
		      {
		         Logger.error("AbstractObject", "Unable to find data provider for module " + module);
		      }
		   }
		}
		
		// Responsible users
		Long[] users = msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_RESPONSIBLE_USERS);
		for(i = 0; i < users.length; i++)
		{
		   responsibleUsers.add(users[i]);
		}
	}

	/**
	 * Check if object should be represented by class default image
	 * 
	 * @return true if default image should be used
	 */
	public boolean isDefaultImage()
	{
		return image.equals(NXCommon.EMPTY_GUID);
	}
	
	/**
	 * Get number of parent objects
	 * 
	 * @return number of parent objects
	 */
	public int getParentCount()
	{
	   return parents.size();
	}

	/**
	 * @return Iterator for list of parent objects
	 */
	public Iterator<Long> getParents()
	{
		return parents.iterator();
	}

	/**
	 * @return Iterator for list of child objects
	 */
	public Iterator<Long> getChildren()
	{
		return children.iterator();
	}

	/**
	 * @return Access list
	 */
	public AccessListElement[] getAccessList()
	{
		return accessList.toArray(new AccessListElement[accessList.size()]);
	}
	
	/**
	 * Get associated URLs
	 * 
	 * @return associated URLs
	 */
	public List<ObjectUrl> getUrls()
	{
	   return new ArrayList<ObjectUrl>(urls);
	}
	
	/**
	 * Check if object has associated URLs 
	 * 
	 * @return true if object has associated URLs
	 */
	public boolean hasUrls()
	{
	   return !urls.isEmpty();
	}

	/**
	 * Get object's comments.
	 * 
	 * @return object's comments
	 */
	public String getComments()
	{
		return comments;
	}

	/**
	 * Get object ID (unique within single NetXMS server).
	 * 
	 * @return object ID
	 */
	public long getObjectId()
	{
		return objectId;
	}

	/**
	 * Get object name.
	 * 
	 * @return object name
	 */
	public String getObjectName()
	{
		return objectName;
	}

	/**
	 * Get object status.
	 * 
	 * @return object status
	 */
	public ObjectStatus getStatus()
	{
		return status;
	}

	/**
	 * @return the isDeleted
	 */
	public boolean isDeleted()
	{
		return isDeleted;
	}

	/**
	 * Check setting of "inherit access rights" flag
	 * 
	 * @return true if "inherit access rights" flag is set
	 */
	public boolean isInheritAccessRights()
	{
		return inheritAccessRights;
	}

	/**
	 * Check if given object is direct or indirect parent
	 * 
	 * @param objectId ID of object to check
	 * @return true if given object is direct or indirect parent of this object
	 */
	public boolean isChildOf(final long objectId)
	{
	   if (parents.contains(objectId))
	      return true;
	   
		for(Long id : parents)
		{
			AbstractObject object = session.findObjectById(id);
			if (object != null)
			{
				if (object.isChildOf(objectId))
				   return true;
			}
		}
		return false;
	}

	/**
	 * Check if at least one of given objects is direct or indirect parent
	 * @param objects List of object ID to check
	 * @return true if at least one of given objects is direct or indirect parent of this object
	 */
	public boolean isChildOf(final long[] objects)
	{
		for(long object : objects)
		{
			if (isChildOf(object))
				return true;
		}
		return false;
	}

	/**
	 * Check if given object is direct parent
	 * 
	 * @param objectId ID of object to check
	 */
	public boolean isDirectChildOf(final long objectId)
	{
	   return parents.contains(objectId);
	}

   /**
    * Check if given object is direct or indirect child
    * 
    * @param objectId ID of object to check
    * @return true if given object is direct or indirect child of this object
    */
   public boolean isParentOf(final long objectId)
   {
      if (children.contains(objectId))
         return true;
      
      for(Long id : children)
      {
         AbstractObject object = session.findObjectById(id);
         if (object != null)
         {
            if (object.isParentOf(objectId))
               return true;
         }
      }
      return false;
   }

   /**
    * Check if at least one of given objects is direct or indirect child
    * @param objects List of object identifiers to check
    * @return true if at least one of given objects is direct or indirect child of this object
    */
   public boolean isParentOf(final long[] objects)
   {
      for(long object : objects)
      {
         if (isParentOf(object))
            return true;
      }
      return false;
   }

   /**
    * Check if given object is direct child
    * 
    * @param objectId ID of object to check
    * @return true if given object is direct child of this object
    */
   public boolean isDirectParentOf(final long objectId)
   {
      return children.contains(objectId);
   }

	/**
	 * @return List of parent objects
	 */
	public AbstractObject[] getParentsAsArray()
	{
		final Set<AbstractObject> list = new HashSet<AbstractObject>(parents.size());
		for (Long parent : parents)
		{
			AbstractObject obj = session.findObjectById(parent);
			if (obj != null)
			{
				list.add(obj);
			}
		}
		return list.toArray(new AbstractObject[list.size()]);
	}

	/**
	 * Get all child objects as array.
	 * 
	 * @return Array of child objects
	 */
	public AbstractObject[] getChildrenAsArray()
	{
		final Set<AbstractObject> list = new HashSet<AbstractObject>(children.size());
		for(Long id : children)
		{
			AbstractObject obj = session.findObjectById(id);
			if (obj != null)
			{
				list.add(obj);
			}
		}
		return list.toArray(new AbstractObject[list.size()]);
	}

	/**
	 * Return identifiers of all child objects
	 * 
	 * @return list of children
	 */
	public long[] getChildIdList()
	{
		long[] list = new long[children.size()];
		int i = 0;
		for(Long id : children)
			list[i++] = id;
		return list;
	}

	/**
	 * Return identifiers of all parent objects
	 * 
	 * @return list of object parents
	 */
	public long[] getParentIdList()
	{
		long[] list = new long[parents.size()];
		int i = 0;
		for(Long id : parents)
			list[i++] = id;
		return list;
	}
	
	/**
	 * Internal worker function for getAllChilds
	 * @param classFilter class filter
	 * @param set result set
	 */
	private void getAllChildrenInternal(int[] classFilter, Set<AbstractObject> set)
	{
		for (Long child : children)
		{
			AbstractObject obj = session.findObjectById(child);
			if (obj != null)
			{
				if (matchClassFilter(classFilter, obj.getObjectClass()))
				{
					set.add(obj);
				}
				obj.getAllChildrenInternal(classFilter, set);
			}
		}
	}

	/**
	 * Match class filter
	 * 
	 * @param classFilter class filter (list of class IDs)
	 * @param objectClass class to test
	 * @return true if matched
	 */
	private boolean matchClassFilter(int[] classFilter, int objectClass)
	{
		if (classFilter == null)
			return true;
		
		for(int c : classFilter)
		{
			if (objectClass == c)
				return true;
		}
		return false;
	}

	/**
	 * Get all child objects, direct and indirect
	 * 
	 * @param classFilter -1 to get all childs, or NetXMS class id to retrieve objects of given class
	 * @return set of child objects
	 */
	public Set<AbstractObject> getAllChildren(int classFilter)
	{
		Set<AbstractObject> result = new HashSet<AbstractObject>();
		getAllChildrenInternal((classFilter < 0) ? null : new int[] { classFilter }, result);
		return result;
	}

   /**
    * Get all child objects, direct and indirect
    * 
    * @param classFilter null to get all childs, or NetXMS class id(s) to retrieve objects of given class(es)
    * @return set of child objects
    */
   public Set<AbstractObject> getAllChildren(int[] classFilter)
   {
      Set<AbstractObject> result = new HashSet<AbstractObject>();
      getAllChildrenInternal(classFilter, result);
      return result;
   }

	/**
	 * Internal worker function for getAllParents
	 * @param classFilter class filter
	 * @param set result set
	 */
	private void getAllParentsInternal(int[] classFilter, Set<AbstractObject> set)
	{
		for (Long parent : parents)
		{
			AbstractObject obj = session.findObjectById(parent);
			if (obj != null)
			{
				if (matchClassFilter(classFilter, obj.getObjectClass()))
				{
					set.add(obj);
				}
				obj.getAllParentsInternal(classFilter, set);
			}
		}
	}

	/**
	 * Get all parent objects, direct and indirect
	 * 
	 * @param classFilter -1 to get all parents, or NetXMS class id to retrieve objects of given class
	 * @return set of parent objects
	 */
	public Set<AbstractObject> getAllParents(int classFilter)
	{
		Set<AbstractObject> result = new HashSet<AbstractObject>();
		getAllParentsInternal((classFilter < 0) ? null : new int[] { classFilter }, result);
		return result;
	}

   /**
    * Get all parent objects, direct and indirect
    * 
    * @param classFilter null to get all parents, or NetXMS class id(s) to retrieve objects of given class(es)
    * @return set of parent objects
    */
   public Set<AbstractObject> getAllParents(int[] classFilter)
   {
      Set<AbstractObject> result = new HashSet<AbstractObject>();
      getAllParentsInternal(classFilter, result);
      return result;
   }

	/**
	 * Get list of trusted nodes
	 * 
	 * @return List of trusted nodes
	 */
	public List<AbstractObject> getTrustedNodes()
	{
	   return session.findMultipleObjects(trustedNodes.toArray(new Long[trustedNodes.size()]), true);
	}

   /**
    * Get  list of associated dashboards
    * 
    * @param accessibleOnly if set to true, only accessible dashboards will be returned
    * @return list of the dashboard objects
    */
   public List<AbstractObject> getDashboards(boolean accessibleOnly)
   {
      return session.findMultipleObjects(dashboards.toArray(new Long[dashboards.size()]), true);
   }

	/**
	 * @return true if object has parents
	 */
	public boolean hasParents()
	{
		return parents.size() > 0;
	}

	/**
	 * @return true if object has children
	 */
	public boolean hasChildren()
	{
		return children.size() > 0;
	}

	/**
	 * @return true if object has children accessible by this session
	 */
	public boolean hasAccessibleChildren()
	{
		for(Long id : children)
			if (session.findObjectById(id) != null)
				return true;
		return false;
	}
	
	/**
	 * If this method returns true object is allowed to be on custom network map.
	 * Default implementation always returns false.
	 * 
	 * @return true if object is allowed to be on custom network map
	 */
	public boolean isAllowedOnMap()
	{
		return false;
	}
	
   /**
    * If this method returns true object can have visible alarms.
    * Default implementation always returns false.
    * 
    * @return true if object can contain visible alarms
    */
	public boolean isAlarmsVisible()
	{
	   return false;
	}

	/**
	 * @return the objectClass
	 */
	public int getObjectClass()
	{
		return objectClass;
	}

	/**
	 * @return Name of NetXMS object's class
	 */
	public String getObjectClassName()
	{
		return "Class " + Integer.toString(objectClass);
	}

	/**
	 * Get object's custom attributes
	 */
	public Map<String, String> getCustomAttributes()
	{
		return customAttributes;
	}

	/**
	 * @return the geolocation
	 */
	public GeoLocation getGeolocation()
	{
		return geolocation;
	}

	@Override
	public int hashCode()
	{
		return (int)objectId;
	}

	/**
	 * @return the guid
	 */
	public UUID getGuid()
	{
		return guid;
	}

	/**
	 * @return the image
	 */
	public UUID getImage()
	{
		return image;
	}

	/**
	 * @return the submapId
	 */
	public long getDrillDownObjectId()
	{
		return drillDownObjectId;
	}

	/**
	 * @return the statusCalculationMethod
	 */
	public int getStatusCalculationMethod()
	{
		return statusCalculationMethod;
	}

	/**
	 * @return the statusPropagationMethod
	 */
	public int getStatusPropagationMethod()
	{
		return statusPropagationMethod;
	}

	/**
	 * @return the fixedPropagatedStatus
	 */
	public ObjectStatus getFixedPropagatedStatus()
	{
		return fixedPropagatedStatus;
	}

	/**
	 * @return the statusShift
	 */
	public int getStatusShift()
	{
		return statusShift;
	}

	/**
	 * @return the statusTransformation
	 */
	public ObjectStatus[] getStatusTransformation()
	{
		return statusTransformation;
	}

	/**
	 * @return the statusSingleThreshold
	 */
	public int getStatusSingleThreshold()
	{
		return statusSingleThreshold;
	}

	/**
	 * @return the statusThresholds
	 */
	public int[] getStatusThresholds()
	{
		return statusThresholds;
	}

	/**
	 * Update internal session reference during session handover. This method should not be called directly!
	 * 
	 * @param session new session object
	 */
	public final void setSession(NXCSession session)
	{
		this.session = session;
	}

   /**
    * Get effective rights for this object. On first call this method
    * will do request to server, and on all subsequent calls
    * will return cached value obtained at first call.
    * 
    * @return effective user rights on this object
    */
   public int getEffectiveRights()
   {
      if (effectiveRightsCached)
         return effectiveRights;
      
      try
      {
         effectiveRights = session.getEffectiveRights(objectId);
         effectiveRightsCached = true;
      }
      catch(Exception e)
      {
         effectiveRights = 0;
      }
      return effectiveRights;
   }
   
   /**
    * Get module-specific data
    * 
    * @param module module name
    * @return custom data in module-specific format
    */
   public Object getModuleData(String module)
   {
      return (moduleData != null) ? moduleData.get(module) : null;
   }

   /**
    * @return the postalAddress
    */
   public PostalAddress getPostalAddress()
   {
      return postalAddress;
   }

   /**
    * @return the inMaintenanceMode
    */
   public boolean isInMaintenanceMode()
   {
      return inMaintenanceMode;
   }
   
   /**
    * Get primary zone proxy ID
    * 
    * @return primary zone proxy ID
    */
   public long getPrimaryZoneProxyId()
   {
      return primaryZoneProxyId;
   }

   /**
    * Get backup zone proxy ID
    * 
    * @return backup zone proxy ID
    */
   public long getBackupZoneProxyId()
   {
      return backupZoneProxyId;
   }

   /**
    * Add string to string set
    * 
    * @param strings
    * @param s
    */
   protected static void addString(Set<String> strings, String s)
   {
      if (s == null)
         return;
      String ts = s.trim();
      if (!ts.isEmpty())
         strings.add(ts);
   }
   
   /**
    * Get all strings in object
    * 
    * @return set of all strings in object
    */
   public Set<String> getStrings()
   {
      Set<String> strings = new HashSet<String>();
      addString(strings, comments);
      addString(strings, objectName);
      if ((postalAddress != null) && !postalAddress.isEmpty())
         strings.add(postalAddress.getAddressLine());
      for(String s : customAttributes.values())
         addString(strings, s);
      for(ObjectUrl u : urls)
      {
         addString(strings, u.getUrl().toString());
         addString(strings, u.getDescription());
      }
      return strings;
   }
   
   /**
    * Get direct responsible users
    * 
    * @return responsible users list
    */
   public List<Long> getResponsibleUsers()
   {
      return responsibleUsers;
   }
}

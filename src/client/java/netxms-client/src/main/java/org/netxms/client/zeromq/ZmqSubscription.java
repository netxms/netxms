package org.netxms.client.zeromq;

import java.util.ArrayList;
import java.util.List;

public class ZmqSubscription {

    private Long objectId;
    private boolean ignoreItems;
    private List<Long> items = new ArrayList<Long>();

    public ZmqSubscription(long objectId, boolean ignoreItems, long[] dciList) {
	this.objectId = objectId;
	this.ignoreItems = ignoreItems;
	for (long dciId : dciList) {
	    items.add(dciId);
	}
    }

    public Long getObjectId() {
	return objectId;
    }

    public List<Long> getItems() {
	return items;
    }

    public boolean isIgnoreItems() {
	return ignoreItems;
    }

    @Override
    public String toString() {
	return "ZmqSubscription [objectId=" + objectId + ", ignoreItems=" + ignoreItems + ", items=" + items + "]";
    }

}

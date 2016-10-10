package com.github.tomaskir.netxms.subagents.bind9.collection;

import java.util.HashMap;
import java.util.Map;

/**
 * @author Tomas Kirnak
 * @since 2.1-M1
 */
public final class CollectionResult {

    private boolean collectionError = true;

    private final Map<String, String> result = new HashMap<>();

    // getter
    public boolean isCollectionError() {
        return collectionError;
    }

    // setter
    public void setCollectionError(boolean collectionError) {
        this.collectionError = collectionError;
    }

    // getter
    public Map<String, String> getResult() {
        return result;
    }

}

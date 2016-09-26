package com.github.tomaskir.netxms.subagents.bind9.collection;

import lombok.Getter;
import lombok.Setter;

import java.util.HashMap;
import java.util.Map;

/**
 * @author Tomas Kirnak
 * @since 2.1-M1
 */
public final class CollectionResult {

    @Getter
    @Setter
    private boolean collectionError = true;

    @Getter
    private final Map<String, String> result = new HashMap<>();

}

/**
 * 
 */
package org.netxms.base.annotations;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Indicates internal class or field. Serialization code can use it to exclude certain elements.
 */
@Retention(RetentionPolicy.RUNTIME)
public @interface Internal
{
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.client.constants;

/**
 * ASN.1 constants
 */
public final class ASN
{
	public static final int INTEGER                 = 0x02;
	public static final int BIT_STRING              = 0x03;
	public static final int OCTET_STRING            = 0x04;
	public static final int NULL                    = 0x05;
	public static final int OBJECT_ID               = 0x06;
	public static final int SEQUENCE                = 0x30;
	public static final int IP_ADDR                 = 0x40;
	public static final int COUNTER32               = 0x41;
	public static final int GAUGE32                 = 0x42;
	public static final int TIMETICKS               = 0x43;
	public static final int OPAQUE                  = 0x44;
	public static final int NSAP_ADDR               = 0x45;
	public static final int COUNTER64               = 0x46;
	public static final int UINTEGER32              = 0x47;
	public static final int NO_SUCH_OBJECT          = 0x80;
	public static final int NO_SUCH_INSTANCE        = 0x81;
	public static final int GET_REQUEST_PDU         = 0xA0;
	public static final int GET_NEXT_REQUEST_PDU    = 0xA1;
	public static final int RESPONSE_PDU            = 0xA2;
	public static final int SET_REQUEST_PDU         = 0xA3;
	public static final int TRAP_V1_PDU             = 0xA4;
	public static final int GET_BULK_REQUEST_PDU    = 0xA5;
	public static final int INFORM_REQUEST_PDU      = 0xA6;
	public static final int TRAP_V2_PDU             = 0xA7;
	public static final int REPORT_PDU              = 0xA8;
}

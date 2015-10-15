/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.netxms.base.InetAddressEx;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.LinkLayerDiscoveryProtocol;
import org.netxms.client.snmp.SnmpObjectId;

/**
 * Network interface object
 */
public class Interface extends GenericObject
{
	// Interface flags
	public static final int IF_SYNTHETIC_MASK         = 0x00000001;
	public static final int IF_PHYSICAL_PORT          = 0x00000002;
	public static final int IF_EXCLUDE_FROM_TOPOLOGY  = 0x00000004;
	public static final int IF_LOOPBACK               = 0x00000008;
	public static final int IF_CREATED_MANUALLY       = 0x00000010;
	public static final int IF_EXPECTED_STATE_MASK    = 0x30000000;
	
	public static final int ADMIN_STATE_UNKNOWN      = 0;
	public static final int ADMIN_STATE_UP           = 1;
	public static final int ADMIN_STATE_DOWN         = 2;
	public static final int ADMIN_STATE_TESTING      = 3;
	
	public static final int OPER_STATE_UNKNOWN       = 0;
	public static final int OPER_STATE_UP            = 1;
	public static final int OPER_STATE_DOWN          = 2;
	public static final int OPER_STATE_TESTING       = 3;
	
	public static final int PAE_STATE_UNKNOWN        = 0;
	public static final int PAE_STATE_INITIALIZE     = 1;
	public static final int PAE_STATE_DISCONNECTED   = 2;
	public static final int PAE_STATE_CONNECTING     = 3;
	public static final int PAE_STATE_AUTHENTICATING = 4;
	public static final int PAE_STATE_AUTHENTICATED  = 5;
	public static final int PAE_STATE_ABORTING       = 6;
	public static final int PAE_STATE_HELD           = 7;
	public static final int PAE_STATE_FORCE_AUTH     = 8;
	public static final int PAE_STATE_FORCE_UNAUTH   = 9;
	public static final int PAE_STATE_RESTART        = 10;

	public static final int BACKEND_STATE_UNKNOWN    = 0;
	public static final int BACKEND_STATE_REQUEST    = 1;
	public static final int BACKEND_STATE_RESPONSE   = 2;
	public static final int BACKEND_STATE_SUCCESS    = 3;
	public static final int BACKEND_STATE_FAIL       = 4;
	public static final int BACKEND_STATE_TIMEOUT    = 5;
	public static final int BACKEND_STATE_IDLE       = 6;
	public static final int BACKEND_STATE_INITIALIZE = 7;
	public static final int BACKEND_STATE_IGNORE     = 8;
	
	private static final String[] stateText =
		{
			"UNKNOWN",
			"UP",
			"DOWN",
			"TESTING"
		};
	private static final String[] paeStateText =
		{
			"UNKNOWN",
			"INITIALIZE",
			"DISCONNECTED",
			"CONNECTING",
			"AUTHENTICATING",
			"AUTHENTICATED",
			"ABORTING",
			"HELD",
			"FORCE AUTH",
			"FORCE UNAUTH",
			"RESTART"
		};
	private static final String[] backendStateText =
		{
			"UNKNOWN",
			"REQUEST",
			"RESPONSE",
			"SUCCESS",
			"FAIL",
			"TIMEOUT",
			"IDLE",
			"INITIALIZE",
			"IGNORE"
		};
	
	private static final Map<Integer, String> ifTypeNames;
	
	static
	{
	   ifTypeNames = new HashMap<Integer, String>();
	   ifTypeNames.put(1, "other");
	   ifTypeNames.put(2, "regular1822");
	   ifTypeNames.put(3, "hdh1822");
	   ifTypeNames.put(4, "ddnX25");
	   ifTypeNames.put(5, "rfc877x25");
	   ifTypeNames.put(6, "ethernetCsmacd");
	   ifTypeNames.put(7, "iso88023Csmacd");
	   ifTypeNames.put(8, "iso88024TokenBus");
	   ifTypeNames.put(9, "iso88025TokenRing");
	   ifTypeNames.put(10, "iso88026Man");
	   ifTypeNames.put(11, "starLan");
	   ifTypeNames.put(12, "proteon10Mbit");
	   ifTypeNames.put(13, "proteon80Mbit");
	   ifTypeNames.put(14, "hyperchannel");
	   ifTypeNames.put(15, "fddi");
	   ifTypeNames.put(16, "lapb");
	   ifTypeNames.put(17, "sdlc");
	   ifTypeNames.put(18, "ds1");
	   ifTypeNames.put(19, "e1");
	   ifTypeNames.put(20, "basicISDN");
	   ifTypeNames.put(21, "primaryISDN");
	   ifTypeNames.put(22, "propPointToPointSerial");
	   ifTypeNames.put(23, "ppp");
	   ifTypeNames.put(24, "softwareLoopback");
	   ifTypeNames.put(25, "eon");
	   ifTypeNames.put(26, "ethernet3Mbit");
	   ifTypeNames.put(27, "nsip");
	   ifTypeNames.put(28, "slip");
	   ifTypeNames.put(29, "ultra");
	   ifTypeNames.put(30, "ds3");
	   ifTypeNames.put(31, "sip");
	   ifTypeNames.put(32, "frameRelay");
	   ifTypeNames.put(33, "rs232");
	   ifTypeNames.put(34, "para");
	   ifTypeNames.put(35, "arcnet");
	   ifTypeNames.put(36, "arcnetPlus");
	   ifTypeNames.put(37, "atm");
	   ifTypeNames.put(38, "miox25");
	   ifTypeNames.put(39, "sonet");
	   ifTypeNames.put(40, "x25ple");
	   ifTypeNames.put(41, "iso88022llc");
	   ifTypeNames.put(42, "localTalk");
	   ifTypeNames.put(43, "smdsDxi");
	   ifTypeNames.put(44, "frameRelayService");
	   ifTypeNames.put(45, "v35");
	   ifTypeNames.put(46, "hssi");
	   ifTypeNames.put(47, "hippi");
	   ifTypeNames.put(48, "modem");
	   ifTypeNames.put(49, "aal5");
	   ifTypeNames.put(50, "sonetPath");
	   ifTypeNames.put(51, "sonetVT");
	   ifTypeNames.put(52, "smdsIcip");
	   ifTypeNames.put(53, "propVirtual");
	   ifTypeNames.put(54, "propMultiplexor");
	   ifTypeNames.put(55, "ieee80212");
	   ifTypeNames.put(56, "fibreChannel");
	   ifTypeNames.put(57, "hippiInterface");
	   ifTypeNames.put(58, "frameRelayInterconnect");
	   ifTypeNames.put(59, "aflane8023");
	   ifTypeNames.put(60, "aflane8025");
	   ifTypeNames.put(61, "cctEmul");
	   ifTypeNames.put(62, "fastEther");
	   ifTypeNames.put(63, "isdn");
	   ifTypeNames.put(64, "v11");
	   ifTypeNames.put(65, "v36");
	   ifTypeNames.put(66, "g703at64k");
	   ifTypeNames.put(67, "g703at2mb");
	   ifTypeNames.put(68, "qllc");
	   ifTypeNames.put(69, "fastEtherFX");
	   ifTypeNames.put(70, "channel");
	   ifTypeNames.put(71, "ieee80211");
	   ifTypeNames.put(72, "ibm370parChan");
	   ifTypeNames.put(73, "escon");
	   ifTypeNames.put(74, "dlsw");
	   ifTypeNames.put(75, "isdns");
	   ifTypeNames.put(76, "isdnu");
	   ifTypeNames.put(77, "lapd");
	   ifTypeNames.put(78, "ipSwitch");
	   ifTypeNames.put(79, "rsrb");
	   ifTypeNames.put(80, "atmLogical");
	   ifTypeNames.put(81, "ds0");
	   ifTypeNames.put(82, "ds0Bundle");
	   ifTypeNames.put(83, "bsc");
	   ifTypeNames.put(84, "async");
	   ifTypeNames.put(85, "cnr");
	   ifTypeNames.put(86, "iso88025Dtr");
	   ifTypeNames.put(87, "eplrs");
	   ifTypeNames.put(88, "arap");
	   ifTypeNames.put(89, "propCnls");
	   ifTypeNames.put(90, "hostPad");
	   ifTypeNames.put(91, "termPad");
	   ifTypeNames.put(92, "frameRelayMPI");
	   ifTypeNames.put(93, "x213");
	   ifTypeNames.put(94, "adsl");
	   ifTypeNames.put(95, "radsl");
	   ifTypeNames.put(96, "sdsl");
	   ifTypeNames.put(97, "vdsl");
	   ifTypeNames.put(98, "iso88025CRFPInt");
	   ifTypeNames.put(99, "myrinet");
	   ifTypeNames.put(100, "voiceEM");
	   ifTypeNames.put(101, "voiceFXO");
	   ifTypeNames.put(102, "voiceFXS");
	   ifTypeNames.put(103, "voiceEncap");
	   ifTypeNames.put(104, "voiceOverIp");
	   ifTypeNames.put(105, "atmDxi");
	   ifTypeNames.put(106, "atmFuni");
	   ifTypeNames.put(107, "atmIma");
	   ifTypeNames.put(108, "pppMultilinkBundle");
	   ifTypeNames.put(109, "ipOverCdlc");
	   ifTypeNames.put(110, "ipOverClaw");
	   ifTypeNames.put(111, "stackToStack");
	   ifTypeNames.put(112, "virtualIpAddress");
	   ifTypeNames.put(113, "mpc");
	   ifTypeNames.put(114, "ipOverAtm");
	   ifTypeNames.put(115, "iso88025Fiber");
	   ifTypeNames.put(116, "tdlc");
	   ifTypeNames.put(117, "gigabitEthernet");
	   ifTypeNames.put(118, "hdlc");
	   ifTypeNames.put(119, "lapf");
	   ifTypeNames.put(120, "v37");
	   ifTypeNames.put(121, "x25mlp");
	   ifTypeNames.put(122, "x25huntGroup");
	   ifTypeNames.put(123, "transpHdlc");
	   ifTypeNames.put(124, "interleave");
	   ifTypeNames.put(125, "fast");
	   ifTypeNames.put(126, "ip");
	   ifTypeNames.put(127, "docsCableMaclayer");
	   ifTypeNames.put(128, "docsCableDownstream");
	   ifTypeNames.put(129, "docsCableUpstream");
	   ifTypeNames.put(130, "a12MppSwitch");
	   ifTypeNames.put(131, "tunnel");
	   ifTypeNames.put(132, "coffee");
	   ifTypeNames.put(133, "ces");
	   ifTypeNames.put(134, "atmSubInterface");
	   ifTypeNames.put(135, "l2vlan");
	   ifTypeNames.put(136, "l3ipvlan");
	   ifTypeNames.put(137, "l3ipxvlan");
	   ifTypeNames.put(138, "digitalPowerline");
	   ifTypeNames.put(139, "mediaMailOverIp");
	   ifTypeNames.put(140, "dtm");
	   ifTypeNames.put(141, "dcn");
	   ifTypeNames.put(142, "ipForward");
	   ifTypeNames.put(143, "msdsl");
	   ifTypeNames.put(144, "ieee1394");
	   ifTypeNames.put(145, "if-gsn");
	   ifTypeNames.put(146, "dvbRccMacLayer");
	   ifTypeNames.put(147, "dvbRccDownstream");
	   ifTypeNames.put(148, "dvbRccUpstream");
	   ifTypeNames.put(149, "atmVirtual");
	   ifTypeNames.put(150, "mplsTunnel");
	   ifTypeNames.put(151, "srp");
	   ifTypeNames.put(152, "voiceOverAtm");
	   ifTypeNames.put(153, "voiceOverFrameRelay");
	   ifTypeNames.put(154, "idsl");
	   ifTypeNames.put(155, "compositeLink");
	   ifTypeNames.put(156, "ss7SigLink");
	   ifTypeNames.put(157, "propWirelessP2P");
	   ifTypeNames.put(158, "frForward");
	   ifTypeNames.put(159, "rfc1483");
	   ifTypeNames.put(160, "usb");
	   ifTypeNames.put(161, "ieee8023adLag");
	   ifTypeNames.put(162, "bgppolicyaccounting");
	   ifTypeNames.put(163, "frf16MfrBundle");
	   ifTypeNames.put(164, "h323Gatekeeper");
	   ifTypeNames.put(165, "h323Proxy");
	   ifTypeNames.put(166, "mpls");
	   ifTypeNames.put(167, "mfSigLink");
	   ifTypeNames.put(168, "hdsl2");
	   ifTypeNames.put(169, "shdsl");
	   ifTypeNames.put(170, "ds1FDL");
	   ifTypeNames.put(171, "pos");
	   ifTypeNames.put(172, "dvbAsiIn");
	   ifTypeNames.put(173, "dvbAsiOut");
	   ifTypeNames.put(174, "plc");
	   ifTypeNames.put(175, "nfas");
	   ifTypeNames.put(176, "tr008");
	   ifTypeNames.put(177, "gr303RDT");
	   ifTypeNames.put(178, "gr303IDT");
	   ifTypeNames.put(179, "isup");
	   ifTypeNames.put(180, "propDocsWirelessMaclayer");
	   ifTypeNames.put(181, "propDocsWirelessDownstream");
	   ifTypeNames.put(182, "propDocsWirelessUpstream");
	   ifTypeNames.put(183, "hiperlan2");
	   ifTypeNames.put(184, "propBWAp2Mp");
	   ifTypeNames.put(185, "sonetOverheadChannel");
	   ifTypeNames.put(186, "digitalWrapperOverheadChannel");
	   ifTypeNames.put(187, "aal2");
	   ifTypeNames.put(188, "radioMAC");
	   ifTypeNames.put(189, "atmRadio");
	   ifTypeNames.put(190, "imt");
	   ifTypeNames.put(191, "mvl");
	   ifTypeNames.put(192, "reachDSL");
	   ifTypeNames.put(193, "frDlciEndPt");
	   ifTypeNames.put(194, "atmVciEndPt");
	   ifTypeNames.put(195, "opticalChannel");
	   ifTypeNames.put(196, "opticalTransport");
	   ifTypeNames.put(197, "propAtm");
	   ifTypeNames.put(198, "voiceOverCable");
	   ifTypeNames.put(199, "infiniband");
	   ifTypeNames.put(200, "teLink");
	   ifTypeNames.put(201, "q2931");
	   ifTypeNames.put(202, "virtualTg");
	   ifTypeNames.put(203, "sipTg");
	   ifTypeNames.put(204, "sipSig");
	   ifTypeNames.put(205, "docsCableUpstreamChannel");
	   ifTypeNames.put(206, "econet");
	   ifTypeNames.put(207, "pon155");
	   ifTypeNames.put(208, "pon622");
	   ifTypeNames.put(209, "bridge");
	   ifTypeNames.put(210, "linegroup");
	   ifTypeNames.put(211, "voiceEMFGD");
	   ifTypeNames.put(212, "voiceFGDEANA");
	   ifTypeNames.put(213, "voiceDID");
	   ifTypeNames.put(214, "mpegTransport");
	   ifTypeNames.put(215, "sixToFour");
	   ifTypeNames.put(216, "gtp");
	   ifTypeNames.put(217, "pdnEtherLoop1");
	   ifTypeNames.put(218, "pdnEtherLoop2");
	   ifTypeNames.put(219, "opticalChannelGroup");
	   ifTypeNames.put(220, "homepna");
	   ifTypeNames.put(221, "gfp");
	   ifTypeNames.put(222, "ciscoISLvlan");
	   ifTypeNames.put(223, "actelisMetaLOOP");
	   ifTypeNames.put(224, "fcipLink");
	   ifTypeNames.put(225, "rpr");
	   ifTypeNames.put(226, "qam");
	   ifTypeNames.put(227, "lmp");
	   ifTypeNames.put(228, "cblVectaStar");
	   ifTypeNames.put(229, "docsCableMCmtsDownstream");
	   ifTypeNames.put(230, "adsl2");
	   ifTypeNames.put(231, "macSecControlledIF");
	   ifTypeNames.put(232, "macSecUncontrolledIF");
	   ifTypeNames.put(233, "aviciOpticalEther");
	   ifTypeNames.put(234, "atmbond");
	   ifTypeNames.put(235, "voiceFGDOS");
	   ifTypeNames.put(236, "mocaVersion1");
	   ifTypeNames.put(237, "ieee80216WMAN");
	   ifTypeNames.put(238, "adsl2plus");
	   ifTypeNames.put(239, "dvbRcsMacLayer");
	   ifTypeNames.put(240, "dvbTdm");
	   ifTypeNames.put(241, "dvbRcsTdma");
	   ifTypeNames.put(242, "x86Laps");
	   ifTypeNames.put(243, "wwanPP");
	   ifTypeNames.put(244, "wwanPP2");
	   ifTypeNames.put(245, "voiceEBS");
	   ifTypeNames.put(246, "ifPwType");
	   ifTypeNames.put(247, "ilan");
	   ifTypeNames.put(248, "pip");
	   ifTypeNames.put(249, "aluELP");
	   ifTypeNames.put(250, "gpon");
	   ifTypeNames.put(251, "vdsl2");
	   ifTypeNames.put(252, "capwapDot11Profile");
	   ifTypeNames.put(253, "capwapDot11Bss");
	   ifTypeNames.put(254, "capwapWtpVirtualRadio");
	   ifTypeNames.put(255, "bits");
	   ifTypeNames.put(256, "docsCableUpstreamRfPort");
	   ifTypeNames.put(257, "cableDownstreamRfPort");
	   ifTypeNames.put(258, "vmwareVirtualNic");
	   ifTypeNames.put(259, "ieee802154");
	   ifTypeNames.put(260, "otnOdu");
	   ifTypeNames.put(261, "otnOtu");
	   ifTypeNames.put(262, "ifVfiType");
	   ifTypeNames.put(263, "g9981");
	   ifTypeNames.put(264, "g9982");
	   ifTypeNames.put(265, "g9983");
	   ifTypeNames.put(266, "aluEpon");
	   ifTypeNames.put(267, "aluEponOnu");
	   ifTypeNames.put(268, "aluEponPhysicalUni");
	   ifTypeNames.put(269, "aluEponLogicalLink");
	   ifTypeNames.put(270, "aluGponOnu");
	   ifTypeNames.put(271, "aluGponPhysicalUni");
	   ifTypeNames.put(272, "vmwareNicTeam");
	}
	
	private int flags;
	private int ifIndex;
	private int ifType;
	private int mtu;
	private long speed;
	private int slot;
	private int port;
	private MacAddress macAddress;
	private List<InetAddressEx> ipAddressList;
	private int requiredPollCount;
	private long peerNodeId;
	private long peerInterfaceId;
	private LinkLayerDiscoveryProtocol peerDiscoveryProtocol;
	private long zoneId;
	private String description;
	private String alias;
	private int adminState;
	private int operState;
	private int dot1xPaeState;
	private int dot1xBackendState;
	private SnmpObjectId ifTableSuffix;
	
	/**
	 * @param msg
	 */
	public Interface(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
		ifIndex = msg.getFieldAsInt32(NXCPCodes.VID_IF_INDEX);
		ifType = msg.getFieldAsInt32(NXCPCodes.VID_IF_TYPE);
      mtu = msg.getFieldAsInt32(NXCPCodes.VID_MTU);
      speed = msg.getFieldAsInt64(NXCPCodes.VID_SPEED);
		slot = msg.getFieldAsInt32(NXCPCodes.VID_IF_SLOT);
		port = msg.getFieldAsInt32(NXCPCodes.VID_IF_PORT);
		macAddress = new MacAddress(msg.getFieldAsBinary(NXCPCodes.VID_MAC_ADDR));
		requiredPollCount = msg.getFieldAsInt32(NXCPCodes.VID_REQUIRED_POLLS);
		peerNodeId = msg.getFieldAsInt64(NXCPCodes.VID_PEER_NODE_ID);
		peerInterfaceId = msg.getFieldAsInt64(NXCPCodes.VID_PEER_INTERFACE_ID);
		peerDiscoveryProtocol = LinkLayerDiscoveryProtocol.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_PEER_PROTOCOL));
		zoneId = msg.getFieldAsInt64(NXCPCodes.VID_ZONE_ID);
		description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
		alias = msg.getFieldAsString(NXCPCodes.VID_ALIAS);
		adminState = msg.getFieldAsInt32(NXCPCodes.VID_ADMIN_STATE);
		operState = msg.getFieldAsInt32(NXCPCodes.VID_OPER_STATE);
		dot1xPaeState = msg.getFieldAsInt32(NXCPCodes.VID_DOT1X_PAE_STATE);
		dot1xBackendState = msg.getFieldAsInt32(NXCPCodes.VID_DOT1X_BACKEND_STATE);
		ifTableSuffix = new SnmpObjectId(msg.getFieldAsUInt32Array(NXCPCodes.VID_IFTABLE_SUFFIX));
		
		int count = msg.getFieldAsInt32(NXCPCodes.VID_IP_ADDRESS_COUNT);
		ipAddressList = new ArrayList<InetAddressEx>(count);
		long fieldId = NXCPCodes.VID_IP_ADDRESS_LIST_BASE; 
		for(int i = 0; i < count; i++)
		   ipAddressList.add(msg.getFieldAsInetAddressEx(fieldId++));
	}
	
	/**
	 * Get parent node object.
	 * 
	 * @return parent node object or null if it is not exist or inaccessible
	 */
	public AbstractNode getParentNode()
	{
		AbstractNode node = null;
		synchronized(parents)
		{
			for(Long id : parents)
			{
				AbstractObject object = session.findObjectById(id);
				if (object instanceof AbstractNode)
				{
					node = (AbstractNode)object;
					break;
				}
			}
		}
		return node;
	}

	/**
	 * @return Interface index
	 */
	public int getIfIndex()
	{
		return ifIndex;
	}

	/**
	 * @return Interface type
	 */
	public int getIfType()
	{
		return ifType;
	}
	
	/**
	 * Get symbolic name for interface type
	 * 
	 * @return
	 */
	public String getIfTypeName()
	{
	   return getIfTypeName(ifType);
	}

	/**
	 * @return Interface MAC address
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @return Number of polls required to change interface status
	 */
	public int getRequiredPollCount()
	{
		return requiredPollCount;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Interface";
	}

	/* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
    */
   @Override
   public boolean isAllowedOnMap()
   {
      return true;
   }

   /**
	 * @return the slot
	 */
	public int getSlot()
	{
		return slot;
	}

	/**
	 * @return the port
	 */
	public int getPort()
	{
		return port;
	}

	/**
	 * @return the peerNodeId
	 */
	public long getPeerNodeId()
	{
		return peerNodeId;
	}

	/**
	 * @return the peerInterfaceId
	 */
	public long getPeerInterfaceId()
	{
		return peerInterfaceId;
	}

	/**
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @return the dot1xPaeState
	 */
	public int getDot1xPaeState()
	{
		return dot1xPaeState;
	}
	
	/**
	 * Get 802.1x PAE state as text
	 * 
	 * @return
	 */
	public String getDot1xPaeStateAsText()
	{
		try
		{
			return paeStateText[dot1xPaeState];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return paeStateText[PAE_STATE_UNKNOWN];
		}
	}

	/**
	 * @return the dot1xBackendState
	 */
	public int getDot1xBackendState()
	{
		return dot1xBackendState;
	}

	/**
	 * Get 802.1x backend state as text
	 * 
	 * @return
	 */
	public String getDot1xBackendStateAsText()
	{
		try
		{
			return backendStateText[dot1xBackendState];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return backendStateText[BACKEND_STATE_UNKNOWN];
		}
	}

	/**
	 * @return the adminState
	 */
	public int getAdminState()
	{
		return adminState;
	}

	/**
	 * @return the adminState
	 */
	public String getAdminStateAsText()
	{
		try
		{
			return stateText[adminState];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return stateText[ADMIN_STATE_UNKNOWN];
		}
	}

	/**
	 * @return the operState
	 */
	public int getOperState()
	{
		return operState;
	}
	
	/**
	 * @return the operState
	 */
	public String getOperStateAsText()
	{
		try
		{
			return stateText[operState];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return stateText[OPER_STATE_UNKNOWN];
		}
	}

	/**
	 * Get interface expected state
	 * 
	 * @return
	 */
	public int getExpectedState()
	{
		return (flags & IF_EXPECTED_STATE_MASK) >> 28;
	}
	
	/**
	 * @return
	 */
	public boolean isPhysicalPort()
	{
		return (flags & IF_PHYSICAL_PORT) != 0;
	}
	
	/**
	 * @return
	 */
	public boolean isLoopback()
	{
		return (flags & IF_LOOPBACK) != 0;
	}
	
	/**
	 * @return
	 */
	public boolean isExcludedFromTopology()
	{
		return (flags & IF_EXCLUDE_FROM_TOPOLOGY) != 0;
	}

   /**
    * @return the peerDiscoveryProtocol
    */
   public LinkLayerDiscoveryProtocol getPeerDiscoveryProtocol()
   {
      return peerDiscoveryProtocol;
   }

   /**
    * @return the mtu
    */
   public int getMtu()
   {
      return mtu;
   }

   /**
    * @return the speed
    */
   public long getSpeed()
   {
      return speed;
   }

   /**
    * @return the ifTableSuffix
    */
   public SnmpObjectId getIfTableSuffix()
   {
      return ifTableSuffix;
   }

   /**
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
   }

   /**
    * @return the ipAddressList
    */
   public List<InetAddressEx> getIpAddressList()
   {
      return ipAddressList;
   }
   
   /**
    * Check if given address present on interface
    * 
    * @param addr
    * @return
    */
   public boolean hasAddress(InetAddressEx addr)
   {
      return hasAddress(addr.address);
   }

   /**
    * Check if given address present on interface
    * 
    * @param addr
    * @return
    */
   public boolean hasAddress(InetAddress addr)
   {
      for(InetAddressEx a : ipAddressList)
         if (a.address.equals(addr))
            return true;
      return false;
   }
   
   /**
    * Get first unicast address
    * 
    * @return
    */
   public InetAddress getFirstUnicastAddress()
   {
      InetAddressEx a = getFirstUnicastAddressEx();
      return (a != null) ? a.address : null;
   }
   
   /**
    * Get first unicast address
    * 
    * @return
    */
   public InetAddressEx getFirstUnicastAddressEx()
   {
      for(InetAddressEx a : ipAddressList)
         if (!a.address.isAnyLocalAddress() && !a.address.isLinkLocalAddress() && !a.address.isLoopbackAddress() && !a.address.isMulticastAddress())
            return a;
      return null;
   }
   
   /**
    * Get IP address list as string
    * 
    * @return
    */
   public String getIpAddressListAsString()
   {
      StringBuilder sb = new StringBuilder();
      for(InetAddressEx a : ipAddressList)
      {
         if (sb.length() > 0)
            sb.append(", ");
         sb.append(a.toString());
      }
      return sb.toString();
   }
   
   /**
    * Get symbolic name for interface type
    * 
    * @param ifType
    * @return
    */
   public static String getIfTypeName(int ifType)
   {
      return ifTypeNames.get(ifType);
   }
}

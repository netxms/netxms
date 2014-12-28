/*
** nxflowd - NetXMS Flow Collector Daemon
** Copyright (c) 2009 Raden Solutions
*/

#include "nxflowd.h"


//
// Static data
//

static THREAD s_collectorThread = INVALID_THREAD_HANDLE;
static ipfix_col_info_t *s_collectorInfo = NULL;
static int s_numTcpSockets = 0;
static SOCKET *s_tcpSockets = NULL;
static int s_numUdpSockets = 0;
static SOCKET *s_udpSockets = NULL;
static INT64 s_flowId = 1;


//
// Handler for new message
//

static int H_NewMessage(ipfixs_node_t *node, ipfix_hdr_t *header, void *arg) 
{
	if (header->version == IPFIX_VERSION_NF9)
	{
		node->boot_time = header->u.nf9.unixtime - (header->u.nf9.sysuptime / 1000);
		node->export_time = 0;
	}
	else
	{
		node->boot_time = 0;
		node->export_time = header->u.ipfix.exporttime;
	}
	return 0;
}


//
// Get 64bit integer value from data field
//

static INT64 Int64FromData(void *data, int len)
{
	INT64 value;

	switch(len)
	{
		case 1:
			value = *((char *)data);
			break;
		case 2:
			value = *((short *)data);
			break;
		case 4:
			value = *((LONG *)data);
			break;
		case 8:
			value = *((INT64 *)data);
			break;
		default:
			value = 0;
			break;
	}
	return value;
}


//
// Handler for data record
//

static struct
{
	int ipfixField;
	const TCHAR *dbField;
} s_fieldMapping[] =
{
	{ IPFIX_FT_EXPORTERIPV4ADDRESS, _T("exporter_ip_addr") },
	{ IPFIX_FT_SOURCEMACADDRESS, _T("source_mac_addr") },
	{ IPFIX_FT_DESTINATIONMACADDRESS, _T("dest_mac_addr") },
	{ IPFIX_FT_SOURCEIPV4ADDRESS, _T("source_ip_addr") },
	{ IPFIX_FT_DESTINATIONIPV4ADDRESS, _T("dest_ip_addr") },
	{ IPFIX_FT_PROTOCOLIDENTIFIER, _T("ip_proto") },
	{ IPFIX_FT_SOURCETRANSPORTPORT, _T("source_ip_port") },
	{ IPFIX_FT_DESTINATIONTRANSPORTPORT, _T("dest_ip_port") },
	{ IPFIX_FT_OCTETDELTACOUNT, _T("octet_count") },
	{ IPFIX_FT_PACKETDELTACOUNT, _T("packet_count") },
	{ IPFIX_FT_INGRESSINTERFACE, _T("ingress_interface") },
	{ IPFIX_FT_EGRESSINTERFACE, _T("egress_interface") },
	{ 0, NULL }
};

static int H_DataRecord(ipfixs_node_t *node, ipfixt_node_t *trec, ipfix_datarecord_t *data, void *arg) 
{
	String fields, values;
	char buffer[256];
	INT64 flowStartTime = 0, flowEndTime = 0;

	for(int i = 0; i < trec->ipfixt->nfields; i++)
	{
		int ftype = trec->ipfixt->fields[i].elem->ft->ftype;
		switch(ftype)
		{
			case IPFIX_FT_FLOWSTARTSYSUPTIME:
				if (node->boot_time != 0)
					flowStartTime = node->boot_time * 1000 + Int64FromData(data->addrs[i], data->lens[i]);
				break;
			case IPFIX_FT_FLOWENDSYSUPTIME:
				if (node->boot_time != 0)
					flowEndTime = node->boot_time * 1000 + Int64FromData(data->addrs[i], data->lens[i]);
				break;
			case IPFIX_FT_FLOWSTARTSECONDS:
				flowStartTime = Int64FromData(data->addrs[i], data->lens[i]) * 1000;
				break;
			case IPFIX_FT_FLOWENDSECONDS:
				flowEndTime = Int64FromData(data->addrs[i], data->lens[i]) * 1000;
				break;
			case IPFIX_FT_FLOWSTARTMILLISECONDS:
				flowStartTime = Int64FromData(data->addrs[i], data->lens[i]);
				break;
			case IPFIX_FT_FLOWENDMILLISECONDS:
				flowEndTime = Int64FromData(data->addrs[i], data->lens[i]);
				break;
			case IPFIX_FT_FLOWSTARTMICROSECONDS:
				flowStartTime = Int64FromData(data->addrs[i], data->lens[i]) / 1000;
				break;
			case IPFIX_FT_FLOWENDMICROSECONDS:
				flowEndTime = Int64FromData(data->addrs[i], data->lens[i]) / 1000;
				break;
			case IPFIX_FT_FLOWSTARTNANOSECONDS:
				flowStartTime = Int64FromData(data->addrs[i], data->lens[i]) / 1000000;
				break;
			case IPFIX_FT_FLOWENDNANOSECONDS:
				flowEndTime = Int64FromData(data->addrs[i], data->lens[i]) / 1000000;
				break;
			case IPFIX_FT_FLOWSTARTDELTAMICROSECONDS:
				if (node->export_time != 0)
					flowStartTime = node->export_time * 1000 + Int64FromData(data->addrs[i], data->lens[i]);
				break;
			case IPFIX_FT_FLOWENDDELTAMICROSECONDS:
				if (node->export_time != 0)
					flowEndTime = node->export_time * 1000 + Int64FromData(data->addrs[i], data->lens[i]);
				break;
			default:
				for(int j = 0; s_fieldMapping[j].dbField != NULL; j++)
				{
					if (ftype == s_fieldMapping[j].ipfixField)
					{
						fields += _T(",");
						fields += s_fieldMapping[j].dbField;
						trec->ipfixt->fields[i].elem->snprint(buffer, sizeof(buffer), data->addrs[i], data->lens[i]);
						values.appendFormattedString(_T(",'%hs'"), buffer);
						break;
					}
				}
				break;
		}
	}

	if (!fields.isEmpty() && (flowStartTime != 0) && (flowEndTime != 0))
	{
		String query = _T("INSERT INTO flows (flow_id,start_time,end_time");
		query += (const TCHAR *)fields;
		query.appendFormattedString(_T(") VALUES (") INT64_FMT _T(",") INT64_FMT _T(",") INT64_FMT, s_flowId++, flowStartTime, flowEndTime);
		query += (const TCHAR *)values;
		query += _T(")");

		DBQuery(g_dbConnection, query);
	}
	return 0;
}


//
// Close collectors
//

static void CloseCollectors()
{
	int i;

	for(i = 0; i < s_numTcpSockets; i++)
		ipfix_col_close(s_tcpSockets[i]);

	for(i = 0; i < s_numUdpSockets; i++)
		ipfix_col_close(s_udpSockets[i]);
}


//
// Collector thread
//

static THREAD_RESULT THREAD_CALL CollectorThread(void *arg)
{
   nxlog_write(MSG_COLLECTOR_STARTED, EVENTLOG_INFORMATION_TYPE, NULL);

	while(!(g_flags & AF_SHUTDOWN))
	{
		if (mpoll_loop(2) < 0)
		{
		   nxlog_write(MSG_POLLING_ERROR, EVENTLOG_ERROR_TYPE, NULL);
			break;
		}
	}

   nxlog_write(MSG_COLLECTOR_STOPPED, EVENTLOG_INFORMATION_TYPE, NULL);
	return THREAD_OK;
}


//
// Start collector
//

bool StartCollector()
{
	// Initialize flow ID
	DB_RESULT hResult = DBSelect(g_dbConnection, _T("SELECT max(flow_id) FROM flows"));
	if (hResult != NULL)
	{
		s_flowId = DBGetFieldInt64(hResult, 0, 0) + 1;
		DBFreeResult(hResult);
	}

	s_collectorInfo = (ipfix_col_info_t *)malloc(sizeof(ipfix_col_info_t));
	s_collectorInfo->export_newsource = NULL;
	s_collectorInfo->export_newmsg = H_NewMessage;
	s_collectorInfo->export_trecord = NULL;
	s_collectorInfo->export_drecord = H_DataRecord;
	s_collectorInfo->export_dset = NULL;
	s_collectorInfo->export_cleanup = NULL;
	s_collectorInfo->data = NULL;

	if (ipfix_col_register_export(s_collectorInfo) < 0)
	{
      nxlog_write(MSG_IPFIX_REGISTER_FAILED, EVENTLOG_ERROR_TYPE, NULL);
		goto failure;
	}

	if (ipfix_col_listen(&s_numTcpSockets, &s_tcpSockets, IPFIX_PROTO_TCP, (int)g_tcpPort, AF_INET, 5) < 0)
	{
      nxlog_write(MSG_IPFIX_LISTEN_FAILED, EVENTLOG_ERROR_TYPE, "s", "TCP");
		goto failure;
	}

	if (ipfix_col_listen(&s_numUdpSockets, &s_udpSockets, IPFIX_PROTO_UDP, (int)g_udpPort, AF_INET, 0) < 0)
	{
      nxlog_write(MSG_IPFIX_LISTEN_FAILED, EVENTLOG_ERROR_TYPE, "s", "UDP");
		goto failure;
	}

	s_collectorThread = ThreadCreateEx(CollectorThread, 0, NULL);
	return true;

failure:
	CloseCollectors();
	free(s_collectorInfo);
	return false;
}


//
// Wait for collector thread termination
//

void WaitForCollectorThread()
{
	ThreadJoin(s_collectorThread);
}

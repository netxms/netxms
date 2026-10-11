#include <nms_common.h>
#include <nms_util.h>
#include <nxconfig.h>
#include <testtools.h>

static const char s_firstConfig[] = "[CORE]\nFileStore = /first\nSubAgent = a.nsm\nSubAgent = b.nsm\n";
static const char s_secondConfig[] = "[CORE]\nFileStore = /second\nSubAgent = c.nsm\n";

/**
 * Test value precedence when the same key is set in several configuration sources
 */
void TestConfig()
{
   StartTest(_T("Config - value precedence"));
   Config config;
   config.setLogErrors(false);
   AssertTrue(config.loadIniConfigFromMemory(s_firstConfig, strlen(s_firstConfig), _T("first.conf"), _T("CORE")));
   AssertTrue(config.loadIniConfigFromMemory(s_secondConfig, strlen(s_secondConfig), _T("second.conf"), _T("CORE")));

   AssertEquals(config.getValue(_T("/CORE/FileStore")), _T("/second"));
   ConfigEntry *entry = config.getEntry(_T("/CORE/FileStore"));
   AssertNotNull(entry);
   AssertEquals(entry->getValueCount(), 2);
   AssertEquals(entry->getValue(), _T("/second"));
   AssertEquals(entry->getValue(1), _T("/first"));
   AssertNull(entry->getValue(2));
   AssertEquals(entry->getValueFile(), _T("second.conf"));
   AssertEquals(entry->getValueLine(), 2);
   AssertEquals(entry->getValueFile(1), _T("first.conf"));
   AssertEquals(entry->getValueLine(1), 2);
   EndTest();

   StartTest(_T("Config - template parsing"));
   TCHAR fileStore[MAX_PATH] = _T("");
   StringList subAgents;
   NX_CFG_TEMPLATE cfgTemplate[] =
   {
      { _T("FileStore"), CT_STRING, 0, 0, MAX_PATH, 0, fileStore, nullptr },
      { _T("SubAgent"), CT_STRING_LIST, 0, 0, 0, 0, &subAgents, nullptr },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
   };
   AssertTrue(config.parseTemplate(_T("CORE"), cfgTemplate));
   AssertEquals(fileStore, _T("/second"));
   AssertEquals(subAgents.size(), 3);
   AssertEquals(subAgents.get(0), _T("a.nsm"));
   AssertEquals(subAgents.get(1), _T("b.nsm"));
   AssertEquals(subAgents.get(2), _T("c.nsm"));
   AssertEquals(config.getWarnings().size(), 1);
   AssertNotNull(_tcsstr(config.getWarnings().get(0), _T("second.conf")));
   AssertNotNull(_tcsstr(config.getWarnings().get(0), _T("first.conf")));
   EndTest();

   StartTest(_T("Config - directory load order"));
   TCHAR dir[MAX_PATH];
#ifdef _WIN32
   GetTempPath(MAX_PATH, dir);
   _sntprintf(&dir[_tcslen(dir)], MAX_PATH - _tcslen(dir), _T("nxtest-config-%u"), static_cast<unsigned int>(GetCurrentProcessId()));
#else
   _sntprintf(dir, MAX_PATH, _T("/tmp/nxtest-config-%u"), static_cast<unsigned int>(getpid()));
#endif
   AssertTrue(CreateDirectoryTree(dir));

   // Create files in reverse name order so that creation order differs from sorted order
   TCHAR fileName[MAX_PATH];
   _sntprintf(fileName, MAX_PATH, _T("%s") FS_PATH_SEPARATOR _T("zz.conf"), dir);
   AssertTrue(SaveFile(fileName, s_secondConfig, strlen(s_secondConfig), false) == SaveFileStatus::SUCCESS);
   _sntprintf(fileName, MAX_PATH, _T("%s") FS_PATH_SEPARATOR _T("aa.conf"), dir);
   AssertTrue(SaveFile(fileName, s_firstConfig, strlen(s_firstConfig), false) == SaveFileStatus::SUCCESS);

   Config dirConfig;
   dirConfig.setLogErrors(false);
   AssertTrue(dirConfig.loadConfigDirectory(dir, _T("CORE")));
   AssertEquals(dirConfig.getValue(_T("/CORE/FileStore")), _T("/second"));
   AssertEquals(dirConfig.getValue(_T("/CORE/FileStore"), nullptr, 1), _T("/first"));
   AssertTrue(DeleteDirectoryTree(dir));
   EndTest();
}

static const char s_xmlConfig[] =
   "<config>\n"
   "   <Server>\n"
   "      <Name>main &amp; &quot;backup&quot; &lt;x&gt;</Name>\n"
   "      <Port>4701</Port>\n"
   "      <Port>4702</Port>\n"
   "      <Node id=\"1\" enabled=\"true\">node1</Node>\n"
   "      <Node id=\"2\">node2</Node>\n"
   "   </Server>\n"
   "   <Agent>\n"
   "      <Flag></Flag>\n"
   "   </Agent>\n"
   "</config>\n";

/**
 * Test serialization of configuration to XML and JSON
 */
void TestConfigSerialization()
{
   StartTest(_T("Config - XML serialization"));
   Config config;
   config.setLogErrors(false);
   AssertTrue(config.loadXmlConfigFromMemory(s_xmlConfig, strlen(s_xmlConfig)));

   String xml = config.createXml();
   AssertTrue(xml.startsWith(_T("<config>\n")));
   AssertTrue(xml.endsWith(_T("</config>\n")));

   // Special characters must be escaped
   AssertTrue(xml.contains(_T("<Name>main &amp; &quot;backup&quot; &lt;x&gt;</Name>")));
   AssertFalse(xml.contains(_T("main & ")));

   // Multiple values are written as repeated elements in load order
   ssize_t p1 = xml.find(_T("<Port>4701</Port>"));
   ssize_t p2 = xml.find(_T("<Port>4702</Port>"));
   AssertTrue(p1 != -1);
   AssertTrue(p2 != -1);
   AssertTrue(p1 < p2);

   // Entries with ID get id attribute, "#id" suffix must not leak into element name; other attributes are preserved
   AssertTrue(xml.contains(_T("<Node id=\"1\" enabled=\"true\">node1</Node>")));
   AssertTrue(xml.contains(_T("<Node id=\"2\">node2</Node>")));
   AssertFalse(xml.contains(_T("Node#")));

   // Empty value
   AssertTrue(xml.contains(_T("<Flag></Flag>")));

   // Nesting is reflected by indentation
   AssertTrue(xml.contains(_T("\n    <Server>\n        <Name>")));
   AssertTrue(xml.contains(_T("\n    </Server>\n")));
   EndTest();

   StartTest(_T("Config - XML round trip"));
   std::string xmlUtf8 = xml.getUTF8StdString();
   Config reloaded;
   reloaded.setLogErrors(false);
   AssertTrue(reloaded.loadXmlConfigFromMemory(xmlUtf8.c_str(), xmlUtf8.length()));
   AssertEquals(reloaded.getValue(_T("/Server/Name")), _T("main & \"backup\" <x>"));
   ConfigEntry *port = reloaded.getEntry(_T("/Server/Port"));
   AssertNotNull(port);
   AssertEquals(port->getValueCount(), 2);
   AssertEquals(port->getValue(0), _T("4702"));
   AssertEquals(port->getValue(1), _T("4701"));
   ConfigEntry *node = reloaded.getEntry(_T("/Server/Node#1"));
   AssertNotNull(node);
   AssertEquals(node->getId(), 1);
   AssertEquals(node->getValue(), _T("node1"));
   AssertEquals(node->getAttribute(_T("enabled")), _T("true"));
   node = reloaded.getEntry(_T("/Server/Node#2"));
   AssertNotNull(node);
   AssertEquals(node->getId(), 2);
   AssertEquals(node->getValue(), _T("node2"));
   AssertEquals(reloaded.getValue(_T("/Agent/Flag")), _T(""));
   AssertTrue(reloaded.createXml().equals(xml));
   EndTest();

   StartTest(_T("Config - XML serialization of programmatically built config"));
   Config built;
   built.setTopLevelTag(_T("nxagentd"));
   AssertTrue(built.setValue(_T("/CORE/LogFile"), _T("/var/log/nxagentd")));
   AssertTrue(built.setValue(_T("/CORE/DebugLevel"), static_cast<int32_t>(5)));
   String builtXml = built.createXml();
   AssertTrue(builtXml.startsWith(_T("<nxagentd>\n")));
   AssertTrue(builtXml.endsWith(_T("</nxagentd>\n")));
   AssertTrue(builtXml.contains(_T("<LogFile>/var/log/nxagentd</LogFile>")));
   AssertTrue(builtXml.contains(_T("<DebugLevel>5</DebugLevel>")));

   Config empty;
   AssertEquals(empty.createXml().cstr(), _T("<config></config>\n"));
   EndTest();

   StartTest(_T("Config - JSON serialization"));
   json_t *json = config.createJson();
   AssertNotNull(json);
   AssertTrue(json_is_object(json));
   AssertEquals(json_object_size(json), 2);

   // Names are converted to camel case
   json_t *server = json_object_get(json, "server");
   AssertNotNull(server);
   AssertTrue(json_is_object(server));
   AssertNull(json_object_get(json, "Server"));

   json_t *name = json_object_get(server, "name");
   AssertNotNull(name);
   AssertTrue(json_is_string(name));
   AssertEquals(json_string_value(name), "main & \"backup\" <x>");

   // Multi-valued entries are serialized as arrays, effective (most recently loaded) value first
   json_t *jport = json_object_get(server, "port");
   AssertNotNull(jport);
   AssertTrue(json_is_array(jport));
   AssertEquals(json_array_size(jport), 2);
   AssertEquals(json_string_value(json_array_get(jport, 0)), "4702");
   AssertEquals(json_string_value(json_array_get(jport, 1)), "4701");

   // "#id" suffix is stripped from key
   AssertNotNull(json_object_get(server, "node"));
   AssertNull(json_object_get(server, "node#1"));
   AssertNull(json_object_get(server, "node#2"));

   // Empty value
   json_t *agent = json_object_get(json, "agent");
   AssertNotNull(agent);
   json_t *flag = json_object_get(agent, "flag");
   AssertNotNull(flag);
   AssertTrue(json_is_string(flag));
   AssertEquals(json_string_value(flag), "");
   json_decref(json);

   // Original names kept at all levels when camel case conversion is disabled
   json = config.createJson(nullptr, false);
   server = json_object_get(json, "Server");
   AssertNotNull(server);
   AssertNull(json_object_get(json, "server"));
   AssertNotNull(json_object_get(server, "Name"));
   AssertNull(json_object_get(server, "name"));
   json_decref(json);

   // Only subtree under given top level tag is serialized
   json = config.createJson(_T("Server"));
   AssertEquals(json_object_size(json), 3);
   AssertNull(json_object_get(json, "server"));
   AssertEquals(json_string_value(json_object_get(json, "name")), "main & \"backup\" <x>");
   AssertTrue(json_is_array(json_object_get(json, "port")));
   json_decref(json);

   json = config.createJson(_T("NoSuchSection"));
   AssertNotNull(json);
   AssertTrue(json_is_object(json));
   AssertEquals(json_object_size(json), 0);
   json_decref(json);

   json = empty.createJson();
   AssertNotNull(json);
   AssertEquals(json_object_size(json), 0);
   json_decref(json);
   EndTest();

   StartTest(_T("Config - JSON round trip"));
   json = config.createJson();
   char *jsonText = json_dumps(json, 0);
   json_decref(json);
   AssertNotNull(jsonText);

   Config fromJson;
   fromJson.setLogErrors(false);
   AssertTrue(fromJson.loadJsonConfigFromMemory(jsonText, strlen(jsonText), nullptr));
   MemFree(jsonText);
   AssertEquals(fromJson.getValue(_T("/Server/Name")), _T("main & \"backup\" <x>"));
   AssertEquals(fromJson.getValue(_T("/Agent/Flag")), _T(""));

   // Arrays are loaded as multiple values with precedence preserved
   port = fromJson.getEntry(_T("/Server/Port"));
   AssertNotNull(port);
   AssertEquals(port->getValueCount(), 2);
   AssertEquals(port->getValue(0), _T("4702"));
   AssertEquals(port->getValue(1), _T("4701"));

   // Arrays with mixed scalar types
   static const char arrayJson[] = "{\"List\": [\"a\", 2, true]}";
   Config fromArray;
   fromArray.setLogErrors(false);
   AssertTrue(fromArray.loadJsonConfigFromMemory(arrayJson, strlen(arrayJson), nullptr));
   ConfigEntry *list = fromArray.getEntry(_T("/List"));
   AssertNotNull(list);
   AssertEquals(list->getValueCount(), 3);
   AssertEquals(list->getValue(0), _T("a"));
   AssertEquals(list->getValue(1), _T("2"));
   AssertEquals(list->getValue(2), _T("true"));

   // Non-string JSON values are serialized back as strings
   static const char typedJson[] = "{\"Port\": 4701, \"Enabled\": true, \"Ratio\": 0.5}";
   Config typed;
   typed.setLogErrors(false);
   AssertTrue(typed.loadJsonConfigFromMemory(typedJson, strlen(typedJson), _T("CORE")));
   json = typed.createJson(_T("CORE"));
   AssertTrue(json_is_string(json_object_get(json, "port")));
   AssertEquals(json_string_value(json_object_get(json, "port")), "4701");
   AssertTrue(json_is_string(json_object_get(json, "enabled")));
   AssertEquals(json_string_value(json_object_get(json, "enabled")), "true");
   AssertTrue(json_is_string(json_object_get(json, "ratio")));
   AssertEquals(json_string_value(json_object_get(json, "ratio")), "0.500000");
   json_decref(json);
   EndTest();
}

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

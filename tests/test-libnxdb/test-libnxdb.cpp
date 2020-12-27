#include <nms_common.h>
#include <nms_util.h>
#include <nxdbapi.h>
#include <testtools.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(test-libnxdb)

#define MYSQL_SERVER   _T("mysql")
#define MYSQL_DBNAME   _T("nx_build_test")
#define MYSQL_LOGIN    _T("builder")
#define MYSQL_PASSWORD _T("builder1")

#define ORA_SERVER   _T("//127.0.0.1/XE")
#define ORA_LOGIN    _T("netxms")
#define ORA_PASSWORD _T("netxms")

#ifdef _WIN32
#define SQLITE_DB   _T("C:\\test-libnxdb.sqlite")
#else
#define SQLITE_DB   _T("/tmp/test-libnxdb.sqlite")
#endif

void TestOracleBatch(const TCHAR *server, const TCHAR *login, const TCHAR *password);

/**
 * Common tests
 */
static void CommonTests(const TCHAR *prefix, const TCHAR *driver, const TCHAR *server,
         const TCHAR *dbName, const TCHAR *login, const TCHAR *password, const TCHAR *syntax)
{
   /*** open database ***/
   StartTest(prefix, _T("open database"));

   DB_DRIVER drv = DBLoadDriver(driver, _T(""), NULL, NULL);
   AssertNotNull(drv);

   TCHAR buffer[DBDRV_MAX_ERROR_TEXT];
   DB_HANDLE session = DBConnect(drv, server, dbName, login, password, NULL, buffer);
   AssertNotNull(session);

   EndTest();

   /*** drop test tables if exist ***/
   if (DBIsTableExist(session, _T("nx_test")) == DBIsTableExist_Found)
      DBQuery(session, _T("DROP TABLE nx_test"));
   if (DBIsTableExist(session, _T("metadata")) == DBIsTableExist_Found)
      DBQuery(session, _T("DROP TABLE metadata"));

   /*** create metadata table ***/
   StartTest(prefix, _T("create metadata table"));
   AssertTrueEx(DBQueryEx(session, _T("CREATE TABLE metadata (var_name varchar(63) not null, var_value varchar(255) not null, PRIMARY KEY(var_name))"), buffer), buffer);
   AssertTrue(DBIsTableExist(session, _T("metadata")) == DBIsTableExist_Found);
   TCHAR query[256];
   _sntprintf(query, 256, _T("INSERT INTO metadata (var_name,var_value) VALUES ('Syntax','%s')"), syntax);
   AssertTrueEx(DBQueryEx(session, query, buffer), buffer);
   EndTest();

   /*** create test table ***/
   StartTest(prefix, _T("create test table"));
   AssertTrueEx(DBQueryEx(session, _T("CREATE TABLE nx_test (id integer not null,value1 varchar(63), value2 integer, PRIMARY KEY(id))"), buffer), buffer);
   AssertTrue(DBIsTableExist(session, _T("nx_test")) == DBIsTableExist_Found);
   EndTest();

   /*** insert single record ***/
   StartTest(prefix, _T("insert single record"));
   DB_STATEMENT hStmt = DBPrepareEx(session, _T("INSERT INTO nx_test (id,value1,value2) VALUES (?,?,?)"), true, buffer);
   AssertNotNullEx(hStmt, buffer);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (INT32)0);
#ifdef UNICODE
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, WideStringFromMBString(__FILE__), DB_BIND_DYNAMIC);
#else
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, __FILE__, DB_BIND_STATIC);
#endif
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)42);
   AssertTrueEx(DBExecuteEx(hStmt, buffer), buffer);
   DBFreeStatement(hStmt);
   EndTest();

   /*** check record count ***/
   StartTest(prefix, _T("check record count"));
   DB_RESULT hResult = DBSelectEx(session, _T("SELECT count(*) FROM nx_test"), buffer);
   AssertNotNullEx(hResult, buffer);
   AssertTrue(DBGetFieldLong(hResult, 0, 0) == 1);
   DBFreeResult(hResult);
   EndTest();

   /*** rename column ***/
   StartTest(prefix, _T("rename column"));
   AssertTrue(DBRenameColumn(session, _T("nx_test"), _T("value2"), _T("value2_new")));
   hResult = DBSelectEx(session, _T("SELECT value2_new FROM nx_test WHERE id=0"), buffer);
   AssertNotNullEx(hResult, buffer);
   AssertTrue(DBGetFieldLong(hResult, 0, 0) == 42);
   DBFreeResult(hResult);
   EndTest();

   /*** generate test set ***/
   StartTest(prefix, _T("insert in transaction"));
   AssertTrue(DBBegin(session));
   for(int i = 1; i <= 1000; i++)
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("INSERT INTO nx_test (id,value1,value2_new) VALUES (%d,'test',%d)"), i, i);
      AssertTrueEx(DBQueryEx(session, query, buffer), buffer);
   }
   AssertTrue(DBCommit(session));
   EndTest();

   /*** select prepared unbufferd ***/
   StartTest(prefix, _T("unbuffered select"));
   hStmt = DBPrepareEx(session, _T("SELECT value2_new FROM nx_test WHERE id>?"), false, buffer);
   AssertNotNullEx(hStmt, buffer);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (INT32)800);
   DB_UNBUFFERED_RESULT hResult2 = DBSelectPreparedUnbuffered(hStmt);
   AssertNotNull(hResult2);
   int count = 0;
   while(DBFetch(hResult2))
   {
      count++;
      AssertTrue(DBGetFieldLong(hResult2, 0) > 800);
   }
   DBFreeResult(hResult2);
   DBFreeStatement(hStmt);
   AssertEquals(count, 200);
   EndTest();

   /*** drop test table ***/
   StartTest(prefix, _T("drop test table"));
   AssertTrue(DBQuery(session, _T("DROP TABLE nx_test")));
   AssertTrue(DBQuery(session, _T("DROP TABLE metadata")));
   EndTest();

   /*** disconnect ***/
   StartTest(prefix, _T("close database"));
   DBDisconnect(session);
   DBUnloadDriver(drv);
   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   bool skipMySQL = false;
   bool skipOracle = false;
   bool skipSQLite = false;

   for(int i = 1; i < argc; i++)
   {
      if (!strcmp(argv[i], "--skip-mysql"))
         skipMySQL = true;
      else if (!strcmp(argv[i], "--skip-oracle"))
         skipOracle = true;
      else if (!strcmp(argv[i], "--skip-sqlite"))
         skipSQLite = true;
   }

   DBInit();

   if (!skipMySQL)
   {
      CommonTests(_T("MySQL"), _T("mysql.ddr"), MYSQL_SERVER, MYSQL_DBNAME, MYSQL_LOGIN, MYSQL_PASSWORD, _T("MYSQL"));
   }

   if (!skipOracle)
   {
      CommonTests(_T("Oracle"), _T("oracle.ddr"), ORA_SERVER, NULL, ORA_LOGIN, ORA_PASSWORD, _T("ORACLE"));
      TestOracleBatch(ORA_SERVER, ORA_LOGIN, ORA_PASSWORD);
   }

   if (!skipSQLite)
   {
      CommonTests(_T("SQLite"), _T("sqlite.ddr"), SQLITE_DB, NULL, NULL, NULL, _T("SQLITE"));
   }
   return 0;
}

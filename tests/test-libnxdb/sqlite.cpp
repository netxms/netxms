#include <nms_common.h>
#include <nms_util.h>
#include <nxdbapi.h>
#include <testtools.h>

#ifdef _WIN32
#define SQLITE_DB   _T("C:\\test-libnxdb.sqlite")
#else
#define SQLITE_DB   _T("/tmp/test-libnxdb.sqlite")
#endif

/**
 * Test SQLite driver
 */
void TestSQLite()
{
   /*** open database ***/
   StartTest(_T("SQLite: open database"));

   DB_DRIVER drv = DBLoadDriver(_T("sqlite.ddr"), _T(""), false, NULL, NULL);
   AssertNotNull(drv);

   TCHAR buffer[DBDRV_MAX_ERROR_TEXT];
   DB_HANDLE session = DBConnect(drv, NULL, NULL, NULL, SQLITE_DB, NULL, buffer);
   AssertNotNull(session);

   EndTest();

   /*** drop test tables if exist ***/
   if (DBIsTableExist(session, _T("nx_test")) == DBIsTableExist_Found)
      DBQuery(session, _T("DROP TABLE nx_test"));
   if (DBIsTableExist(session, _T("metadata")) == DBIsTableExist_Found)
      DBQuery(session, _T("DROP TABLE metadata"));

   /*** create metadata table ***/
   StartTest(_T("SQLite: create metadata table"));
   AssertTrueEx(DBQueryEx(session, _T("CREATE TABLE metadata (var_name varchar(63) not null, var_value varchar(255) not null, PRIMARY KEY(var_name))"), buffer), buffer);
   AssertTrue(DBIsTableExist(session, _T("metadata")) == DBIsTableExist_Found);
   AssertTrueEx(DBQueryEx(session, _T("INSERT INTO metadata (var_name,var_value) VALUES ('Syntax','SQLITE')"), buffer), buffer);
   EndTest();

   /*** create test table ***/
   StartTest(_T("SQLite: create test table"));
   AssertTrueEx(DBQueryEx(session, _T("CREATE TABLE nx_test (id integer not null,value1 varchar(63), value2 integer, PRIMARY KEY(id))"), buffer), buffer);
   AssertTrue(DBIsTableExist(session, _T("nx_test")) == DBIsTableExist_Found);
   EndTest();

   /*** insert single record ***/
   StartTest(_T("SQLite: insert single record"));
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
   StartTest(_T("SQLite: check record count"));
   DB_RESULT hResult = DBSelectEx(session, _T("SELECT count(*) FROM nx_test"), buffer);
   AssertNotNullEx(hResult, buffer);
   AssertTrue(DBGetFieldLong(hResult, 0, 0) == 1);
   DBFreeResult(hResult);
   EndTest();

   /*** rename column ***/
   StartTest(_T("SQLite: rename column"));
   AssertTrue(DBRenameColumn(session, _T("nx_test"), _T("value2"), _T("value2_new")));
   hResult = DBSelectEx(session, _T("SELECT value2_new FROM nx_test WHERE id=0"), buffer);
   AssertNotNullEx(hResult, buffer);
   AssertTrue(DBGetFieldLong(hResult, 0, 0) == 42);
   DBFreeResult(hResult);
   EndTest();

   /*** drop test table ***/
   StartTest(_T("SQLite: drop test table"));
   AssertTrue(DBQuery(session, _T("DROP TABLE nx_test")));
   AssertTrue(DBQuery(session, _T("DROP TABLE metadata")));
   EndTest();

   /*** disconnect ***/
   StartTest(_T("SQLite: close database"));
   DBDisconnect(session);
   DBUnloadDriver(drv);
   EndTest();
}

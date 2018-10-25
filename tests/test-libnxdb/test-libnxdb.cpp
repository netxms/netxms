#include <nms_common.h>
#include <nms_util.h>
#include <nxdbapi.h>
#include <testtools.h>

NETXMS_EXECUTABLE_HEADER(test-libnxdb)

void TestOracleBatch();
void TestSQLite();

/**
 * main()
 */
int main(int argc, char *argv[])
{
   bool skipOracle = false;
   bool skipSQLite = false;

   for(int i = 1; i < argc; i++)
   {
      if (!strcmp(argv[i], "--skip-oracle"))
         skipOracle = true;
      else if (!strcmp(argv[i], "--skip-sqlite"))
         skipSQLite = true;
   }

   DBInit(0, 0);

   if (!skipSQLite)
      TestSQLite();
   if (!skipOracle)
      TestOracleBatch();

   return 0;
}

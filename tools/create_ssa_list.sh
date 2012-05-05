#!/bin/sh

TIMESTAMP=`date`
LIST=`echo $1 | tr "[a-z]" "[A-Z]"`

echo "/* static_subagents.cpp  Generated at $TIMESTAMP */"
cat << EOT
#include <nxagentd.h>

extern "C"
{
EOT

for sa in $LIST; do
   echo "   BOOL NxSubAgentRegister_$sa(NETXMS_SUBAGENT_INFO **, Config *);"
done

cat << EOT
}

void InitStaticSubagents()
{
EOT

for sa in $LIST; do
   echo "   InitSubAgent(NULL, \"static:$sa\", NxSubAgentRegister_$sa, \"NxSubAgentRegister_$sa\");"
done

echo "}"

exit 0

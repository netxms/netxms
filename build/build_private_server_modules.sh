#!/usr/bin/env bash
#
# Rebuild and install all private server modules (private/*/src/server).
# Each module gets "make clean" followed by "make -j<nproc> install".
#
# Usage:
#    build_private_server_modules.sh [-c] [-k] [-n] [module ...]
#
#    -c        also build customer modules (private/customers/*/src/server)
#    -k        keep going after a module fails (failures are listed at the end)
#    -n        only list modules that would be built
#    module    build only modules whose directory path contains this string
#              (e.g. "ist", "atmcore", "netxms/src/server/aiext")
#
# PREFIX is passed through to make when set in the environment
# (modules default to /opt/netxms).
#

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS=$(nproc)
KEEP_GOING="no"
CUSTOMERS="no"
LIST_ONLY="no"
FILTERS=()

while [[ $# -gt 0 ]]; do
   case "$1" in
      -c) CUSTOMERS="yes" ;;
      -k) KEEP_GOING="yes" ;;
      -n) LIST_ONLY="yes" ;;
      -h|--help) sed -n '3,18p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
      *) FILTERS+=("$1") ;;
   esac
   shift
done

# Module directory is src/server itself when it has a Makefile, otherwise each
# immediate subdirectory with a Makefile (e.g. atm/src/server/atmcore)
MODULES=()
while IFS= read -r serverdir; do
   if [[ -f "$serverdir/Makefile" ]]; then
      MODULES+=("$serverdir")
   else
      for d in "$serverdir"/*/; do
         [[ -f "$d/Makefile" ]] && MODULES+=("${d%/}")
      done
   fi
done < <(
   for d in "$ROOT"/private/*/src/server; do [[ -d "$d" ]] && echo "$d"; done
   if [[ "$CUSTOMERS" == "yes" ]]; then
      for d in "$ROOT"/private/customers/*/src/server; do [[ -d "$d" ]] && echo "$d"; done
   fi
)

if [[ ${#FILTERS[@]} -gt 0 ]]; then
   SELECTED=()
   for m in "${MODULES[@]}"; do
      for f in "${FILTERS[@]}"; do
         if [[ "${m#$ROOT/}" == *"$f"* ]]; then
            SELECTED+=("$m")
            break
         fi
      done
   done
   MODULES=("${SELECTED[@]}")
fi

if [[ ${#MODULES[@]} -eq 0 ]]; then
   echo "No private server modules found"
   exit 1
fi

if [[ "$LIST_ONLY" == "yes" ]]; then
   printf '%s\n' "${MODULES[@]#$ROOT/}"
   exit 0
fi

MAKE_ARGS=()
[[ -n "$PREFIX" ]] && MAKE_ARGS+=("PREFIX=$PREFIX")

FAILED=()
for m in "${MODULES[@]}"; do
   name="${m#$ROOT/}"
   echo "=== $name ==="
   if make -C "$m" "${MAKE_ARGS[@]}" clean && make -C "$m" "${MAKE_ARGS[@]}" -j"$JOBS" install; then
      echo "=== $name: OK ==="
   else
      echo "=== $name: FAILED ==="
      FAILED+=("$name")
      [[ "$KEEP_GOING" == "no" ]] && exit 1
   fi
done

echo
echo "Built ${#MODULES[@]} module(s), ${#FAILED[@]} failed"
for f in "${FAILED[@]}"; do
   echo "   FAILED: $f"
done
[[ ${#FAILED[@]} -eq 0 ]]

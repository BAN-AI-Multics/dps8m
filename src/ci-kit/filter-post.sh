#!/usr/bin/env bash
grep --line-buffered \
-ve '^[0-9]' \
-ve '^[a-z]' \
-ve '^---$' \
-ve '^[<>]$' \
-ve '^[<>]  xx$' \
-ve '^[<>]  xxx$' \
-ve '^[<>] --> cord$' \
-ve '^[<>] ??:??:?? ??????? ==$' \
-ve '^< stack_\. \.$' \
/dev/stdin | sed -e 's/??:??:?? ???????//'

#!/usr/bin/env sh
test -f prefilter.txt || { echo 'Error: No prefilter.txt'; exit 1; };
rm -f outfilter1.txt > /dev/null 2>&1 || true
rm -f outfilter2.txt > /dev/null 2>&1 || true
grep '^< ' prefilter.txt | sed 's/^< //' | sort -u 2>/dev/null > outfilter1.txt 2>/dev/null
grep '^> ' prefilter.txt | sed 's/^> //' | sort -u 2>/dev/null > outfilter2.txt 2>/dev/null 
comm -13 <(sort outfilter1.txt) <(sort outfilter2.txt) > only_in_new.txt
comm -12 <(sort outfilter1.txt) <(sort outfilter2.txt) > only_in_old.txt
rm -f outfilter1.txt; rm -f outfilter2.txt; rm -f prefilter.txt

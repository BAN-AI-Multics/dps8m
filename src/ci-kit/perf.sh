#!/usr/bin/env bash
cd ./run
cp -r ../../perf_test/ .
cd perf_test
../dps8 nqueensx.ini


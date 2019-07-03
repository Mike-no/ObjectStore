###########################################################
# 
#  @author: Michael De Angelis
#  @mat: 560049
#  @project: Sistemi Operativi e Laboratorio [SOL]
#  @A.A: 2018 / 2019
# 
###########################################################

#!/bin/bash

printf "########################################\n\n"

if [ ! -f ./testout.log ]; then
    echo "Log file not found"
    printf "\n########################################\n\n"
    exit
fi

echo "#Clients executed  : $(grep usertest* testout.log -c)"

printf "\n"

echo "#Clients battery 1 : $(grep 'Test battery' testout.log | tr -dc '1' | wc -c)"
echo "#Clients battery 2 : $(grep 'Test battery' testout.log | tr -dc '2' | wc -c)"
echo "#Clients battery 3 : $(grep 'Test battery' testout.log | tr -dc '3' | wc -c)"

printf "\n"

echo "#Clients that reported anomalies : $(grep 'Operations terminated w/o' testout.log | tr -dc '2' | wc -c)"

echo "Anomalies battery 1 : $(grep 'Test battery' testout.log -A 3 | grep -v 'Operations executed ' | \
grep -v 'Operations terminated w/ ' | grep -v '-' | tr -d '[:blank:]' | grep 'Testbattery:1' -A 1 |
grep -v 'Testbattery:1' | sed 's/[^0-9]*//g' | awk '{ SUM += $1} END { print SUM }')"
echo "Anomalies battery 2 : $(grep 'Test battery' testout.log -A 3 | grep -v 'Operations executed ' | \
grep -v 'Operations terminated w/ ' | grep -v '-' | tr -d '[:blank:]' | grep 'Testbattery:2' -A 1 |
grep -v 'Testbattery:2' | sed 's/[^0-9]*//g' | awk '{ SUM += $1} END { print SUM }')"
echo "Anomalies battery 3 : $(grep 'Test battery' testout.log -A 3 | grep -v 'Operations executed ' | \
grep -v 'Operations terminated w/ ' | grep -v '-' | tr -d '[:blank:]' | grep 'Testbattery:3' -A 1 |
grep -v 'Testbattery:3' | sed 's/[^0-9]*//g' | awk '{ SUM += $1} END { print SUM }')"

printf "\n########################################\n\n"

kill -10 "$(pidof object_store)"
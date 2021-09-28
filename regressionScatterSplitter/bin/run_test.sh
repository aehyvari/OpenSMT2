#!/bin/bash
echo "This is the script for running regression tests for scatter splitter after learnt clause injection"
echo " - date: $(date '+%Y-%m-%d at %H:%M.%S')"
echo " - host name $(hostname -f)"

success=0
total=0
error=0
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

if [ $# != 1 ]; then
    echo "Usage: $0 <path to OpenSMT executable>"
    exit 1
fi
if [[ "${1}" != *"opensmt" ]];
    then
      echo ${RED}"provide the path to OpenSMT executable"
      exit 1
fi


for folder in "../instances"/*; do
    for file in "$folder"/*/*; do

        if [[ "${file}" != *".smt2" ]];
            then
              continue
        fi

        ((total=total+1))
        echo "***********************************************************************"
        echo "Test  #${total}:   "$file

        result=$(/usr/bin/time -p "$1" $file 2>&1)
        echo ${RED}${result}${NC}
        if [[ "${result}" != *"unknown"* ]];
            then
              ((error=error+1))
              echo ${RED}${result}${NC}
              continue
        fi

        numOfPartitions=(${result//:/ })

        echo "  Solve Result:                           " ${numOfPartitions[0]}
        echo "  Partition Result:                       " ${numOfPartitions[1]}
        echo "  Number of partition splits requested:   " ${numOfPartitions[2]}
        echo "  Number of partition splits created:     " ${numOfPartitions[3]}
        echo "  total time:                             " ${numOfPartitions[5]}

        if  [ ${numOfPartitions[2]} == ${numOfPartitions[3]} ]
        then
          ((success=success+1))
          echo  "${GREEN}Passed${NC}"

        else
          echo  "${RED}Failed${NC}"
        fi

       wait

    done
done

echo "................................................"
echo "Number of instances........." ${total}
echo "Number of successed........." ${success}

((failed=total-success-error))

if  [ ${error} == 0 ]
  then
    echo "Number of error............." ${error}

  else
    echo ${RED}"Number of error............." ${error}${NC}
fi

if  [ ${failed} -le 0 ]
  then
    echo "Number of failed............" ${failed}

  else
    echo ${RED}"Number of failed............" ${failed} " -------- P.S. Means number of splits requested is not equal to created splits"${NC}
fi



#!/bin/sh

###########################
# https://bugzilla.sw.ru/show_bug.cgi?id=109374
#
# State machine spec: 
# http://wiki.parallels.com/images/5/5e/Vm-state-machine.xls
#
###########################
# How use it:
# $ ./stateMachine.test.sh vm-name
#	OR 
# $ ./stateMachine.test.sh $RANDOM
###########################


prlctl create $1 &&
prlctl set $1 --memsize 64 &&
while ((1)); do
	# STOPPED
	echo "==STOPPED state tests=="
        prlctl stop  $1 && break
        prlctl reset $1 && break
        prlctl pause $1 && break
        prlctl suspend $1 && break
        # prlctl resume  $1 && break
        prlctl stop $1 --kill && break

	# RUNNING state test
	echo "==RUNNING state test=="
	echo ==1
        prlctl start $1 || break 
        prlctl start $1 && break 
        prlctl resume $1 && break 
        prlctl stop  $1 ||  break 
		prlctl stop $1 --kill ||  break 

	echo ==2
        prlctl start $1 || break 
        prlctl reset $1 || break 
		sleep 2
		prlctl stop $1 --kill ||  break 
		
	# PAUSED state test
	echo "==PAUSED state test=="
	echo ==1
        prlctl start $1 || break
        prlctl pause $1 || break
        prlctl start $1 || break
        prlctl stop $1 --kill || break
	echo ==2
        prlctl start $1 || break
        prlctl pause $1 || break
        prlctl stop $1 --kill || break
	echo ==3
        prlctl start $1 || break
        prlctl pause $1 || break
        prlctl reset $1 || break
		sleep 2
        prlctl stop $1 --kill || break
	echo ==4
        prlctl start $1 || break
        prlctl pause $1 || break
        prlctl pause $1 && break
        prlctl stop $1 --kill || break
	echo ==5	
        prlctl start $1 || break
        prlctl pause $1 || break
        prlctl suspend $1 && break
        prlctl stop $1 --kill || break
	echo ==6	
        prlctl start $1 || break
        prlctl pause $1 || break
        prlctl stop $1 && break  # (shutdown)

			
	echo "Finalize test.."
    prlctl stop $1 --kill ||  break 
	prlctl destroy $1 && echo ok || break
	echo "test PASS !!!"
	break;
done
echo "test was finished at " `date`


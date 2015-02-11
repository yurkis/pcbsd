#!/bin/sh

DEFAULT="/usr/local"

if [ -z "$1" ] ; then 
	LB="${DEFAULT}" 
else
	LB="${1}" 
fi

DIR=`dirname $0`
DIR=`realpath $DIR`
cd ${DIR}

SUBPROJ="about de-info de-logout pbreg pc-adctl pc-checkxdisplay \
	 pc-extractoverlay pc-fbsdupdatecheck pc-firstgui pc-installdialog \
	 pc-pkg pc-runxgui pc-su pc-sysconfig pc-sysinstall pc-systemflag \
	 pc-thinclient pc-updatemanager pc-xdgutil personacrypt"

# Add all the sub-projects
for i in ${SUBPROJ}
do
  cd $i
  if [ $? -ne 0 ] ; then exit 1 ; fi
  make install
  if [ $? -ne 0 ] ; then exit 1 ; fi
  cd ${DIR}  
  if [ $? -ne 0 ] ; then exit 1 ; fi
done
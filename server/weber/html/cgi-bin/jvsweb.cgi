#!/bin/sh
eval `./proccgi`
echo Content-type: application/json

#echo

#echo $FORM_cmd

#echo $FORM_action

#echo $FORM_param

#echo $FORM_username

#echo $FORM_password

#echo

./wagent $FORM_cmd $FORM_action "$FORM_param" $FORM_username $FORM_password


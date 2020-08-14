#!/bin/bash
LOCAL_PATH= 'PWD'
MANIFEST_XML_FLIE=$LOCAL_PATH/manifest.xml
PROJECT_NAME_PREFIX="sdm845-la13-master"

USER_NAME="dfyuan"
SERVER_IP="10.0.16.15"
SERVER_PORT="29418"

OUTPUT_PROJECT_LIST_FILE_NAME=$LOCAL_PATH/project_list_name
OUTPUT_PROJECT_LIST_FILE_PATH=$LOCAL_PATH/project_list_path

function getNameAndPath()
{
  echo > $OUTPUT_PROJECT_LIST_FILE_NAME
  echo > $OUTPUT_PROJECT_LIST_FILE_PATH
	
  while read LINE
  do 
		command_line='echo $LINE | grep "<project"'
		if [ "$command_line" ]
		then
			#echo $LINE
       reposity_name_sec= ${LINE#*NAME=\"}
	   reposity_path_sec=${LINE#*path=\"}
	 
			if ["$reposity_name_sec"] && [ "$reposity_path_sec" ]
			then	
			   reposity_name= ${reposity_name_sec%%\"*}
			   reposity_path= ${reposity_path_sec%%\"*}
			   
			   echo "$reposity_name"  >> $OUTPUT_PROJECT_LIST_FILE_NAME
			   echo "$reposity_path"  >> $OUTPUT_PROJECT_LIST_FILE_PATH
			fi
		fi
	done < $MANIFEST_XML_FILE
}

function creatEmptyGerritProject()
{
	for i in 'cat $OUTPUT_PROJECT_LIST_FILE_NAME';
	do
		echo $i
		echo "ssh -p $SERVER_PORT $USER_NAME@SERVER_IP gerrit create-project --empty-commit $PROJECT_NAME_PREFIX/$i"
		ssh -p $SERVER_PORT $USER_NAME@SERVER_IP gerrit create-project --empty-commit $PROJECT_NAME_PREFIX/$i
	done
}

function removeFiles()
 {
	rm -rf $LOCAL_PATH/project_list_name
	rm -rf $LOCAL_PATH/project_list_path
}

getNameAndPath
creatEmptyGerritProject
removeFiles
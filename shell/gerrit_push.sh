#!/bin/bash
LOCAL_PATH= 'PWD'
MANIFEST_XML_FLIE=$LOCAL_PATH/manifest.xml
PROJECT_NAME_PREFIX="sdm845-la13-master"

USER_NAME="dfyuan"
SERVER_IP="10.0.16.15"
SERVER_PORT="29418"

OUTPUT_PROJECT_LIST_FILE_NAME=$LOCAL_PATH/project_list_name
OUTPUT_PROJECT_LIST_FILE_PATH=$LOCAL_PATH/project_list_path

function pushLocalToRemote()
{
  while read LINE
  do 
	cd $LOCAL_PATH
	command_line='echo $LINE | grep "<project"'
	if [ "$command_line" ]
	then
		#echo $LINE
		reposity_name_sec=${LINE#*name=\"}
	    reposity_path_sec=${LINE#*path=\"}
		
		if ["$reposity_name_sec"] && [ "$reposity_path_sec" ]
			then	
			   reposity_name= ${reposity_name_sec%%\"*}
			   reposity_path= ${reposity_path_sec%%\"*}
			   
			src_path=$LOCAL_PATH/$reposity_path
			
			if [ -d "$src_path" ];then
				cd $src_path
				echo 'pwd'
				rm -rf .git
				rm -rf .gitignore
				git init
				git remote add origin ssh://$USER_NAME@SERVER_IP:$SERVER_PORT/$PROJECT_NAME_PREFIX/$reposity_name.git	
				git pull origin master
				git add -A .
				git commit -am "init commit"
				git push origin master
				cd -
			fi
		fi
		
		
		if ["$reposity_name_sec"] && [ !"$reposity_path_sec" ]
			then	
			   reposity_name= ${reposity_name_sec%%\"*}
			   reposity_path= $reposity_path
			   
				src_path=$LOCAL_PATH/$reposity_path
			
				if [ -d "$src_path" ];then
				cd $src_path
				echo 'pwd'
				rm -rf .git
				rm -rf .gitignore
				git init
				git remote add origin ssh://$USER_NAME@SERVER_IP:$SERVER_PORT/$PROJECT_NAME_PREFIX/$reposity_name.git	
				git pull origin master
				git add -A .
				git commit -am "init commit"
				git push origin master
				cd -
			fi
		fi
	fi
	
	done  <$MANIFEST_XML_FILE	
}

pushLocalToRemote
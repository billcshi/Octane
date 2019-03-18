#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>

#include "task_config.h"
#include "Stage1/stage1main.h"
#include "Stage2/stage2main.h"
#include "Stage3/stage3main.h"
#include "Stage4/stage4main.h"
#include "Stage5/stage5main.h"
#include "GlobalVar.h"

int input_section(std::ifstream &inputStream, task_config* taskConfig)
{
    char templine[64];
    while(inputStream.getline(templine,64))
    {
	    if(templine[0]=='#') continue;
	    taskConfig->update_config(templine);
    }
    return 0;
}

int main(int argc, char ** argv)
{
    if(argc<2)
    {
        std::cout<<"No Input File"<<std::endl;
        return -1;
    }
    task_config taskConfig;
    std::ifstream inputStream(argv[1]);
    input_section(inputStream,&taskConfig);
    if(taskConfig.stage==1)
    {
        stage1main(&taskConfig);
    }
    else if(taskConfig.stage==2)
    {
        stage2main(&taskConfig);
    }
    else if(taskConfig.stage==3)
    {
        stage3main(&taskConfig);
    }
    else if(taskConfig.stage==4)
    {
        stage4main(&taskConfig);
    }
    else if(taskConfig.stage==5)
    {
        stage5main(&taskConfig);
    }
    else
    {
        std::cout<<"Invalid Stage"<<std::endl;
        //Default Stage: Nothing to do here
        return -1;
    }
    return 0;
}

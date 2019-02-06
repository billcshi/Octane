#include <cstring>
#include <iostream>

#include "task_config.h"

#define stage_fieldname "stage"
#define num_routers_fieldname "num_routers"

void task_config::update_config(char * inputString)
{
    char field[64];
    int i=0;
    while(inputString[i]!=' ')
    {
        field[i]=inputString[i];
        i++;
    }
    field[i]='\0';
    if(std::strcmp(field,stage_fieldname)==0)
    {
        char number[20];
        i++;
	int j=0;
        while(inputString[i]!='\0')
        {
            number[j]=inputString[i];
            i++;
	    j++;
        }
	number[j]='\0';
        int i_number=std::stoi(number);
        stage=i_number;
    }
    else if(std::strcmp(field,num_routers_fieldname)==0)
    {
	char number[20];
        i++;
	int j=0;
        while(inputString[i]!='\0')
        {
            number[j]=inputString[i];
            i++;
	    j++;
        }
	number[j]='\0';
        int i_number=std::stoi(number);
        num_routers=i_number;
    }
    else
    {
        // defalut skip
    }
    return;
}

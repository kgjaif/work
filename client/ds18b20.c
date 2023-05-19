#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>  
#include <stdlib.h>


int ds18b20_get_temperature(float *temper)
{
    DIR                             *dirp=NULL;
    struct  dirent       *direntp=NULL;
    char                  path_sn[32];
    char                  ds18b20_path[64];
    int                   found=0;
    int                   fd=-1;
    char                  buf[128];
    char            *ptr=NULL;
    char            w1_path[128]="/sys/bus/w1/devices/";

    dirp=opendir(w1_path);
    if(!dirp)
    {   
        printf("open the folser %s failure:%s\n",w1_path,strerror(errno));
        return -6;
    }       

    while((direntp=readdir(dirp))!=NULL)
    {   
        if(strcmp(direntp->d_name,".")==strcmp(direntp->d_name,"..")==0)
        {   
            continue;
            printf("%s\n",direntp->d_name);
        }       
        if(strstr(direntp->d_name,"28-"))
        {   
            strncpy(path_sn,direntp->d_name,sizeof(path_sn));
        }       
        found=1;
    }       

    closedir(dirp);

    if(!found)
    {   
        printf("Can Not found ds18b20 chipset\n");
        return -7;
    }       

    strncat(w1_path,path_sn,sizeof(w1_path)-strlen(w1_path));
    strncat(w1_path,"/w1_slave",sizeof(w1_path)-strlen(w1_path));

    if((fd=open(w1_path,O_RDONLY))<0)
    {   
        printf("Open the file failure:%s\n",strerror(errno));
        return -8;
    }

    memset(buf,0,sizeof(buf));
    if(read(fd,buf,sizeof(buf))<0)
    {
        printf("read data from fd =%d failure:%s\n",fd,strerror(errno));
        return -9;
    }

    ptr=strstr(buf,"t=");
    if(!ptr)
    {
        printf("Can Not find t=???\n");
        return -1;
    }
    ptr+=2;
    *temper=atof(ptr)/1000;

    write(fd,buf,sizeof(buf));

    close(fd);

    return 0;
}


#include "fileDeque.h"
#include "common.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

std::mutex FileDeque::innerMutex;
int  FileDeque::_dirSplitNum;
volatile std::list<FILEDATA> FileDeque::_innerFilesDataQueue;
std::atomic<bool> FileDeque::_isRunning(bool(false));
int FileDeque::_nowCount(0);
int FileDeque::_innerSize(0);
DIR* FileDeque::_innerDir=NULL;
std::thread* FileDeque::_innerT;
std::string  FileDeque::_path;
int FileDeque::_srcPid; 
int FileDeque::_curPid;
std::string FileDeque::_pidFile;
static MYSQL* FileDeque::_sqlIns=NULL;


void FileDeque::getFileHandlerCb()
{
    struct dirent *dir_info = NULL;
    char filename[1024];
    uint32_t index=0;
    FILEDATA tmp;

    while(FileDeque::_isRunning)
    {
        while(  NULL !=(dir_info=readdir(FileDeque::_innerDir)) ) 
        {
            if((strcmp(dir_info->d_name,".")==0)||(strcmp(dir_info->d_name,"..")==0))
            {
                continue;
            }
            std::unique_lock<std::mutex> holder(FileDeque::innerMutex);
            if(FileDeque::_nowCount <= FileDeque::_innerSize)
            {
                snprintf(filename,1024,"/%s",dir_info->d_name);
                std::string finPath = _path+std::string(filename);
                tmp.pid = _curPid;
                tmp.dir_index = _curPid/_dirSplitNum;
                tmp.path = finPath;
                _innerFilesDataQueue.push_back(tmp); 
                ++FileDeque::_nowCount; 
                ++_curPid;
            }
            else
                sleep(1);         
        }

        FileDeque::_isRunning = false;
        closedir(FileDeque::_innerDir);
        FileDeque::setPid();
    }
   
}

int FileDeque::init(const Conf & conf,const std::string& dir)
{
    int ret=0;
    _dirSplitNum =conf._dirSplitNum;
    _innerSize = conf._innerSize;
    _pidFile = conf._pidFile;

    mylogD("dir is %s",dir.c_str());
    _path=dir;
    FileDeque::_innerDir=opendir(dir.c_str());
    if(FileDeque::_innerDir==NULL){
       mylogF("Open dir error !\n");
       ret = -1;
    }
    else
    {
        mylogD("open dir ok");
        ret = 0;
    }
    
    if( _sqlIns==NULL )
    {
       _sqlIns = openMysql(conf._sqlIp.c_str(),conf._sqlPort,conf._sqlUser.c_str(),conf._sqlPwd.c_str(),conf._sqlDb.c_str());
    }
    return ret;
}

int FileDeque::unInit()
{
    if(_sqlIns!=NULL)
    {
        closeMysql(_sqlIns);
        _sqlIns=NULL;
    }
}

void FileDeque::doStart()
{
    FileDeque::_isRunning = true;
    FileDeque::getPid();
   _innerT = new std::thread(FileDeque::getFileHandlerCb);
   _innerT->detach();

}

void FileDeque::doStop()
{
    FileDeque::_isRunning=false;
}


FILEDATA FileDeque::getFileData()
{
    FILEDATA ret;
    std::unique_lock<std::mutex> holder(FileDeque::innerMutex);
    if(_nowCount > 0)
    {
        ret = _innerFilesDataQueue.front();
        _innerFilesDataQueue.pop_front();
        --_nowCount;
   }
   else
   {
       _nowCount=0;
       ret.pid=0; 
       ret.dir_index=0;
       ret.path="";
   }

    return ret;
}

bool FileDeque::isRunning()
{
    return FileDeque::_isRunning;
}


/*判断文件是否是个目录文件，是目录返回 0 ，不是目录返回 -1*/
int FileDeque::_testdir(char *path)
{
    if(path==NULL)
        return -1;
    struct stat buf;
    if(lstat(path,&buf)<0)
    {
       return -1;
    }
    if(S_ISDIR(buf.st_mode))
    {
       return 0; //directory
    }
    return -1;
}


bool FileDeque::getPid()
{
    FILE* file = NULL;
    if((file = fopen(_pidFile.c_str(),"r+"))==NULL)
    {
        mylogF("open pid file fail");
        return false;
    }
    char pidStr[1024]={0};
    fgets(pidStr,1024,file);
    _srcPid = atoi(pidStr);
    if(_srcPid<=0)
        _srcPid=0;
    _curPid= _srcPid;
    fclose(file);
    return true;
}

bool FileDeque::setPid()
{
    FILE* file = NULL;
    if((file = fopen(_pidFile.c_str(),"w+"))==NULL)
    {
        mylogF("open pid file fail");
        return false;
    }
    char pidStr[1024]={0};
    snprintf(pidStr,1024,"%d",_curPid);
    fputs(pidStr,file);
    fclose(file);
    char sql[1024]="0";
    SQLDATA res; 
    snprintf(sql,1024,"INSERT INTO info_status (uniq,count) VALUES (0,%d) ON DUPLICATE KEY UPDATE count=%d;",_curPid,_curPid);
    bool ret =  doSql(_sqlIns,sql,res);
    return ret;
}
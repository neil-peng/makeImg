#pragma once
#include <list>
#include <string>
#include <atomic>
#include <sys/types.h>
#include <dirent.h> 
#include <mutex>
#include <thread>
#include <string>
#include "common.h"
#include "mmysql.h"
//typedef std::pair<uint32_t,uint32_t,std::string> FILEDATA;// <pid , path index , src path>
struct FILEDATA
{
    FILEDATA():pid(0),dir_index(0),path(""){}
    uint32_t pid;
    uint32_t dir_index;
    std::string path;
};
class FileDeque
{
private:
    static int _innerSize;
    static int _dirSplitNum;
    static std::string _pidFile;
    static int _curPid;
    static int _srcPid;
    static std::atomic<bool> _isRunning;
    static int _nowCount;
    static volatile std::list<FILEDATA> _innerFilesDataQueue;
    static DIR* _innerDir;
    static std::thread* _innerT;
    static int _testdir(char *path);
    static void getFileHandlerCb();   
    static std::string _path;
    static MYSQL* _sqlIns;
    FileDeque(FileDeque&){}
    FileDeque(){}
    FileDeque& operator=(FileDeque&){}
    
public:
    
    static std::mutex innerMutex;
    static int init(const Conf& conf,const std::string& dir);
    static void doStart();
    static void doStop();
    static FILEDATA getFileData();
    static bool isRunning();
    static bool getPid();
    static bool setPid();
    static int unInit();
};

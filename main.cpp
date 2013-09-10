#include "common.h"
#include "stdlib.h"
#include "worker.h"
#include "./lib/lightConf/ConfigFile.h"
const int MAXPATH = 1024;
Conf gConf;
void loadConf()
{
    ConfigFile conf("./conf/processImg.conf");
    try
    {
        gConf._threadNum = (int)conf.Value("common","threadNum");
        int count = (int)conf.Value("srcDirList","count");
        for(int i=0;i<count;i++)
        {   
            char tmp[128]={0};
            snprintf(tmp,128,"%d",i);
            mylogD("here is %s",tmp);
            std::string item = (std::string) conf.Value("srcDirList",std::string(tmp));
            gConf._srcList.push_back(item);
        }
        gConf._baseDir = (std::string) conf.Value("common","baseDir");
        gConf._pidFile = (std::string) conf.Value("common","pidFile");
        gConf._innerSize = (int) conf.Value("common","processInnerSize");
        gConf._dirSplitNum = (int)conf.Value("common","eachDir");
        gConf._pageCount = (int)conf.Value("common","pageCount");
        gConf._sqlIp = (std::string) conf.Value("mysql","ip");
        gConf._sqlPort = (int) conf.Value("mysql","port");
        gConf._sqlUser = (std::string) conf.Value("mysql","user");
        gConf._sqlPwd = (std::string) conf.Value("mysql","pwd");
        gConf._sqlDb = (std::string) conf.Value("mysql","db");
        gConf._splitNum = (int) conf.Value("mysql","eachSplit");
        gConf._mcIp = (std::string) conf.Value("mc","ip");
        gConf._mcPort = (int) conf.Value("mc","port");
    }
    catch(char const* msg)
    {
        mylogD("catch exception msg: %s",msg);
        exit(0);
    }
}

int main( int argc, char** argv )
{
    loadConf();

    Worker **worker = new Worker*[gConf._threadNum];
    
    for(int i=0;i<gConf._threadNum;i++)
    {
        worker[i] = new Worker();
        worker[i]->init(gConf);
    }
    
    for(std::string eachDir : gConf._srcList)
    {
        FileDeque::init(gConf,eachDir);
        FileDeque::doStart();
        for(int i=0;i<gConf._threadNum;i++)
        {
            worker[i]->start();
        }
        for(int i=0;i<gConf._threadNum;i++)
        {
            worker[i]->join();
        }
    }
    
    for(int i=0;i<gConf._threadNum;i++)
    {
        worker[i]->unInit();
        delete worker[i];
    }
    FileDeque::unInit();
    delete worker;
    return 0;
}

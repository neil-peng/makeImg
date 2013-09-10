#include "worker.h"
#include "common.h"
#include "MyImage.h"
#include "util.h"

static uint32_t Worker::_sqlSplit = 0;
static uint32_t Worker::_pageCount = 0;
static std::string Worker::_baseDir="";

static bool makeStaicSql(MYSQL* sqlIns ,uint32_t pid,const std::string& path,uint32_t sqlSplit)
{
    char sql[2048]={0};
    uint32_t sqlIndex = pid/sqlSplit;
    snprintf(sql,2048,"insert into info_static_%d values(%d,\"%s\",0,0)", sqlIndex,pid,path.c_str() );
    SQLDATA res; 
    bool ret =  doSql(sqlIns,sql,res);
    for(std::pair<std::string,std::string > each : res)
        mylogD("insert static sql res:%s, %s",each.first.c_str(), each.second.c_str() );
    return ret;

}

static bool makeWeightMc(memcached_st* mcIns ,uint32_t pid,int weight)
{
    char mcKey[1024]={0};
    char mcVal[1024]={0};
    snprintf(mcKey,1024,"uptW_%d",pid);
    snprintf(mcVal,1024,"%d",weight);
    return setMc(mcIns,mcKey,mcVal);
}

static bool makeDynamicSqlAndMc(MYSQL* sqlIns , memcached_st* mcIns, uint32_t pid,uint32_t pageCount,uint32_t sqlSplit)
{
    char sql[2048]={0};
    uint32_t pageIndex = pid%pageCount;
    int weight = getRandInt(0,6);
    //uint32_t sqlIndex = pid/sqlSplit;
    snprintf(sql,2048,"insert into info_dynamic values(%d,%d,%d)",pid,pageIndex,weight);
    SQLDATA res; 
    bool ret =  doSql(sqlIns,sql,res);
    for(std::pair<std::string,std::string > each : res)
        mylogD("insert dynamic sql res:%s, %s",each.first.c_str(), each.second.c_str() );
    if(makeWeightMc(mcIns,pid,weight)!=true)
        return false;
    return ret;

}



static bool Worker::_processImg(const std::string& inPath,uint32_t index, const std::string& imgName, uint32_t pid, MYSQL* sqlIns,memcached_st* mcIns)
{
    char indexDir[1024]={0};
    char showDir[1024]={0};
    char cpid[1024]={0};
    std::string wh;
    snprintf(indexDir,1024,"/index%d/",index);
    snprintf(showDir,1024,"/show%d/",index);
    snprintf(cpid,1024,"%d_",pid);
    std::string smallDirFull = _baseDir +  indexDir;
    std::string showDirFull = _baseDir + showDir;

    MyImage proImg(inPath);
    
    if(""==(wh = proImg.getWidthHeight()))    
        return false;
    std::string sImgname = cpid + wh +".jpg";
    std::string showImgname = getMd5(sImgname); 

    std::string smallImg = smallDirFull + sImgname;
    std::string showImg = showDirFull + showImgname + ".jpg";
    
    mylogD("get smalldir: %s , smallImg: %s, showdir:%s, showImg:%s. pid: %d",smallDirFull.c_str(), smallImg.c_str(), showDirFull.c_str(),showImg.c_str() ,pid);
    makeStaicSql(sqlIns,pid,sImgname,_sqlSplit);    
    makeDynamicSqlAndMc(sqlIns, mcIns, pid,_pageCount,_sqlSplit);

    if(! proImg.reSize(420,280) )
        return false;
    
    if(! proImg.saveDstImg(smallImg,smallDirFull))
        return false;
   
    if(! proImg.saveSrcImg(showImg,showDirFull))
        return false;
   return true;
}


static void Worker::_proessCb( MYSQL* sqlIns ,memcached_st* mcIns)
{	
	int emptyTime=0;
    while(true)
	{
       FILEDATA imgData = FileDeque::getFileData();
       std::string imgPath = imgData.path;
       uint32_t pid = imgData.pid;
       uint32_t index = imgData.dir_index;
       if(imgPath =="")
		{
            sleep(1);
        	emptyTime++;
            mylogD("the pid:  %d get null",pthread_self());
        }
		else
		{
            emptyTime=0;
            unsigned pos  =  imgPath.find(".jpg");
            unsigned dirPos  =  imgPath.find_last_of("/");
            if(pos>10240 || dirPos>10240 )
            {
                mylogD("spcail case: %s",imgPath.c_str()); 
                continue;
            }

            std::string imgName = imgPath.substr(dirPos+1,pos-dirPos-1);
            if(_processImg(imgPath,index,imgName,pid,sqlIns,mcIns))
              //  mylogD("prcess img succ: %s , and small dir :%s, small img: %s , and md5dir:%s ,md5 img:%s ,pid: %d", imgPath.c_str(),smallDirPath.c_str(), smallImgPath.c_str(), md5DirPath.c_str(),md5ImgPath.c_str() ,pid );
                mylogD("prcess img succ: %s", imgPath.c_str());
            else
                mylogD("prcess img fail: %s", imgPath.c_str());
        }
		if(emptyTime>=5)
		{
        	break;
        }
		
	}
    mylogD("the pthread id:  %d finish",pthread_self());
}

int Worker::init(const Conf& conf)
{
    if( _sqlIns==NULL )
    {
       _sqlIns = openMysql(conf._sqlIp.c_str(),conf._sqlPort,conf._sqlUser.c_str(),conf._sqlPwd.c_str(),conf._sqlDb.c_str());
       _sqlSplit = conf._splitNum;
    }
    if( _mcIns==NULL)
    {
        _mcIns = openMc(conf._mcIp.c_str(),conf._mcPort);
    }
    if(_sqlSplit==0)
    {
        _sqlSplit=conf._splitNum;
    }
    if(_pageCount==0)
    {
        _pageCount=conf._pageCount;
    }
    if(_baseDir=="")
    {
        _baseDir=conf._baseDir;
    }
    if( _sqlIns==NULL || _mcIns==NULL)
        return -1;
    return 0;
}

int Worker::unInit()
{
    if(_sqlIns!=NULL)
    {
        closeMysql(_sqlIns);
        _sqlIns=NULL;
    }

    if(_mcIns!=NULL)
    {
        closeMc(_mcIns);
        _mcIns=NULL;
    }
    return 0;
}



int Worker::start()
{
	if(_p_processT==NULL)
		_p_processT = new std::thread(Worker::_proessCb,_sqlIns,_mcIns);
	return 0;
}


int Worker::join()
{
    while(FileDeque::isRunning())
        sleep(1);
	if(_p_processT!=NULL)
	{
    	_p_processT->join();
        delete _p_processT;
        _p_processT=NULL;
    }
}

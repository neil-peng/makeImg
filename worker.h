#pragma once
#include "fileDeque.h"
#include <thread>
#include "common.h"
#include "mmysql.h"
#include "mmc.h"
class Worker
{
private:
	std::thread* _p_processT;
	static bool _processImg(const std::string& inPath ,uint32_t index, const std::string& imgName, uint32_t pid,MYSQL* sqlIns ,memcached_st* mcIns,int wt,int ht);
	static void _proessCb(MYSQL* sqlIns ,memcached_st* mcIns,int wt,int ht);
	MYSQL* _sqlIns;
    memcached_st* _mcIns;
	static uint32_t _sqlSplit;
	static uint32_t _pageCount;
	static std::string _baseDir;
	int _wt;
	int _ht;

	Worker(Worker&){}
	Worker& operator=(Worker&){}


public:
	Worker():_p_processT(NULL),_sqlIns(NULL),_mcIns(NULL){
	}
    //int init( const char* ip,int port,const char* user, const char* pwd,const char* db, uint32_t sqlSplit); 
    int init(const Conf& conf); 
    int unInit();
    int start();
	int join();
	~Worker(){
		if(_p_processT!=NULL)
		{
			delete _p_processT;
			_p_processT=NULL;
            mylogF("should not happen");
		}
	}

};

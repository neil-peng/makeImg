#pragma once
#include <unistd.h>
#include <string>
#include <vector>

#define mylog(format,args...) do{\
    fprintf(stdout,format"\n",##args);\
}while(0)

#define mylogD(format,args...) do{\
    fprintf(stdout,"DEBUG: "format"\n",##args);\
}while(0)

#define mylogS(check,format,args...) do{\
    if(check)\
        fprintf(stdout,format"\n",##args);\
}while(0)


#define mylogF(format,args...) do{\
    fprintf(stdout,"FATAL: "format"\n",##args);\
}while(0)

#define mylogW(format,args...) do{\
    fprintf(stdout,"WARNNING: "format"\n",##args);\
}while(0)


struct Conf{
    int _threadNum;
    int _dirSplitNum;
    int _innerSize;
    std::string _pidFile;
    std::vector<std::string> _srcList;
    std::string _baseDir;
    std::string _sqlIp;
    int _sqlPort;
    int _pageCount;
    std::string _sqlUser;
    std::string _sqlPwd;
    std::string _sqlDb;
    std::string _mcIp;
    int _mcPort;

    int _splitNum; 
};

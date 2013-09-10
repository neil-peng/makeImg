#include "mmysql.h"
#include "stdio.h"
#include "stdlib.h"
MYSQL* openMysql(const char* serv,int port,const char* user, const char* pwd,const char* db)
{
   MYSQL *conn=NULL;;
  /* db configure*/
   conn = mysql_init(NULL);

   /* Connect to database */
   if (!mysql_real_connect(conn, serv,
         user, pwd, db, port, NULL, 0)) {
      mylogD("%s\n", mysql_error(conn));
      exit(0);
   } 
   mysql_set_character_set(conn,"utf8"); 
   return conn;
}

//select * from 
bool doSql(MYSQL *conn,char* sql,SQLDATA& data)
{
   MYSQL_ROW row;
   MYSQL_FIELD *field;
   MYSQL_RES *res=NULL;
   int index=0;
   if (mysql_query(conn, sql)) {
        mylogD("%s\n", mysql_error(conn));
        exit(0);
   }

   res = mysql_store_result(conn);
   
	if(res!=NULL)
	{
		if(1!=mysql_num_rows(res))
		return false;

    	row = mysql_fetch_row(res);

		while((field = mysql_fetch_field(res)))
		{
				std::string key(field->name);
				std::string value(row[index++]);
//				mylog("%s  %s \n",key.c_str(),value.c_str());
				data.push_back(std::make_pair(key,value));
		}

    	mysql_free_result(res);
		return true;
	}
    return false;
}

MYSQL* closeMysql(MYSQL* conn)
{
    mysql_close(conn);
    conn=NULL;
    return conn;
}



// This file is part of ScopePort (Linux server).
//
// Copyright 2007, 2008 Lennart Koopmann
//
// ScopePort (Linux server) is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ScopePort (Linux server) is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ScopePort (Linux server).  If not, see <http://www.gnu.org/licenses/>.

#include "../internal.h"
#include "../log/Log.h"
#include "../database/Database.h"
#include "../notifications/GeneralNotifications.h"

Log::Log(const char* myLogfile, mySQLData myDBData)
			: Database(myDBData) {
	logfile = myLogfile;
}

void Log::putLog(int severity, string errorcode, string logmsg){
	time_t rawtime;
  time(&rawtime);
  char* logtime = noNewLine(ctime(&rawtime), strlen(ctime(&rawtime)));
	ofstream log("/var/log/scopeport-server.log", ios::app);
	if(!log.fail()){
		if(logmsg != ".newrun"){
			log << logtime << " - " << logmsg << endl;
			if(initConnection()){
				MYSQL* init = getHandle();
				char *safeMsg = new char[strlen(logmsg.c_str())*2 + 1];
				mysql_real_escape_string(init, safeMsg,logmsg.c_str(),strlen(logmsg.c_str()));
				stringstream query;
				query 	<< "INSERT INTO `logmessages` (logtime,severity,errorcode,logmsg) VALUES('"
						<< rawtime
						<< "',\'"
						<< severity
						<< "',\'"
						<< errorcode
						<< "',\'"
						<< safeMsg
						<< "\')";
				mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str()));
				mysql_close(init);
			}
		}else{
			log << "--------------" << endl;
		}
	}
	log.close();
}

void Log::debug(bool debug, string msg){
  if(!debug){
    return;
  }

	time_t rawtime;
  time(&rawtime);
  char* logtime = noNewLine(ctime(&rawtime), strlen(ctime(&rawtime)));
  cout << logtime << " - " << msg << endl;
}

// This file is part of ScopePort (Linux server).
//
// Copyright 2009 Lennart Koopmann
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

#include "Clickatell.h"
#include "../internal.h"
#include "../database/Database.h"

mobilecData Clickatell::fetchSettings(mySQLData dbData){
  mobilecData clickatell;

  Database db(dbData);
  if(db.initConnection()){
    const char* getSettingsSQL = "SELECT doMobileClickatell, mobilecUsername, mobilecPassword"
                                "mobilecAPIID FROM settings";
    if(mysql_real_query(db.getHandle(), getSettingsSQL, strlen(getSettingsSQL)) == 0){
      MYSQL_RES* res;
      if((res = mysql_store_result(db.getHandle())) != NULL){
        MYSQL_ROW row;
        row = mysql_fetch_row(res);
        stringstream result;
        if(mysql_num_rows(res) > 0){

          // doMobileClickatell
          if(row[0] != NULL){
            if(strcmp(row[0], "1") == 0){
              clickatell.doMobileC = 1;
            }else{
              clickatell.doMobileC = 0;
            }
          }

          // mobilecUsername
          if(row[1] != NULL){
            clickatell.username = row[1];
          }

          // mobilecPassword
          if(row[2] != NULL){
            clickatell.username = row[2];
          }

          // mobilecPassword
          if(row[3] != NULL){
            clickatell.apiID = row[3];
          }

        }else{
          // Nothing fetched. Disable.
          clickatell.doMobileC = 0;
        }
      }else{
        // Error. Disable.
    	clickatell.doMobileC = 0;
      }
    }else{
      // Query failed. Disable.
      clickatell.doMobileC = 0;
    }
    mysql_close(db.getHandle());
  }else{
    // Could not connect to DB. Disable.
	clickatell.doMobileC = 0;
  }

  return clickatell;
}

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

#include "Cloud.h"
#include "../database/Database.h"
#include "../internal.h"

Cloud::Cloud(int myNodeID, mySQLData myDBData){
  nodeID = myNodeID;
  dbData = myDBData;
}

int Cloud::checkNodeID(int nodeID, Database db){
  // Connected to database.  
  stringstream query;
  query << "SELECT id FROM nodes WHERE id = " << nodeID;

  // Count if there is a node with the given ID.
  if(db.getNumOfResults(query.str()) == 1){
    // Yes.
    return 1;
  }else{
    // No.
    return 0;
  }

  // Could not connect to database.
  return -1;
}

bool Cloud::setTakeoff(int nodeID, Database db){
  time_t rawtime;
	time(&rawtime);
  
  stringstream query;
  query << "UPDATE nodes SET takeoff = " << rawtime << " WHERE id = " << nodeID;

  // Perform the query.
  if(db.setQuery(db.getHandle(), query.str())){
    // Query has been performed. No problems.
    return 1;
  }else{
    // An error occured.
    return 0;
  }
}

bool Cloud::updateOwnStatus(Database db){
  time_t rawtime;
	time(&rawtime);
  
  stringstream query;
  query << "UPDATE nodes SET last_update = " << rawtime << ", consumption = 1 WHERE id = " << nodeID;

  // Perform the query.
  if(db.setQuery(db.getHandle(), query.str())){
    // Query has been performed. No problems.
    return 1;
  }else{
    // An error occured.
    return 0;
  }
}

unsigned int Cloud::getNumberOfOwnServices(Database db){
  stringstream query;
  query << "SELECT id FROM services WHERE node_id = " << nodeID;
  return db.getNumOfResults(query.str());
}


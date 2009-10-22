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

Cloud::Cloud(int myNodeID, mySQLData myDBData)
{
  nodeID = myNodeID;
  dbData = myDBData;
}


int Cloud::checkNodeID(unsigned int nodeID, Database db)
{
  // Connected to database.
  stringstream query;
  query << "SELECT id FROM nodes WHERE id = " << nodeID;

  // Count if there is a node with the given ID.
  if(db.getNumOfResults(query.str()) == 1)
  {
    // Yes.
    return 1;
  }
  else
  {
    // No.
    return 0;
  }

  // Could not connect to database.
  return -1;
}


bool Cloud::setTakeoff(unsigned int nodeID, Database db)
{
  time_t rawtime;
  time(&rawtime);

  stringstream query;
  query << "UPDATE nodes SET takeoff = " << rawtime << " WHERE id = " << nodeID;

  // Perform the query.
  if(db.setQuery(db.getHandle(), query.str()))
  {
    // Query has been performed. No problems.
    return 1;
  }
  else
  {
    // An error occured.
    return 0;
  }
}


bool Cloud::updateOwnStatus(Database db)
{
  time_t rawtime;
  time(&rawtime);

  stringstream query;
  query << "UPDATE nodes SET last_update = "
    << rawtime
    << ", consumption = 1 WHERE id = "
    << nodeID;

  // Perform the query.
  if(db.setQuery(db.getHandle(), query.str()))
  {
    // Query has been performed. No problems.
    return 1;
  }
  else
  {
    // An error occured.
    return 0;
  }
}


unsigned int Cloud::getNumberOfOwnServices(Database db)
{
  stringstream query;
  query << "SELECT id FROM services WHERE node_id = " << nodeID;
  return db.getNumOfResults(query.str());
}


unsigned int Cloud::getIdOfNodeWithMostServices(Database db)
{
  time_t rawtime;
  time(&rawtime);

  stringstream query;
  query << "SELECT s.node_id, count(*) AS count "
    "FROM services AS s "
    "LEFT JOIN nodes AS n ON s.node_id = n.id "
    "WHERE n.last_update >  " << rawtime-10 << " "
    "GROUP BY s.node_id ORDER BY count DESC";
  return(stringToInteger(db.sGetQuery(query.str())));
}


unsigned int Cloud::getNumberOfServicesFromNode(unsigned int foreignNodeID, Database db)
{
  stringstream query;
  query << "SELECT id FROM services WHERE node_id = "
    << foreignNodeID;
  return db.getNumOfResults(query.str());
}


bool Cloud::action_requestServices(unsigned int count, unsigned int from_node, Database db)
{
  return storeAction(from_node, "service_request", integerToString(count), generateConversationID(), db);
}


bool Cloud::storeAction(unsigned int receiver, string type, string value, unsigned int conversation_id, Database db)
{
  stringstream query;
  query << "INSERT INTO nodecommunications(sender_id, receiver_id, type, value, timestamp, conversation_id) "
    "VALUES("
    << getOwnID()
    << ","
    << receiver
    << ",'"
    << type
    << "','"
    << value
    << "', NOW(), "
    << conversation_id
    << ")";

  return db.setQuery(db.getHandle(), query.str());
}


bool Cloud::action_replyServiceRequest(unsigned int handoverRequester, string status, unsigned int conversationID, Database db)
{
  return storeAction(handoverRequester, "service_request_response", status, conversationID, db);
}


void Cloud::action_logEvent(string message, Database db)
{
  storeAction(0, "log_message", message, generateConversationID(), db);
}


unsigned int Cloud::checkForServiceHandoverRequest(Database db)
{
  stringstream query;
  query << "SELECT conversation_id "
    "FROM nodecommunications "
    "WHERE type = 'service_request' "
    "AND receiver_id = "
    << getOwnID()
    << " AND TIMEDIFF(timestamp, NOW()) < '00:00:15' ORDER BY timestamp DESC "
    << "LIMIT 1";
  string result = db.sGetQuery(query.str());
  if(result == "NULL")
  {
    return 0;
  }
  else
  {
    return stringToInteger(result);
  }
}


unsigned int Cloud::getNumberOfRequestedServices(Database db, unsigned int conversationID)
{
  stringstream query;
  query << "SELECT value "
    "FROM nodecommunications "
    "WHERE conversation_id = "
    << conversationID
    << " ORDER BY timestamp DESC"
    << " LIMIT 1";
  string result = db.sGetQuery(query.str());
  if(result == "NULL")
  {
    return 0;
  }
  else
  {
    return stringToInteger(result);
  }
}


unsigned int Cloud::getNodeIdOfHandoverRequester(Database db, unsigned int conversationID)
{
  stringstream query;
  query << "SELECT sender_id "
    "FROM nodecommunications "
    "WHERE conversation_id = "
    << conversationID
    << " ORDER BY timestamp DESC "
    << "LIMIT 1";
  string result = db.sGetQuery(query.str());
  if(result == "NULL")
  {
    return 0;
  }
  else
  {
    return stringToInteger(result);
  }
}


bool Cloud::handOverServices(unsigned int numberOfServices, unsigned int handoverRequester, Database db)
{
  stringstream query;
  query << "UPDATE services SET reserved_for = " << handoverRequester
    << ", reserved_on = NOW() "
    "WHERE node_id = "
    << getOwnID()
    << " LIMIT " << numberOfServices;
  return db.setQuery(db.getHandle(), query.str());
}


unsigned int Cloud::generateConversationID()
{
  // Generate random number.
  struct timeval tv;
  gettimeofday(&tv, NULL);
  srand(tv.tv_sec * tv.tv_usec);

  unsigned int result;

  stringstream conversationID;
  conversationID << getOwnID() << rand() % 50000;
  result = stringToInteger(conversationID.str());
  return result;
}


unsigned int Cloud::getOwnID()
{
  return nodeID;
}

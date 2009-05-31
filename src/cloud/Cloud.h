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

#ifndef CLOUD_H_
#define CLOUD_H_

#include "../internal.h"
#include "../database/Database.h"

class Cloud{
  private:
    int nodeID;
    mySQLData dbData;

    unsigned int generateConversationID();
    bool storeAction(unsigned int receiver, string type, string value, unsigned int conversation_id, Database db);

  public:
    Cloud(int myNodeID, mySQLData myDBData);
    bool updateOwnStatus(Database db);

    // Requesting service handovers.
    unsigned int getNumberOfOwnServices(Database db);
    unsigned int getIdOfNodeWithMostServices(Database db);
    unsigned int getNumberOfServicesFromNode(unsigned int foreignNodeID, Database db);

    // Handling service handover requests addressed to us.
    unsigned int checkForServiceHandoverRequest(Database db);
    unsigned int getNumberOfRequestedServices(Database db, unsigned int conversationID);
    unsigned int getNodeIdOfHandoverRequester(Database db, unsigned int conversationID);
    bool handOverServices(unsigned int numberOfServices, unsigned int handoverRequester, Database db);

    // Actions.
    void action_logEvent(string message, Database db);
    bool action_requestServices(unsigned int count, unsigned int from_node, Database db);
    bool action_replyServiceRequest(unsigned int handoverRequester, string status, unsigned int conversationID, Database db);

    unsigned int getOwnID();

    static int checkNodeID(unsigned int nodeID, Database db);
    static bool setTakeoff(unsigned int nodeID, Database db);
};

#endif /*CLOUD_H_*/

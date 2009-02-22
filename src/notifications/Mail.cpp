// This file is part of ScopePort (Linux server).
//
// Copyright 2008, 2009 Lennart Koopmann
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

#include "Mail.h"
#include "../internal.h"
#include "../database/Database.h"

Mail::Mail(mailingData myMailData){
	mailData = myMailData;
}

void Mail::updateSettings(mailingData myMailData){
	mailData = myMailData;
}

int sock;

bool send_socket(string msg){
	ssize_t len;
	len = write(sock,msg.c_str(),strlen(msg.c_str()));
	if(len <= 0)
		return 0;
	return 1;
}

#define MAILBUFSIZE 1024

bool read_socket(){
	ssize_t len;
	char mailBuf[MAILBUFSIZE];
	len = read(sock,mailBuf, MAILBUFSIZE-1);
	if(len <= 0)
		return 0;
	return 1;
}

static const string base64_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

string encodeBase64(string stringToEncode) {
	unsigned char const * bytes_to_encode = reinterpret_cast<const unsigned char*>(stringToEncode.c_str());
	unsigned int in_len = stringToEncode.length();
	string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while(in_len--){
		char_array_3[i++] = * (bytes_to_encode++);
		if (i == 3){
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if(i){
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for(j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while((i++ < 3))
			ret += '=';
	}

	return ret;
}

bool Mail::sendMail(string toMail, string subject, string mailText){

	// Prepared debug data.
	//	cout	<< "from: "
	//			<< mailData.mailFrom
	//			<< endl
	//			<< "to: "
	//			<< toMail
	//			<< endl
	//			<< "subject: "
	//			<< subject
	//			<< endl
	//			<< "text: "
	//			<< mailText
	//			<< endl << endl
	//			<< "hostname: " << mailData.mailHostname << endl
	//			<< "user: " << mailData.mailUser << endl
	//			<< "password: " << mailData.mailPass
	//			<< endl << endl;

	struct sockaddr_in server;
	struct hostent *host;
	// Create socket.
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		// Creating socket failed.
		return 0;
	}

	// Verify host.
	server.sin_family = AF_INET;
	host = gethostbyname(mailData.mailServer.c_str());
	if(host==(struct hostent *) 0){
		// Host unknown.
		return 0;
	}

	// Connect to defined SMTP port.
	memcpy((char *) &server.sin_addr, (char *) host->h_addr, host->h_length);
	server.sin_port=htons(mailData.mailPort);

	if(connect(sock, (struct sockaddr *) &server, sizeof server) < 0) {
		// Could not connect.
		return 0;
	}

	// A little bit Conversation.

	// Welcome string.
	if(!read_socket())
		return 0;

	if(mailData.mailUseAuth){
		// EHLO.
		stringstream ehlo;
		ehlo	<< "EHLO "
				<< mailData.mailHostname
				<< endl;

		if(!send_socket(ehlo.str()))
			return 0;
		// Get reply.
		if(!read_socket())
			return 0;
		// Ask for AUTH LOGIN.
		if(!send_socket("AUTH LOGIN\n"))
			return 0;
		if(!read_socket())
			return 0;
		// Send username.
		if(!send_socket(encodeBase64(mailData.mailUser)))
			return 0;
		if(!send_socket("\n"))
			return 0;
		if(!read_socket())
			return 0;
		// Send Password.
		if(!send_socket(encodeBase64(mailData.mailPass)))
			return 0;
		if(!send_socket("\n"))
			return 0;
		if(!read_socket())
			return 0;
	}else{
		// HELO.
		stringstream helo;
		helo	<< "HELO "
				<< mailData.mailHostname
				<< endl;

		if(!send_socket(helo.str()))
			return 0;
		// Get reply.
		if(!read_socket())
			return 0;
	}

	// Sender.
	stringstream mailFrom;
	mailFrom	<< "Mail from: <"
				<< mailData.mailFrom
				<< ">"
				<< endl;

	if(!send_socket(mailFrom.str()))
		return 0;
	if(!read_socket())
		return 0;

	// Recipient.
	stringstream mailTo;
	mailTo	<< "RCPT to: <"
				<< toMail
				<< ">"
				<< endl;

	if(!send_socket(mailTo.str()))
		return 0;
	if(!read_socket())
		return 0;

	// Data.
	if(!send_socket("DATA\n"))
		return 0;
	if(!read_socket())
		return 0;

	// Header.
	stringstream headerTo;
	headerTo	<< "to: "
				<< toMail
				<< endl;
	if(!send_socket(headerTo.str()))
		return 0;

	stringstream headerFrom;
	headerFrom	<< "from: "
				<< mailData.mailFrom
				<< endl;
	if(!send_socket(headerFrom.str()))
		return 0;

	stringstream headerSubject;
	headerSubject	<< "subject: [ScopePort] "
					<< subject
					<< endl << endl;
	if(!send_socket(headerSubject.str()))
		return 0;
	if(!send_socket(mailText))
		return 0;
	if(!send_socket("\n\n.\n"))
		return 0;

	// Quit.
	if(!read_socket())
		return 0;

	if(!send_socket("QUIT\n"))
		return 0;

	if(!read_socket())
		return 0;

	// Close socket.
	close(sock);

	return 1;
}

mailingData Mail::fetchSettings(mySQLData dbData){
  mailingData mailData;

  Database db(dbData);
  if(db.initConnection()){
    const char* getSettingsSQL = "SELECT mail_enabled, mail_useauth, mail_server,"
                                "mail_port, mail_user, mail_pass, mail_hostname,"
                                "mail_from FROM settings";
    if(mysql_real_query(db.getHandle(), getSettingsSQL, strlen(getSettingsSQL)) == 0){
      MYSQL_RES* res;
      if((res = mysql_store_result(db.getHandle())) != NULL){
        MYSQL_ROW row;
        row = mysql_fetch_row(res);
        stringstream result;
        if(mysql_num_rows(res) > 0){

          // mail_enabled
          if(row[0] != NULL){
            if(strcmp(row[0], "1") == 0){
              mailData.doMailing = 1;
            }else{
              mailData.doMailing = 0;
            }
          }

          // mail_useauth
          if(row[1] != NULL){
            if(strcmp(row[1], "1") == 0){
              mailData.mailUseAuth = 1;
            }else{
              mailData.mailUseAuth = 0;
            }
          }

          // mail_server
          if(row[2] != NULL){
            mailData.mailServer = row[2];
          }

          // mail_port
          if(row[3] != NULL){
            mailData.mailPort = stringToInteger(row[3]);
          }

          // mail_user
          if(row[4] != NULL){
            mailData.mailUser = row[4];
          }

          // mail_pass
          if(row[5] != NULL){
            mailData.mailPass = row[5];
          }

          // mail_hostname
          if(row[6] != NULL){
            mailData.mailHostname = row[6];
          }

          // mail_from
          if(row[7] != NULL){
            mailData.mailFrom = row[7];
          }

        }else{
          // Nothing fetched. Disable mailing.
          mailData.doMailing = 0;
        }
      }else{
        // Error. Disable mailing.
        mailData.doMailing = 0;
      }
      mysql_free_result(res);
    }else{
      // Query failed. Disable mailing.
      mailData.doMailing = 0;
    }

    mysql_close(db.getHandle());
  }else{
    // Could not connect to DB. Diable mailing.
    mailData.doMailing = 0;
  }

  return mailData;
}

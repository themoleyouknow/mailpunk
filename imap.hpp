#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>

namespace IMAP {

class Session;
  
/* -------------------- Class: Message ------------------- */
class Message {
private:
        // Constructor attributes:
        Session* session;
        uint32_t const uid;
        // Other attributes:
        std::string from = "";
        std::string subject;
        std::string body;

  /* ----- setMessageFields ----- */
  // Function to set the message fields for a given set of message attributes:
        void setMessageFields(mailimap_msg_att* msg_att);
  
  /* ----- setFrom ----- */
  // Function to set the from field, called by setMessageFields (can process a list of senders!).
        void setFrom(clist* frm_list);
public:
  /* ----- CONSTRUCTOR ----- */
        Message(Session* session, uint32_t uid) : session(session), uid(uid) {};

  /* ----- getBody ----- */
  // Function to return the body of a message.
        std::string getBody() const {return body;}

  /* ----- getField ----- */
  // Function to return the appropriate field of a message.
        std::string getField(std::string field) const {
          if (field == "Subject") {return subject;}
          else if (field == "From") {return from;}
          else {return "";}
        }
  /* ----- setMessage ----- */
  // Function to set the message attributes.
        void setMessage();
        
  /* ----- deleteFromMailbox ----- */
  // Function to delete a this message from its mailbox.
	void deleteFromMailbox();
  
};

  
/* -------------------- Class: Session  -------------------- */
class Session {
private:
         mailimap* imap_session;
         Message** messages;
         uint32_t num_msgs;
         std::string mailbox;

  
  /* ----- fetchUID ----- */
  // Function to fetch the UID of a message, used in getMessages!
         uint32_t fetchUID(struct mailimap_msg_att* msg_att);
  

public:
  /* ----- CONTRUCTOR ----- */
        Session(std::function<void()> updateUI);
  
  /* ----- UPDATEUI ----- */
        std::function<void()> updateUI;
  
  /* ----- connect ----- */
  // Function to connect to specified server (143 is the standard unencrpyted imap port).
	void connect(std::string const& server, size_t port = 143);

  /* ----- login ----- */
  // Funcion to log in to server (connect first, then log in).
	void login(std::string const& userid, std::string const& password);

  /* ----- getMessages ----- */
  // Function to fetch all messages within session's mailbox, terminated by a nullptr (as in class).
        Message** getMessages();
  
  /* ----- selectMailbox ----- */
  // Function to select a mailbox (only one can be selected at any given time), can only be performed after login.
	void selectMailbox(std::string const& mb);
  
  /* ----- deleteAll ----- */
  // Function to delete all messages within mailbox.
        void deleteAll();

  /* ----- getMailbox ----- */
  // Funcion to return session mailbox.
       std::string getMailbox() const {return mailbox;}
  
  /* ----- getIMAP ----- */
  // Function to return session imap_session.
        mailimap* getIMAP() const {return imap_session;}

  /* ----- getNumMessages ----- */
  // Function to get the number of messages in the session mailbox.
        uint32_t getNumMessages(std::string mb);

  
  /* ----- DESCTRUCTOR ----- */
	~Session();
  
};
}

#endif /* IMAP_H */

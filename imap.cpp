#include "imap.hpp"

using namespace IMAP;
using namespace std;

/* ----------------- Session Functions ---------------- */
/* ----- CONSTRUCTOR ----- */
Session::Session(function<void()> updateUI) : updateUI(updateUI), mailbox("INBOX"), messages(nullptr), num_msgs(0) {
  imap_session = mailimap_new(0, nullptr);
}

/* ----- login ----- */
void Session::login(string const& userid, string const& password) {
  // Define error message and attempt to login:
  string login_err = "Login Error: Unable to log in ";
  login_err += userid; login_err += ".\n\nError code: ";
  check_error(mailimap_login(imap_session, userid.c_str(), password.c_str()), login_err);
}

/* ----- connect ----- */
void Session::connect(string const& server, size_t port) {
  // Define error message and attempt to connect:
  string connect_err = "Connection Error: Unable to connect to ";
  connect_err += server; connect_err += ".\n\nError code: ";
  check_error(mailimap_socket_connect(imap_session, server.c_str(), port), connect_err);
}

/* ----- selectMailbox ----- */
void Session::selectMailbox(string const& mb) {
  mailbox = mb;
  // Define error message and attempt to select mailbox:
  string mailbox_err = "Mailbox Error: Unable to select mailbox ";
  mailbox_err += mailbox; mailbox_err += ".\n\nError code: ";
  check_error(mailimap_select(imap_session, mailbox.c_str()), mailbox_err);
}

/* ----- DESTRUCTOR ----- */
Session::~Session(){
  // Delete messages:
  deleteAll();
  
  // Define logout error message and attempt to log out:
  string logout_err = "Logout Error: Unable to log out.\n\nError code: ";
  check_error(mailimap_logout(imap_session), logout_err);

  // Free imap_session:
  mailimap_free(imap_session);
}

/* ----- getMessages function ----- */
Message** Session::getMessages() {
  // Retrieve number of message using getNumMessages:
  uint32_t num_messages = getNumMessages(mailbox);
  // Check if mailbox is empty!
  if (num_messages == 0) {
    messages = new Message*[1];
    messages[0] = nullptr;
    return messages;
  }
  // Initialise Session attribute list of Messages of size num_messages+1 (add a nullptr to end):
  messages = new Message*[num_messages + 1];
  // Create a new set, fetch type, fetch attribute and result structure:: 
  auto set = mailimap_set_new_interval(1,0);//mailimap_set_item to retrieve all messages:  
  auto fetch_att = mailimap_fetch_att_new_uid();//mailimap_fetch_att
  auto fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();//empty mailimap_fetch_type
  clist* result;//result structure for mailimap_fetch function
  // Define mailimap_fetch_type_new_fetch_att_list_add error and attempt to add to fetch_type:
  string fetch_add_err = "Fetch Type Error: Unable to add fetch uid attribute to fetch type structure.\n\n Error code: ";
  check_error(mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att), fetch_add_err);

  // Define message retrieval error and attempt to retrieve messages:
  string get_msgs_err = "Message Retrieval Error: Unable to retrieve all messages from mailbox ";
  get_msgs_err += mailbox; get_msgs_err += ".\n\nError code: ";
  check_error(mailimap_fetch(imap_session, set, fetch_type, &result), get_msgs_err);

  // Iterate through result list structure and set messages:
  clistiter* cur;
  int count = 0;
  for(cur = clist_begin(result); cur != NULL; cur = clist_next(cur)) {
    auto msg_att = (mailimap_msg_att*)clist_content(cur);
    uint32_t uid = fetchUID(msg_att);
    if (uid) {
      messages[count] = new Message(this, uid);
      messages[count]->setMessage();
      count++;
    } 
  }
  messages[count] = nullptr;
  // Free associated data structures:
  mailimap_fetch_list_free(result);
  mailimap_fetch_type_free(fetch_type);
  mailimap_set_free(set);

  // Return messages
  return messages;
}

/* ----- fetchUID function ----- */
uint32_t Session::fetchUID(struct mailimap_msg_att* msg_att) {
  // Declare clistiter variable pointer cur:
  clistiter* cur;
  // Iterate over the attributes:
  for(cur = clist_begin(msg_att->att_list); cur != NULL; cur = clist_next(cur)) {
    auto item = (mailimap_msg_att_item*)clist_content(cur);
    // Check to see if att_type is not static, and  continue:
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {continue;}
    // Check to see if static data is not of att_type UID, and continue:
    if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_UID) {continue;}
    // Return UID of message
    return item->att_data.att_static->att_data.att_uid;
  }
  // Return 0 otherwise:
  return 0;
}

/* ----- getNumMessages function ----- */
uint32_t Session::getNumMessages(string mb = "INBOX") {
  // Declare sa_list variable and define as void:
  auto sa_list = mailimap_status_att_list_new_empty(); // mailimap_status_att_list*
  // Declare a result structure:
  mailimap_mailbox_data_status* result; // mailimap_mailbox_data_status*

  // Define fetch type add  error and attempt to add MAILIMAP_STATUS_ATT_MESSAGES attribute to sa_list,
  // i.e. we want the number of messages!
  string status_add_err = "Fetch Type Error: Unable to add MAILIMAP_STATUS_ATT_MESSAGES to status attribute structure while getting number of messages in mailbox ";
  status_add_err += mailbox; status_add_err += ".\n\n Error code: ";
  check_error(mailimap_status_att_list_add(sa_list, MAILIMAP_STATUS_ATT_MESSAGES), status_add_err);
  
  // Define mailbox status error:
  string mailbox_st_err = "Mailbox Status Error: Unable to retrieve number of message in  mailbox ";
  mailbox_st_err += mb; mailbox_st_err += ".\n\nError code: ";
  // Attempt to retrieve status of mailbox using sa_list and storing in result (passed by reference as input **):
  check_error(mailimap_status(imap_session, mailbox.c_str(), sa_list, &result), mailbox_st_err);

  // Declare and define info to store the mailimap_status_info-typecasted-first-element of result's info list: 
  auto info = (struct mailimap_status_info*)clist_content(clist_begin(result->st_info_list));
  // Declare and set number of messages num_messages from info structure:
  uint32_t  num_messages = info->st_value;
  
  // Free sa_list and result:
  mailimap_status_att_list_free(sa_list);
  mailimap_mailbox_data_status_free(result);

  // Return value:
  return num_messages;
}

/* ----- deleteAll ----- */
void Session::deleteAll() {
  for(int count = 0; count < num_msgs; count++) {
    delete messages[count];
  }
  delete [] messages;
}

/* ----------------- Message Functions ---------------- */
/* ----- setMessage ----- */
void Message::setMessage() {
  // Declare and initialise a new set variable with a single message:
  auto set =  mailimap_set_new_single(uid); // mailimap_set*
  // Declare and initialise a new fetch_env variable::
  auto env_att = mailimap_fetch_att_new_envelope(); // mailimap_fetch_att*
  // Define new section to retrieve message body:
  auto section = mailimap_section_new(NULL); // mailimap_section*
  auto body_att = mailimap_fetch_att_new_body_section(section); // mailimap_fetch_att*
  // Declare and initialise a new empty fetch_type:
  auto fetch_type = mailimap_fetch_type_new_fetch_att_list_empty(); // mailimap_fetch_type*
  // Declare and initialise a new clist item to store the results of mailimap_uid_fetch of type mailimap_msg_att:
  clist* result;
 
  // Define mailimap_fetch_type_new_fetch_att_list_add error and attempt to add to fetch_type:
  string fetch_add_env_err = "Fetch Type Error: Unable to add env_att to fetch type structure in setMessage.\n\nError code: ";
  check_error(mailimap_fetch_type_new_fetch_att_list_add(fetch_type, env_att), fetch_add_env_err);

  // Define mailimap_fetch_type_new_fetch_att_list_add error and attempt to add to fetch_type:
  string fetch_add_body_err = "Fetch Type Error: Unable to add body_att to fetch type structure in setMessage.\n\nError code: ";
  check_error(mailimap_fetch_type_new_fetch_att_list_add(fetch_type, body_att), fetch_add_body_err);

  // Define mailimap_uid_fetch error and attempt to fetch message attributes:
  string fetch_uid_err = "UID Fetch Error: Unable to fetch message attributes for message with UID ";
  fetch_uid_err += uid; fetch_uid_err += ".\n\nError code: ";
  check_error(mailimap_uid_fetch(session->getIMAP(), set, fetch_type, &result), fetch_uid_err);

  // Extract message attributes from result (should be a single message):
  auto msg_att = (mailimap_msg_att*)clist_content(clist_begin(result));

  // Use setMessageFields to set all the message attributes:
  setMessageFields(msg_att);
  
  // Free set, env_att, body_att, section, fetch_type and result:
  mailimap_set_free(set);
  //mailimap_fetch_att_free(env_att);
  //mailimap_fetch_att_free(body_att);
  //mailimap_section_free(section);
  mailimap_fetch_type_free(fetch_type);
  mailimap_fetch_list_free(result);
}

/* ------ setMessageFields ----- */
void Message::setMessageFields(mailimap_msg_att* msg_att) {
  // Declare clist pointer:
  clistiter* cur;
  // For loop to run through mailimap_msg_att content to determine the appropriate fields to assign:
  for(cur = clist_begin(msg_att->att_list); cur != NULL; cur = clist_next(cur)) {
    auto item = (mailimap_msg_att_item*)clist_content(cur);
    // Check if att_type is not static, cotinue:
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {continue;}
    
    // Check if att_data.att_static's att_type is MAILIMAP_MSG_ATT_ENVELOPE i.e. subject or sender:
    if (item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_ENVELOPE) {
      // Check if subject, if so so then assign to subject:
      if (item->att_data.att_static->att_data.att_env->env_subject) {
	subject = item->att_data.att_static->att_data.att_env->env_subject;
      }
      // Check if sender, if so use setFrom function to assign to from:
      if (item->att_data.att_static->att_data.att_env->env_from->frm_list) {
	setFrom(item->att_data.att_static->att_data.att_env->env_from->frm_list);
      }
    }
    
    // Check if att_type.att_static's att_type is MAILIMAP_MSG_ATT_BODY_SECTION i.e. body:
    if (item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_BODY_SECTION) {
      // Check if there is a body part, if so then assign to body:
      if (item->att_data.att_static->att_data.att_body_section->sec_body_part) {
	body = item->att_data.att_static->att_data.att_body_section->sec_body_part;
      }
    }
  }
}

/* ----- setFrom ----- */
void Message::setFrom(clist* frm_list) {
  // Declare variable to traverse list of (possibly multiple) senders:
  clistiter* cur;
  // For loop to run through mailimap_address content and 
  for (cur = clist_begin(frm_list); cur != NULL; cur = clist_next(cur)) {
    auto address = (mailimap_address*)clist_content(cur);
    // If there is a personal name, add it to from:
    if (address->ad_personal_name) {
      from += address->ad_personal_name;from +=  ", ";
    }
    // If there is a mailbox and a host name, add them to from:
    if (address->ad_mailbox_name && address->ad_host_name) {
      from += "<"; from += address->ad_mailbox_name; from += "@"; from += address->ad_host_name; from += ">; ";
    }
  }
}

/* ----- deleteFromMailbox ----- */
void Message::deleteFromMailbox() {
  // Declare and initialise an empty flag_list:
  auto flag_list = mailimap_flag_list_new_empty(); // mailimap_flag_list*
  auto del_flag = mailimap_flag_new_deleted(); // mailimap_flag*
  auto set = mailimap_set_new_single(uid); // mailimap_set*

  // Define flag add error and attempt to add del_flag to flag list:
  string del_flag_add_err = "Flag List Errror: Unable to add 'delete' flag to current message with UID: ";
  del_flag_add_err += uid; del_flag_add_err += ".\n\nError code: ";
  check_error(mailimap_flag_list_add(flag_list, del_flag), del_flag_add_err);
  // Declare and define variable to store flaglist:
  auto store = mailimap_store_att_flags_new_set_flags(flag_list); // mailimap_store_att_flags*

  // Define store error and attempt to store flaglist for set of messages:
  string store_err = "Store Error: Unable to store flag list containing 'delete' flag for set containing message with UID: ";
  store_err += uid; store_err += ".\n\nError code: ";
  check_error(mailimap_uid_store(session->getIMAP(), set, store), store_err);
  // Define expunge error and attempt to expunge:
  string  exp_err = "Expunge Error: Unable to expunge message with UID ";
  exp_err += uid; exp_err += " mailbox "; exp_err += session->getMailbox(); exp_err += ".\n\nError code: ";
  check_error(mailimap_expunge(session->getIMAP()), exp_err);

  // Update UI:
  session->updateUI();

  // Delete flaglist, del_flag, set, and store:
  mailimap_set_free(set);
  //mailimap_flag_free(del_flag);
  //mailimap_flag_list_free(flag_list);
  mailimap_store_att_flags_free(store);

  delete this;
}

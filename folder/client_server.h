//USECASE: Structure files for simple broadcast chat server

#ifndef client_server
#define client_server

struct sb_chat_server_hdr{

	unsigned int version:9; //9 bits as defined in the mannual. 
	unsigned int type:   7; //7 bits as defined. 
	int length;
};

struct sb_chat_server_att{

	int type;
	int length;
	char PayloadData [512];
	
};

struct sb_chat_server_MSG{

	struct sb_chat_server_hdr hdr;
	struct sb_chat_server_att attr[2];// to identify the two different msgs. i.e. the actual payload message and the username.
};


struct sb_chat_server_usr_info{

	char username[16];//as definedin the manual 
	int f_d; 
	int ClientCount;
};



#endif 
int ERROR_MSG(const char* string) //display and exit 
{
	error(string);
	exit(1);
}

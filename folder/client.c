//ABOUT:  Code detailing the client functionality in Simple Broadcast Chat Server (SBCS)
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>


#include "client_server.h"
time_t current_time;

//connection is made, time to join the chat room

void start_chatting (int soc_fd, char * argv[]){

	struct sb_chat_server_hdr hdr;
	struct sb_chat_server_att attr;
	struct sb_chat_server_MSG msg;
	
	int status = 0;

	hdr.version = '3'; // protocol version as defined in the mannual
	hdr.type    = '2'; //join request hdr

	attr.type = 2;//sending the username 
	attr.length = strlen(argv[3]) + 1; //length of username + null char
	strcpy(attr.PayloadData, argv[3]); // copy the username 
	
	msg.hdr = hdr; //encapsulate
	msg.attr[0] = attr;// just one attr for joining.

	write(soc_fd,(void *)&msg, sizeof(msg));

	status = read_msg(soc_fd);

	if (status==1)
		{ERROR_MSG("username already present.");
		 close(soc_fd);}

}	

//read server msg

int read_msg(int soc_fd){

	struct sb_chat_server_MSG server_msg;
	int i;
	int status_Value = 0;
	int numb_bytes = 0;

// reading the bytes from the server. 
	numb_bytes = read(soc_fd,(struct sb_chat_server_MSG*)& server_msg, sizeof(server_msg));

	int total_count = 0;//will be used to check the size of the payload received. 
	
	// forward_msg

	//check the username, compare the actual length with the length specified in the hdr, check the hdr type with the attr index. If all are correct print the msg. 

	if (server_msg.hdr.type==3){// forward msg
	
		if((server_msg.attr[0].PayloadData!=NULL||server_msg.attr[0].PayloadData!='\0') &&(server_msg.attr[1].PayloadData!=NULL||server_msg.attr[1].PayloadData!='\0') &&(server_msg.attr[0].type==4)
		 
                &&(server_msg.attr[1].type==2)){
		//checking the size of the payload matches the payload length specified. 
			for(i=0; i<sizeof(server_msg.attr[0].PayloadData);i++){
				if (server_msg.attr[0].PayloadData[i]=='\0'){//end of string found 
					total_count = i-1;
					break;
				}
			}//end for 

			if(total_count==server_msg.attr[0].length){
				
				printf("USER by the username  %s has sent %s",server_msg.attr[1].PayloadData,
					server_msg.attr[0].PayloadData);
				current_time = time (NULL);
			
				printf("CLIENT:: current time is :%s \n",asctime(localtime(&current_time)));

			}

			else ERROR_MSG("Incorrect length of message at client \n");
		}//mismatch of the any data such as payload data type or username data type 
		else ERROR_MSG("CLIENT: hdr type mismatch or null value has been recevied\n");

		status_Value = 0; //sucess/
	}//if (server_msg.hdr.type)


	//if the server sends a NACK
	if(server_msg.hdr.type ==5){
		if((server_msg.attr[0].PayloadData!=NULL||server_msg.attr[0].PayloadData!='\0')
		 &&(server_msg.attr[0].type==1)){// indicating the reason of failure
			printf("CLIENT:: Failed  to join as of now, reason %s",server_msg.attr[0].PayloadData);
		}
		status_Value = 1;		
	}// if (server_msg.hdr.type)

//offline msg received from the server. 
	if (server_msg.hdr.type==6){
		
		if ((server_msg.attr[0].PayloadData!=NULL||server_msg.attr[0].PayloadData!='\0')
		   &&server_msg.attr[0].type ==2){ //i.e.sending the username that left the chat 
		
			printf(" USERNAME:: %s has left the chat room now\n",server_msg.attr[0].PayloadData);
		}
	status_Value =0;//successfully read. 
	}

//ACK with the client count 
	if (server_msg.hdr.type==7){
		if ((server_msg.attr[0].PayloadData!=NULL||server_msg.attr[0].PayloadData!='\0')&&server_msg.attr[0].type ==4){
		   printf("TOTAL  number of clients and the ACK msg is %s\n",server_msg.attr[0].PayloadData);
		}
	status_Value =0;
	}

//new chat participant has arrived. 
	if (server_msg.hdr.type ==8){
		if ((server_msg.attr[0].PayloadData!=NULL||server_msg.attr[0].PayloadData!='\0')
		   &&server_msg.attr[0].type ==2){ //i.e.sending the username that joinded
			printf("New user Name::  %s has joined the chatroom \n",server_msg.attr[0].PayloadData);
		}

	status_Value = 0; 
	}


	return status_Value;
}



void sending(int soc_fd)
{
	struct sb_chat_server_hdr hdr;
	hdr.version = '3';
	hdr.type    = '4';//as defined in the manual.

	struct sb_chat_server_MSG msg;
	struct sb_chat_server_att attr;


	msg.hdr = hdr;// copying the hdr to the msg hdr.

	int num_read = 0;
	char temp_var[512];
	struct timeval waiting_time;
	fd_set read_f_d;

	waiting_time.tv_sec = 2;
	waiting_time.tv_usec = 0;

	FD_ZERO(&read_f_d); //clearing the read descriptor
	FD_SET(STDIN_FILENO, &read_f_d);// set to read from the input 

	select(STDIN_FILENO+1, &read_f_d, NULL, NULL, &waiting_time);

	if(FD_ISSET(STDIN_FILENO, &read_f_d)){
		num_read = read(STDIN_FILENO,temp_var, sizeof(temp_var));

		if (num_read >0)
			temp_var[num_read] = '\0';
	
	
	attr.type = 4;//msg as specified in the mannual 
	strcpy(attr.PayloadData, temp_var);// copy the msg to payload
	msg.attr[0] = attr;
	msg.attr[0].length = num_read -1 ; //excluding the extra read char
	write(soc_fd, (void *)&msg, sizeof(msg));

	}
	else {

		printf("CLIENT:Timeout has happened from the client side \n");
	}
}



 

int main (int argc, char*argv[]){

	//int idle_time_count = 0;
	struct timeval idle_time_count;
	int selected_value_returned =0;
	if (argc!=4)
	{
		printf("CLIENT:USAGE:./client <IP_address> <port_num> <user_name> \n");
		ERROR_MSG("Please specify in the correct format as described");
	}

	int soc_fd = socket(AF_INET, SOCK_STREAM, 0); // Wwould work for both IPv4 AND IPv6
	char * p; // used to point to an array if not converted,Same as MP1
	//server add. 
	int port_num = strtol(argv[2],&p,10);
	struct hostent* IP =  gethostbyname(argv[1]);//IP address
	struct sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address)); // same as MP 1
	server_address.sin_family = AF_INET; //IPv4
	server_address.sin_port   = htons(port_num); // port number as MP1
	//memcpy(&server_address.sin_addr.s_addr, IP->h_addr, IP->h_length);


	//adding file descriptor to the select. 
	fd_set main_f_d;
	fd_set read_f_d;
	
	//clearing them and setting to zero. 
	FD_ZERO(&read_f_d);
	FD_ZERO(&main_f_d); 

	//connec to the server/or we can say the chatroom. 
	int connect_status = connect(soc_fd, (struct sockaddr *)&server_address, sizeof(server_address));
printf("hi exit if %d \n",connect_status );
	if (connect_status < 0)//error
		ERROR_MSG("Error in now connecting to the main server");
	
	printf("Connection Successfull ");

	start_chatting(soc_fd, argv);

	FD_SET(soc_fd, &main_f_d);// to see any input on the socket line 
	FD_SET(STDIN_FILENO, &main_f_d);// to see any input on the command line 


	while (1){

	read_f_d = main_f_d;

	idle_time_count.tv_sec =10;// waitin for 10 secs or the readfiledescriptor to wakeup 

	if ((selected_value_returned = select(soc_fd+1, &read_f_d,NULL,NULL,&idle_time_count))==-1)
		ERROR_MSG("CLIENT:SELECT issue happened");

	if (FD_ISSET(soc_fd,&read_f_d))//read from the socket
		read_msg(soc_fd);

	if (FD_ISSET(STDIN_FILENO, &read_f_d))//read from commadn line send to the server
		sending(soc_fd);
	}

	

	printf("User has now left the chatroom and the chat has eneded\n");
	printf("Closing the client now");
	return 0;

}

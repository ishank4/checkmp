// FIle for server side for Machine Problem 2


#include <string.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "client_server.h"

#define max_string_size 256
// SBC server data structures 
int num_clients = 0;
// client info
struct sb_chat_server_usr_info *clients;

//Acknowledgement  Function: signal sending ack to client 
void ACK_send (int fd_client){
	struct sb_chat_server_MSG ack_msg;
	char str_temp [256];

	//ACK hdr format
	ack_msg.hdr.version = 3; //hdr type FWD
	ack_msg.hdr.type = 7;
	ack_msg.attr[0].type = 4;

	snprintf(str_temp,5,"%d%s",num_clients," ");
	int k = strlen(str_temp);
	int i;
	for (i=0; i < num_clients-1; i++)
	{
		strcat(str_temp,",");
		strcat(str_temp,clients[i].username);
	}
	ack_msg.attr[0].length = strlen(str_temp) + 1;
	strcpy (ack_msg.attr[0].PayloadData, str_temp);
	
	write (fd_client,(void *)&ack_msg, sizeof(ack_msg));
}
// NACK send function implementation
void NACK_send(int fd_client)
{
	struct sb_chat_server_MSG nack_msg;
	char str_temp[256];

	nack_msg.hdr.version = 3;
	nack_msg.hdr.type = 5;
	nack_msg.attr[0].type = 1;
// strcat to concatenate the string
	strcat(str_temp,"USE differnet username , this username is taken already");
	nack_msg.attr[0].length = strlen(str_temp);
	strcpy(nack_msg.attr[0].PayloadData, str_temp);

	write(fd_client,(void *) &nack_msg,sizeof(nack_msg));

	close(fd_client);
}
// This function checks if the name is already been taken by someone , if yes then it returns suggesting TAKEN
int usrname_existence (char username[]){
int i;	
	for (i =0 ; i < num_clients; i++)
	{
		if(strcmp (username, clients[i].username) == 0)
		{
			return 1; //name is present
		}
	}
	return 0; //does not exist
}

//to check if any client has joined yet or waiting
int is_client_joined(int fd_client){
	struct sb_chat_server_MSG join_msg;
	struct sb_chat_server_att join_msg_attr;
	char str_temp [16];  // temp string with total n bits
	read(fd_client,(struct sb_chat_server_MSG *)&join_msg,sizeof(join_msg));
	join_msg_attr = join_msg.attr[0];
	strcpy(str_temp,join_msg_attr.PayloadData);

	if(usrname_existence(str_temp)){
		printf("%s\n","This username is already been taken !" );
		NACK_send(fd_client);
		return 1;
	}
	else{  //string comparison of the username
		strcpy(clients[num_clients].username, str_temp);
        printf("num_clients = %d\n",num_clients);
		clients[num_clients].f_d = fd_client;
		clients[num_clients].ClientCount = num_clients;
		num_clients ++;
		ACK_send(fd_client);

	}
	return 0;
}

// this is the main function call of the server 
int main(int argc, char*argv[])
{
	struct sb_chat_server_MSG receive_msg, forward_msg, join_broadcast_msg, leave_broadcast_msg;
	struct sb_chat_server_att client_attr;

	//address information stored for the server
	struct sockaddr_in servers_add, *client_add;
    servers_add.sin_family = AF_INET;
    servers_add.sin_port = htons(atoi(argv[1]));
    socklen_t server_addr_size = sizeof(servers_add);
   //clients for max number
    int maximum_num_clients = atoi(argv[2]);
    printf("Maximum Clients number%d\n", maximum_num_clients);

    int serv_status;
    struct addrinfo address_hints; //addd hint values
    struct addrinfo *ser_information; 
    memset(&address_hints,0,sizeof address_hints); //null structure
 
    address_hints.ai_family = AF_UNSPEC; //IPv4 vs IPv6
    address_hints.ai_socktype = SOCK_STREAM; //Info for TCP sockets 


    fd_set file_descrip_master; 
    fd_set temp_fd; //temporary decriptor 
    FD_ZERO (&file_descrip_master); //reconfiguring the entries in file descriptors
    FD_ZERO (&temp_fd);  // for the temporary descriptor

    int latest_client = 0; //new accepted socket count
    struct sockaddr_in address_client; //Address of the client
    address_client.sin_family = AF_INET;
    /* Server socket is not binded to any particular specific IP.
    BIND is independent of server IP*/
    address_client.sin_addr.s_addr = htons(INADDR_ANY);  //ANY addr sin client
    address_client.sin_port = htons (atoi(argv[1]));
    int total_BYTES = 0; //total bytes received on the socket side
    //Initialization of the socket
    int sockket_server_num = socket(AF_INET, SOCK_STREAM,0);   //server socekett number stored
    printf("Socket server value %d\n",sockket_server_num );
    if(sockket_server_num < 0){
    	printf("%s\n","Failed to connect between client & server" );
    	ERROR_MSG("Establishishing the connection");
    	exit (0);
    }

    int temporary_val = 1;
    if (setsockopt(sockket_server_num,SOL_SOCKET,SO_REUSEADDR,&temporary_val,sizeof(int))<0){
    	printf("%s\n","Failed :: setsockopt(SO_REUSEADDR)" );
    }
// for establishing the server socket connection
    printf("Server Socket connection being established");

    //binding  function call to the socket server side
    if (bind(sockket_server_num, (struct sockaddr *) &address_client ,sizeof(address_client)) < 0)
    {
    	printf("Failed in socket binding");
    	ERROR_MSG("BINDING NOW");
    	exit(0);
    }
// success when the socket is binded 
    printf("%s\n","Successfull: Socket Binded\n" );
//cleints structure and allocating memeory using malloc
    clients = (struct sb_chat_server_usr_info *)malloc(maximum_num_clients*sizeof(struct sb_chat_server_usr_info));
// store the added client structure 
    client_add = (struct sockaddr_in *)malloc(maximum_num_clients*sizeof(struct sockaddr_in));

    //listening phase
    if(listen (sockket_server_num,10) < 0) {
    	printf("Fail:: To the client found\n");
    	ERROR_MSG("LISTENING");
    	exit(0);
    }
    printf("%s\n","Waiting or Listening to the client now !" );

    FD_SET(sockket_server_num, &file_descrip_master); 
    // counting the file _descrip
    int fd_max = sockket_server_num; 
    printf(" Value of Max FD= %d\n",fd_max);
    int temp;

    while(1){
    	temp_fd = file_descrip_master;
    	if (select(fd_max + 1,&temp_fd, NULL, NULL,NULL) == -1){
    		printf("%s\n","Error has occured when selecting \n" );
    		ERROR_MSG("SELECT::");
    		exit(0);
    	}
    	int i;
    	for ( i=0; i<=fd_max; i++){ 
    		if(FD_ISSET(i,&temp_fd)){ 
    			if(i == sockket_server_num){ // matches the server socket take the action if any client wants to connect
    		//client added to the sized array client address
    				socklen_t sz_client_addr = sizeof(client_add[num_clients]);
    				latest_client = accept(sockket_server_num, (struct sockaddr *)&client_add[num_clients], &sz_client_addr );
    				if(latest_client == -1){
    					printf("ERROR :: When accepting new client \n Error Number%d\n",(int)errno );
    				}else{
    					temp = fd_max;
    					FD_SET(latest_client, &file_descrip_master); //added the new connection to the client list

    					//file descriptors being updated
    					if(latest_client > fd_max){
    						fd_max = latest_client;
    					}
    					if(num_clients + 1 > maximum_num_clients){  // to check  if it exceeds the client count value
    						printf("ERROR:: Exceeded the number of connected users\n Connection has been denied \n");
    						fd_max = temp;
    						FD_CLR(latest_client, & file_descrip_master);
    						NACK_send(latest_client);
    					}else{
    						if(is_client_joined(latest_client) == 0){
    							//client has become online
    							printf("USER : %s has connected and joined CHAT ROOM\n", clients[num_clients-1].username);
    							join_broadcast_msg.hdr.version = 3;
    							join_broadcast_msg.hdr.type = 8;
    							join_broadcast_msg.attr[0].type = 2;
    							strcpy(join_broadcast_msg.attr[0].PayloadData, clients[num_clients-1].username)	;
    							int j;
    							for(j=0; j<=fd_max;j++){ //broadcast the message except to that particular client
    								if(FD_ISSET(j, &file_descrip_master))
    								{
    									if(j != sockket_server_num && j != latest_client){
    										if((write(j,(void *)&join_broadcast_msg,sizeof(join_broadcast_msg))) == -1){
    											ERROR_MSG("Error when doing the broadcasting JOIN of the msg");
    										}
    									}
    								}

    							}
    						} else{
    							fd_max = temp;
    							FD_CLR(latest_client, &file_descrip_master); //if the name not available  
    						}
    					}
    				}  // end of previous condition
    			}else{
    					total_BYTES = read(i, (struct sb_chat_server_MSG *)&receive_msg,sizeof(receive_msg));
    					if(total_BYTES <= 0){  // if the bytes sum is less than zero
    						if(total_BYTES == 0){
    							int k;
    							for(k=0; k < num_clients; k++){
    								if(clients[k].f_d == i){
    									leave_broadcast_msg.attr[0].type = 2;
    									strcpy(leave_broadcast_msg.attr[0].PayloadData,clients[k].username);
    								}
    							} // if the user has left
    							printf(" User by the name %s has vacated the room\n", leave_broadcast_msg.attr[0].PayloadData );
    							leave_broadcast_msg.hdr.version = 3;
    							leave_broadcast_msg.hdr.type = 6;
    							int j;
    							for (j = 0; j <=fd_max; j++){
    								if(FD_ISSET(j,&file_descrip_master)){
    									if(j!=sockket_server_num && j!=latest_client){
    										if((write(j,(void*)&leave_broadcast_msg,sizeof(leave_broadcast_msg))) == -1){
    											ERROR_MSG("LEAVE MESSAGE BROADCASTED");
    										}
    									}
    								}
    							}
    					}else if(total_BYTES < 0){
                            printf("RECEIVING THE MESSAGE, WAITING\n");
    					}
    					close(i);
    					FD_CLR(i, &file_descrip_master); //client structure updated if the client is removed 
    					int x;
    					for(x=i; x<num_clients; x++){
    						clients[x] = clients[x+1];

    					}
    					num_clients--; //decrease the total client number
    				}else{
    					client_attr = receive_msg.attr[0]; //get msg
    					forward_msg = receive_msg;
    					forward_msg.hdr.type = 3;
    					forward_msg.attr[1].type =2;
    					forward_msg.attr[0].length = receive_msg.attr[0].length;
    					char name[16];
    					strcpy(name,receive_msg.attr[1].PayloadData);

    					int k;
    					for(k=0; k<num_clients;k++){
    						if(clients[k].f_d == i){
    							strcpy(forward_msg.attr[1].PayloadData,clients[k].username);
    						}
    					}
    					printf("%s says %s\n",forward_msg.attr[1].PayloadData, forward_msg.attr[0].PayloadData );

    					// MessageForward all of the clients but to  the current client and server
    					int j;
    					for (j=0; j<=fd_max; j++){
    						//send forward msg
    						if(FD_ISSET(j , &file_descrip_master)){
    							if(j!=sockket_server_num && j!=i){
    								if((write(j, (void*)&forward_msg,total_BYTES))==-1){
    									ERROR_MSG("Forwarding the msg");
    								}
    							}
    						}
    					}
    				}// frwd msg
    		}//client condition over
    	}else{
            printf("Connect...\n");}//end new connection
    }//end loop on the file descriptors
} //while loop ends here
//close the socket server now

    close(sockket_server_num);
    return 0;
}

#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<string.h>
/*

	UNDER CONSTRUCTION

	

*/

void start_server(char serverIPAddress[],int port){
	printf("Starting the server\n");
	/*this is going to hold the file descriptor of server socket */
	int server_file_descriptor;
	/*this other file descriptor is for handling client requests*/
	int other_file_descriptor;
	/*server_addr is going to hold server information which will be binded with the socket later.
	client_addr will contain the IP address and port number of clients	*/
	struct sockaddr_in server_addr,client_addr;
	

	char buffer[8192];//temporary variable. i will remove this later.
	
	/*create the server socket*/	
	server_file_descriptor=socket(AF_INET,SOCK_STREAM,0);
	if(server_file_descriptor<0){
		fprintf(stderr,"[ERROR] Cannot open server socket\n");
		exit(1);
	}
	
	/*now let's configure the server address*/
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(serverIPAddress);		/*32-bit IP address in Network Byte Order (Big endian order)*/
	server_addr.sin_port=htons(port);	/*16-bit port number in Network Byte Order (Big endian order)*/

	/*bind the socket to the address configuration*/
	int checkBind=bind(server_file_descriptor,(struct sockaddr*) &server_addr,sizeof(server_addr));
	if(checkBind<0){
		fprintf(stderr,"[ERROR] Cannot bind given address configuration to the socket. Check if the given port is free.\n");
		exit(1);
	}
	
	/*listen to a maximum of 3 connection requests from clients*/
	int checkListen=listen(server_file_descriptor,3);
	if(checkListen<0){
		fprintf(stderr,"[ERROR] Error in listening to clients.\n");
		exit(1);
	}
	printf("Server is ready and waiting for connections...\n");
	/*create another file descriptor for interacting with the client which is connected (after accepting connection)*/
	int clientAddr_len=sizeof(client_addr);
	other_file_descriptor=accept(server_file_descriptor,(struct sockaddr*) &client_addr,(socklen_t*)&clientAddr_len);
	
	read(other_file_descriptor,buffer,sizeof(buffer));
	
	printf("\n%s\n",buffer);	
	
	char content[]="<html><body><h1>Hello, World!</h1></body></html>";
	int len=strlen(content);
	
	
	char response[1000]; //make response size dynamic

	sprintf(response,"HTTP/1.1 200 OK\r\nServer: SimpleWebServer 1.0 (Debian)\r\nContent-Length: %d\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n%s",len,content);

	write(other_file_descriptor,response,sizeof(response));
	printf("Response is:\n%s",response);
	printf("Response sent successfully!\n");
}

int main(int argc,char** argv){
	start_server("127.0.0.1",atoi(argv[1]));
	return 0;
}

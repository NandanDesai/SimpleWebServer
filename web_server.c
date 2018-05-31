/*

	Author: Nandan Desai
	Date: 31st May 2018, 12:23 PM (IST)
	Github repo: https://github.com/NandanDesai/SimpleWebServer
	
	About this project: read README.md file
*/


#include <stdio.h>
#include <stdlib.h> /*for exit(1)*/
#include <unistd.h> /*for read() and write() functions*/
#include <arpa/inet.h> /*for all socket-related functions*/
#include <string.h>
#include <sys/stat.h> /*for stat(), struct stat and mkdir() functions*/
/*

	This program only implements HTTP GET and HEAD methods because that is what a minimal web server is required to implement.
	
*/
#define MAX_HTTP_RES_SIZE 20000	//~20KB
#define MAX_HTTP_REQ_SIZE 8000	//~8KB

/*This is a structure that holds the HTTP method and URL mentioned the incoming HTTP request*/
struct http_request{
	char method[10];
	char url[2000];
	/*We are not interested in other fields yet.*/
};
typedef struct http_request http_request;

/*This is a structure that holds the payload content, status message (like "OK" or "Not Found") and status code (like 200 or 404)*/
struct http_response{
	char* content;
	char* status_msg;
	int status_code;
};
typedef struct http_response http_response;


/*This function parses the HTTP request and returns the HTTP method and URL mentioned in the request.*/
http_request* parse_http_request(char* request){
	http_request* hreq=(http_request*)malloc(sizeof(http_request));
	char delim[]=" \r\n";
	char *token=strtok(request,delim);
	int i=0;
	/*Here, we are not validating the HTTP request format. Also, we are just interested in HTTP request method (GET, POST, DELETE etc.) and the URL*/
	while(strcmp(token,"HTTP/1.1")!=0 && token!=NULL){
		if(i==0){
			strcat(token,"\0");
			strcpy(hreq->method,token);
			token=strtok(NULL,delim);
			i++;
		}else{
			strcat(token,"\0");
			strcpy(hreq->url,token);
			token=strtok(NULL,delim);
			i++;	
		}		
	}
	return hreq;
}

/*This function determines the file being requested and the HTTP method mentioned. It returns the content from the file being requested*/
http_response* get_response(http_request* hreq){
	http_response* response=(http_response*)malloc(sizeof(http_response));
	char buffer[1000];
	char path[]="./www";
	if(strcmp(hreq->method,"GET")==0 || strcmp(hreq->method,"HEAD")==0){
		if(strcmp(hreq->url,"/")==0){
			strcat(path,"/index.html");
		}else{
			strcat(path,hreq->url);
		}
		
		FILE *fp=fopen(path,"r");
		if(fp==NULL){
			response->content=strdup("<html><h2>404 Not Found</h2></html>");
			response->status_msg=strdup("Not Found");
			response->status_code=404;
			return response;
		}
		response->content=(char*)malloc(MAX_HTTP_RES_SIZE);
		while(fgets(buffer,sizeof(buffer),fp)){
			strcat(response->content,buffer);
		}
		response->status_msg=strdup("OK");
		response->status_code=200;
	}else{
		response->content=strdup("<html><h2>501 Method Not Implemented</h2></html>");
		response->status_msg=strdup("Not Implemented");
		response->status_code=501;
	}
	return response;
}


/*This function creates 'www' directory if it does't exists*/
void create_www_directory(){
	struct stat s;

	/*If 'www' directory exists, then struct stat s will be filled with all the metadata related to that directory.
	  If this directory doesn't exists, then stat() will return -1. That's when we create the directory using mkdir().
	*/
	if(stat("./www",&s)==-1){
		mkdir("./www",0777);
		fprintf(stderr,"*\t[WARNING] 'www' directory was not created previously. This program created it for you!\n\tYou may want to place your HTML files that are to be served for the clients in that directory. 'www' currently contains index.html only.\n");

		FILE *fp=fopen("./www/index.html","w");
		fprintf(fp,"<html><head><title>SimpleWebServer</title></head><h2>It works!</h2><p>This program was created by <a href=\"https://github.com/NandanDesai\" target=\"_blank\">Nandan Desai</a>. Feel free to modify the code!</p></html>");
		fclose(fp);
	}
}



void start_server(int port){
	create_www_directory();

	printf("*\tStarting the server\n");
	/*this is going to hold the file descriptor of server socket */
	int server_file_descriptor;
	/*this other file descriptor is for handling client requests*/
	int other_file_descriptor;
	/*server_addr is going to hold server information which will be binded with the socket later.
	client_addr will contain the IP address and port number of clients	*/
	struct sockaddr_in server_addr,client_addr;
	
	/*
		This variable will contain the HTTP requests sent by the clients
	*/
	char buffer[MAX_HTTP_REQ_SIZE]; 
	
	/*create the server socket*/	
	server_file_descriptor=socket(AF_INET,SOCK_STREAM,0);
	if(server_file_descriptor<0){
		fprintf(stderr,"*\t[ERROR] Cannot open server socket\n");
		exit(1);
	}
	
	/*now let's configure the server address*/
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;		/*32-bit IP address in Network Byte Order (Big endian order)*/
	server_addr.sin_port=htons(port);	/*16-bit port number in Network Byte Order (Big endian order)*/
	/*htons() stands for "Host to Network Short" which is used to convert Host Byte Order to Network Byte Order*/

	/*bind the socket to the address configuration*/
	int checkBind=bind(server_file_descriptor,(struct sockaddr*) &server_addr,sizeof(server_addr));
	if(checkBind<0){
		fprintf(stderr,"[ERROR] Cannot bind given address configuration to the socket. Check if the given port is free.\n");
		exit(1);
	}
	
	/*listen to a maximum of 3 connection requests from clients*/
	int checkListen=listen(server_file_descriptor,3);
	if(checkListen<0){
		fprintf(stderr,"*\t[ERROR] Error in listening to clients.\n");
		exit(1);
	}
	printf("*\tServer is ready and waiting for connections...\n");
	printf("*\tTo test the working, open the following link in your browser: http://localhost:%d\n",port);
	while(1){
		/*create another file descriptor for interacting with the client which is connected (after accepting connection)*/
		int clientAddr_len=sizeof(client_addr);
		other_file_descriptor=accept(server_file_descriptor,(struct sockaddr*) &client_addr,(socklen_t*)&clientAddr_len);
		/*
			We want to server many clients, so create a new process for serving the current client.
		*/
		int pid=fork();
		if(pid<0){
			fprintf(stderr,"*\t[Error] Couldn't execute fork()\n");
		}else if(pid==0){
			/*	
				We are in child process here.
				server_file_descriptor is of no use in client. So, close it.
			*/
			close(server_file_descriptor);
			
			/*
				Read the data sent by the client into 'buffer'.
			*/
			read(other_file_descriptor,buffer,sizeof(buffer));
			printf("\n========================================================================");
			printf("\n*\tRequest is:");
			printf("\n%s\n",buffer);	
	
			/*
				Parse the HTTP request to know the method and the content that is being requested.
			*/
			http_request* hreq=parse_http_request(buffer);
			
			/*
				Now, get HTTP response for the given request.
			*/
			http_response* hres=get_response(hreq);
			
			
			char* content=hres->content;
			int content_len=strlen(content);
			strcat(content,"\0");
			char* status_msg=hres->status_msg;
			int status_code=hres->status_code;
			
			
			char http_response[MAX_HTTP_RES_SIZE];
			/*
				char* http_response=(char*)malloc(MAX_HTTP_RES_SIZE); //this won't work, why?
			*/
			
			/*
				If the HTTP request has HEAD method, then reply with the same HTTP headers that you would for a GET request,
				but just don't send the contents.
			*/
			if(strcmp(hreq->method,"HEAD")==0){
				sprintf(http_response,"HTTP/1.1 %d %s\r\nServer: SimpleWebServer 1.0 (Debian)\r\nContent-Length: %d\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8",status_code,status_msg,content_len);
			}else{
				sprintf(http_response,"HTTP/1.1 %d %s\r\nServer: SimpleWebServer 1.0 (Debian)\r\nContent-Length: %d\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n%s",status_code,status_msg,content_len,content);
			}
			write(other_file_descriptor,http_response,sizeof(http_response));
			printf("*\tResponse is:\n%s\n",http_response);
			printf("========================================================================\n\n");
			
			/*
				exit from the child process when the webpage is served.
			*/
			exit(0);
		}else{
			/*
				We are in parent process now.
				close the other_file_descriptor created for serving the client previously as the child process is already doing it's job by now.
				We'll use the other_file_descriptor for the next client.
			*/
			close(other_file_descriptor);
		}
		
	}
}

int main(int argc,char** argv){
	if(argc==2){
		start_server(atoi(argv[1]));
		return 0;
	}else{
		fprintf(stderr,"*\t[Error] expected port number as an argument\n");
		return -1;
	}
}

/* Server code in C++11 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <time.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <utility>
#include <fcntl.h>
#include <map>

#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <utility>
#include <functional>
#include <stdint.h>
#include <fstream>
  
using namespace std;

#define PROTOCOL_HEADER_SIZE_BITS 4

struct Peer
{ 
	string IP;
	int port;
	bool state;
	bool operator == (const Peer& otro_peer)
	{
		if(IP == otro_peer.IP and port == otro_peer.port)
			return true;
		return false;
	}
};


vector <Peer> tracker;
map<string, vector<pair<int, string> > > archivos;
bool var_alive = false;

string prepare_response(char type, string message)
{
	string response;
	int message_size = message.size();

	if(type == 'R')
		message_size -= 32;

	response = to_string(message_size) + string(1,type) + message;

	response.insert(response.begin(), PROTOCOL_HEADER_SIZE_BITS - to_string(message_size).size(), '0');

	return response;
}

string register_user(Peer user)
{
	auto iter=find(tracker.begin(), tracker.end(), user);
	if(iter != tracker.end())
	{
		if((*iter).state == true)
			return prepare_response('E', "Could not register. You are already registered.");
		else
		{
			(*iter).state = true;
			return prepare_response('O', "Welcome Again");
		}
	}
	else
	{
		tracker.push_back(user);	
		return prepare_response('O', "You have been registered.");
	}
	
}

string to_ip_format(char * sequence)
{	
	string ip_formated = "";
	for(int i=0 ; i<4 ; ++i)
	{
		ip_formated += to_string((int)(sequence[i]))+".";
	}
	ip_formated = ip_formated.substr(0, ip_formated.size()-1);
	return ip_formated;
}

string list_of_pairs()
{
	string list = "";
	for(auto &peer : tracker)
	{
		if(peer.state)
			list += peer.IP + "," + to_string(peer.port) + "\n";
	}

	return prepare_response('W', list);
}

void receive_request(int SERVER_CONNECT_ID)
{
	int SERVER_STATUS;

	char header_buffer[PROTOCOL_HEADER_SIZE_BITS+1];
	bzero(header_buffer,PROTOCOL_HEADER_SIZE_BITS+1);
	char type_buffer[2];
	bzero(type_buffer, 2);

	SERVER_STATUS = read(SERVER_CONNECT_ID, header_buffer, PROTOCOL_HEADER_SIZE_BITS);
	if( SERVER_STATUS<0 )
		perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
	header_buffer[PROTOCOL_HEADER_SIZE_BITS] = '\0';


	SERVER_STATUS = read(SERVER_CONNECT_ID, type_buffer, 1);
	if( SERVER_STATUS<0 )
		perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
	type_buffer[1] = '\0';


	int message_size = stoi(header_buffer);
	
	char message_buffer[message_size+1];
	bzero(message_buffer,message_size+1);		
	cout<<"receiving something "<<type_buffer<<endl;
	switch(type_buffer[0])
	{
		case 'R': //register
		{
			while(var_alive);
			char ip_buffer[5];
			bzero(ip_buffer, 5);

			SERVER_STATUS = read(SERVER_CONNECT_ID, ip_buffer, 4);
			ip_buffer[4] = '\0';
			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");

			SERVER_STATUS = read(SERVER_CONNECT_ID, message_buffer, message_size);
			message_buffer[message_size] = '\0';

			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			Peer peer_creado;
			peer_creado.IP = to_ip_format(ip_buffer);
			peer_creado.port = stoi(message_buffer);
			peer_creado.state = true;
			string response_string =register_user(peer_creado); 
			
			write(SERVER_CONNECT_ID, response_string.c_str(), response_string.size());
			break;
		}
		case 'L': // request of logout
		{
			string response = "Successfully logged out";
			SERVER_STATUS = write(SERVER_CONNECT_ID, response.c_str(), response.size());
			break;
		}
		case 'P': // request of list of peers, only server side
		{
			string response = list_of_pairs();
			SERVER_STATUS = write(SERVER_CONNECT_ID, response.c_str(), response.size());

			if (SERVER_STATUS < 0) 
				perror("ERROR writing to socket");

			break;
		}
		case 'W': // only client side
		{
			string response = list_of_pairs();
			SERVER_STATUS = write(SERVER_CONNECT_ID, response.c_str(), response.size());

			if (SERVER_STATUS < 0) 
				perror("ERROR writing to socket");

			int SERVER_STATUS;

			char header_buffer_[PROTOCOL_HEADER_SIZE_BITS+1];
			bzero(header_buffer_,PROTOCOL_HEADER_SIZE_BITS+1);
			char type_buffer_[2];
			bzero(type_buffer_, 2);

			SERVER_STATUS = read(SERVER_CONNECT_ID, header_buffer_, PROTOCOL_HEADER_SIZE_BITS);

			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			header_buffer_[PROTOCOL_HEADER_SIZE_BITS] = '\0';

			SERVER_STATUS = read(SERVER_CONNECT_ID, type_buffer_, 1);
			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			type_buffer_[1] = '\0';

			if(type_buffer_[0] == 'N')
			{
				int message_size_ = stoi(header_buffer_);

				char num_parts[3];
				bzero(num_parts, 3);
				SERVER_STATUS = read(SERVER_CONNECT_ID, num_parts, 2);
				num_parts[2]='\0';
				int total_parts = stoi(num_parts);
				char file_name[message_size_];

				read(SERVER_CONNECT_ID,file_name, message_size_);
				file_name[message_size_] = '\0';
				vector<pair<int, string> > todas_las_partes;
				for(int i=0; i<total_parts; i++)
				{
					char header_buff[PROTOCOL_HEADER_SIZE_BITS+1];
					bzero(header_buff, PROTOCOL_HEADER_SIZE_BITS+1);
					char type_buff[2];
					bzero(type_buff,2);

					read(SERVER_CONNECT_ID, header_buff, PROTOCOL_HEADER_SIZE_BITS);
					header_buff[PROTOCOL_HEADER_SIZE_BITS]='\0';
					int size_of_part = stoi(header_buff);

					read(SERVER_CONNECT_ID, type_buff, 1);
					type_buff[1]='\0';
					if(type_buff[0] == 'P') //una parte del archivo
					{
						char buffer_indice[3];
						read(SERVER_CONNECT_ID, buffer_indice, 2);
						buffer_indice[2]='\0';
						
						int indice = stoi(buffer_indice);
						char parte_buffer[size_of_part+1];
						read(SERVER_CONNECT_ID, parte_buffer, size_of_part);
						parte_buffer[size_of_part] = '\0';
						string temp (parte_buffer);
						todas_las_partes.push_back(make_pair(indice,temp));	
					}
				}
				string temp2(file_name);
				archivos[temp2] = todas_las_partes;

				typedef map<string, vector<pair<int, string> > >::const_iterator MapIterator;
				for (MapIterator iter = archivos.begin(); iter != archivos.end(); iter++)
				{
					cout << "Key: " << iter->first<<endl;
					typedef vector<pair<int, string> >::const_iterator VectIterator;
					for (VectIterator vect_iter = iter->second.begin(); vect_iter != iter->second.end(); vect_iter++)
					{
						cout<<" "<<(*vect_iter).first<<"-"<<(*vect_iter).second<<endl;
					}
				}
			}
			else
				cout<<"Error reading the name of file"<<endl;

			break;
		}
		case 'D': // Download
		{
			cout<<"recibiendo una descarga"<<endl;
			char buffer_message[message_size+1];
			read(SERVER_CONNECT_ID, buffer_message, message_size);
			buffer_message[message_size]='\0';
			string temp(buffer_message);
			if(archivos[temp].size())
			{
				int total_parts = archivos[temp].size();
				stringstream ss;
				ss << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << to_string(total_parts).size();
				string message = ss.str()+'X'+'1'+to_string(total_parts);//0001 X 1 2

				write(SERVER_CONNECT_ID, message.c_str(), message.size());

				for(int i=0; i< archivos[temp].size(); i++)
				{
					//mandando el indice del peer q tiene el archivo
					stringstream ss2;
					int id_peer = archivos[temp][i].first;
					ss2 << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << to_string(id_peer).size();
					message = ss2.str()+'M'+to_string(id_peer);
					write(SERVER_CONNECT_ID, message.c_str(), message.size());

					//mandando el nombre de la parte del archivo
					stringstream ss3;
					string name_file = archivos[temp][i].second;
					ss3 << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << name_file.size();
					message = ss3.str()+'L'+name_file;
					write(SERVER_CONNECT_ID, message.c_str(), message.size());
				}
			}
			else
			{
				string message = "0000X"+'0';//0001 X 1 2
				write(SERVER_CONNECT_ID, message.c_str(), message.size());
			}
			break;		
		}
	}		

	shutdown(SERVER_CONNECT_ID, SHUT_RDWR);
	close(SERVER_CONNECT_ID);
}

void thread_keep_alive()
{
	while(1)
	{
		clock_t tiempo_inicial;
		clock_t tiempo_final;
		tiempo_inicial = clock();
		do
		{
			tiempo_final = clock();
		}
		while((double(tiempo_final-tiempo_inicial)/CLOCKS_PER_SEC ) < 10);
		
		var_alive = true;

		tiempo_inicial = clock();
		do
		{
			tiempo_final = clock();
		}
		while((double(tiempo_final-tiempo_inicial)/CLOCKS_PER_SEC ) < 2);
		
		for(auto &peer: tracker)
		{
			cout<<peer.IP<<"---"<<peer.state<<endl;
			if(peer.state == 1)
			{
				struct sockaddr_in sockAddr;
				sockAddr.sin_family = AF_INET;
				int CLIsocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
				sockAddr.sin_port = htons(peer.port);
				inet_pton(AF_INET, (peer.IP).c_str(), &sockAddr.sin_addr);

				long arg;
				fd_set sdset;
				struct timeval tv;
				socklen_t len;
				int result = -1, valopt;
				// Set socket to non-blocking
				arg = fcntl(CLIsocket, F_GETFL, NULL);
				arg |= O_NONBLOCK;
				fcntl(CLIsocket, F_SETFL, arg);
				// Connect with time limit
				std::string message;
				if ((result = connect(CLIsocket,(const struct sockaddr *)&sockAddr,sizeof(struct sockaddr_in)) < 0)) 
				{
					if (errno == EINPROGRESS)
					{
						tv.tv_sec = 2;
						tv.tv_usec = 0;
						FD_ZERO(&sdset);
						FD_SET(CLIsocket, &sdset);
						if (select(CLIsocket+1, NULL, &sdset, NULL, &tv) > 0)
						{
							len = sizeof(int);
							getsockopt(CLIsocket, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &len);
							if (valopt) {
								fprintf(stderr, "connect() error %d - %s\n", valopt, strerror(valopt));
								result = -1;
							}
							// connection established
							else result = 0;
						}
						else fprintf(stderr, "connect() timed out\n");
					}
					else
					{
						fprintf(stderr, "connect() error %d - %s\n", errno, strerror(errno));
						result = -1;
					}
				}
				// Return socket to blocking mode 
				arg = fcntl(CLIsocket, F_GETFL, NULL);
				arg &= (~O_NONBLOCK);
				fcntl(CLIsocket, F_SETFL, arg);
				if(result==-1)
				{
					peer.state = false;
					cout<<"Peer "<<peer.IP<<" desconectado"<<endl;
				}
			}
		}
		var_alive = false;
	}
}

int main(int argc, char *argv[])
{
	/* SERVER */
	struct sockaddr_in SERVER_STSOCKADDR;
	int SERVER_SOCKET = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int SERVER_STATUS;
	
	if(-1 == SERVER_SOCKET)
	{
		perror("can not create socket");
		exit(EXIT_FAILURE);
	}

	memset(&SERVER_STSOCKADDR, 0, sizeof(struct sockaddr_in));

	SERVER_STSOCKADDR.sin_family = AF_INET;
	SERVER_STSOCKADDR.sin_port = htons(9000);
	SERVER_STSOCKADDR.sin_addr.s_addr = INADDR_ANY;

	if(-1 == bind(SERVER_SOCKET,(const struct sockaddr *)&SERVER_STSOCKADDR, sizeof(struct sockaddr_in)))
	{
		perror("error bind failed");
		close(SERVER_SOCKET);
		exit(EXIT_FAILURE);
	}

	if(-1 == listen(SERVER_SOCKET, 10))
	{
		perror("error listen failed");
		close(SERVER_SOCKET);
		exit(EXIT_FAILURE);
	}

	thread(thread_keep_alive).detach();
	while(1)
	{

		int SERVER_CONNECT_ID = accept(SERVER_SOCKET, NULL, NULL);

		if(0 > SERVER_CONNECT_ID)
		{
			perror("error accept failed");
			close(SERVER_SOCKET);
			exit(EXIT_FAILURE);
		}
		thread(receive_request, SERVER_CONNECT_ID).detach();
	}
	
	/* SERVER */
	
}
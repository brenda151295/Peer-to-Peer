/* Client code in C++11 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <time.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <utility>
#include <functional>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <time.h>  


using namespace std;

#define PORT_TRACKER 9000
#define PROTOCOL_HEADER_SIZE_BITS 4
#define SIZE 2

struct sockaddr_in CLIstSockAddr;
int CLISocket;
int myPort;
string myIp;
vector<string> files;

int have_file(string name_file)
{
	for(int i=0; i<files.size(); i++)
	{
		if(files[i] == name_file)
			return i;
	}
	return -1;
}

string to_char_ip_format(string sequence)
{ 
	string char_ip_formated = "";
	string token;
	istringstream iss(sequence);

	while(getline(iss, token, '.'))
	{
		int num = stoi(token);

		char_ip_formated += (char)num;
	}

	return char_ip_formated;
}
vector<pair<string, string>> list_peers_to_vector(string list_peers)
{
	vector<pair<string, string>> vector_peers;

	istringstream f(list_peers);
	string peer;    
	while (getline(f, peer, '\n')) {
		int pos = peer.find(",");
		string IP = peer.substr(0,pos);
		string Port = peer.substr(pos+1, peer.size()-IP.size());
		vector_peers.push_back(make_pair(IP, Port));
	}
	return vector_peers;
}
bool sign_in(string IP, string Port, int CLISocket)
{
	string IP_format = to_char_ip_format(IP);
	string IP_Port = IP_format + Port;

	int size_port = Port.size();
	stringstream ss;
	ss << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << size_port;
	string temp = ss.str()+'R'+IP_format+Port;
	write(CLISocket, temp.c_str(), PROTOCOL_HEADER_SIZE_BITS+1+4+size_port);
	char buffer[5];
	int n;
	n = read(CLISocket, buffer, 4);
	buffer[4]='\0';
	char ok[2];
	int size_msg = stoi(buffer);
	read(CLISocket, ok, 1);
	ok[1] = '\0';

	char buffer_msg[size_msg+1];
	read(CLISocket, buffer_msg, size_msg);
	buffer_msg[size_msg]='\0';

	cout<<buffer_msg<<endl;

	if(ok[0]=='O')
		return 1;
	else
		return 0;

}
void sign_in_option()
{
	cout<<"You need to sig in, please"<<endl;
	cout<<"Please enter your IP: "<<endl;
	cin>>myIp;
	cout<<"Please enter your port: "<<endl;
	cin>>myPort;
	int Res;
	//MAKE THE CONNECTION WITH THE TRACKER
	int CLISocket2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	{

		if (-1 == CLISocket2)
		{
			perror("cannot create socket");
			exit(EXIT_FAILURE);
		}
		CLIstSockAddr.sin_family = AF_INET;
		CLIstSockAddr.sin_port = htons(PORT_TRACKER);
		Res = inet_pton(AF_INET, "127.0.0.1", &CLIstSockAddr.sin_addr);

		if (0 > Res)    {
			perror("error: first parameter is not a valid address family");
			close(CLISocket2);
			exit(EXIT_FAILURE);
		}
		else if (0 == Res)    {
			perror("char string (second parameter does not contain valid ipaddress");
			close(CLISocket2);
			exit(EXIT_FAILURE);
		}
		if (-1 == connect(CLISocket2, (const struct sockaddr *)&CLIstSockAddr, sizeof(struct sockaddr_in)))
		{
			perror("connect failed");
			close(CLISocket2);
			exit(EXIT_FAILURE);
		}
	}

	//SIG IN A PEER
	if(sign_in(myIp,to_string(myPort),CLISocket2))
	{
		shutdown(CLISocket2, SHUT_RDWR);
		close(CLISocket2);
	}
	else
	{
		sign_in_option();
	}
	
}

void thread_create_clients(string part_of_file, string name_file, string IP,  string Port, int size_file, int position, vector<pair<string,string> > lista_peers)
{
	const char* IP_char = IP.c_str();
	int Port_int = stoi(Port);

	int CLISocket2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == CLISocket2)
	{
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}
	CLIstSockAddr.sin_family = AF_INET;
	CLIstSockAddr.sin_port = htons(Port_int);
	int Res = inet_pton(AF_INET, IP_char, &CLIstSockAddr.sin_addr);

	if (0 > Res)    {
		perror("error: first parameter is not a valid address family");
		close(CLISocket2);
		exit(EXIT_FAILURE);
	}
	else if (0 == Res)    {
		perror("char string (second parameter does not contain valid ipaddress");
		close(CLISocket2);
		exit(EXIT_FAILURE);
	}
	if (-1 == connect(CLISocket2, (const struct sockaddr *)&CLIstSockAddr, sizeof(struct sockaddr_in)))
	{
		perror("connect failed create clients");
		close(CLISocket2);
		exit(EXIT_FAILURE);
	}


	//Format name file: 0008Nfile.txt

	//writing name of file
	int size_name_file = name_file.size();
	stringstream ss;
	ss << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << size_name_file;
	string temp = ss.str()+'N'+name_file;
	write(CLISocket2, temp.c_str(), temp.size());

	//writing size of file
	stringstream st;
	string temp2 = to_string(size_file);
	st << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << temp2.size();
	string sizeFile= to_string(size_file);
	temp = st.str() + 'T'+ sizeFile;
	write(CLISocket2, temp.c_str(), temp.size());

	//writing part of file
	stringstream sr;
	sr << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << part_of_file.size();
	stringstream s_pos;
	s_pos << setfill('0') << setw(2) << position;
	string pos = s_pos.str();
	temp = sr.str()+'F'+pos+part_of_file;
	write(CLISocket2, temp.c_str(), temp.size());

	
	//writing in track

	string name_file_saved = name_file +'.'+"part"+pos;
	cout<<"NOMBRE:"<<name_file_saved<<endl;

	int p;
	for(int i=0; i<lista_peers.size();i++)
	{
		if(lista_peers[i].first == IP and lista_peers[i].second == Port)
		{
			cout<<"position:"<<p<<endl;
			p = i;
		}
	}

	int size_name_file2=name_file_saved.size();
	stringstream su;
	su << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << size_name_file2;
	stringstream sy;
	sy << setfill('0') << setw(2) << p;
	temp = su.str() + 'P'+sy.str()+ name_file_saved;
	write(CLISocket, temp.c_str(), temp.size());
	shutdown(CLISocket2, SHUT_RDWR);
	close(CLISocket2);	

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

	switch(type_buffer[0])
	{
		case 'N': //GUARDAR UN PEDAZO DE ARCHIVO
		{
			//reading name of file
			SERVER_STATUS = read(SERVER_CONNECT_ID, message_buffer, message_size);
			message_buffer[message_size] = '\0';
			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			string name_file(message_buffer);

			//reading size of file
			char header_buffer1 [PROTOCOL_HEADER_SIZE_BITS+1];
			bzero(header_buffer,PROTOCOL_HEADER_SIZE_BITS+1);
			bzero(type_buffer, 2);
			SERVER_STATUS = read(SERVER_CONNECT_ID, header_buffer1, PROTOCOL_HEADER_SIZE_BITS);
			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			header_buffer1[PROTOCOL_HEADER_SIZE_BITS] = '\0';
			

			SERVER_STATUS = read(SERVER_CONNECT_ID, type_buffer, 1);
			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			type_buffer[1] = '\0';
			

			message_size = stoi(header_buffer1);
			message_buffer[message_size+1];
			bzero(message_buffer,message_size+1);
			
			if(type_buffer[0] == 'T')
			{
				SERVER_STATUS = read(SERVER_CONNECT_ID, message_buffer, message_size);
				message_buffer[message_size] = '\0';

				int size_file_received = stoi(message_buffer); 
				if( SERVER_STATUS<0 )
					perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			}
			else
			{
				cout<<"ERROR reading"<<endl;
				break;
			}
			
			//reading part of file
			bzero(header_buffer,PROTOCOL_HEADER_SIZE_BITS+1);
			bzero(type_buffer, 2);
			SERVER_STATUS = read(SERVER_CONNECT_ID, header_buffer, PROTOCOL_HEADER_SIZE_BITS);
			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			header_buffer[PROTOCOL_HEADER_SIZE_BITS] = '\0';

			
			SERVER_STATUS = read(SERVER_CONNECT_ID, type_buffer, 1);
			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			type_buffer[1] = '\0';

			message_size = stoi(header_buffer);
			
			//reading the position (2B)
			char buffer_position[3];
			char content_buffer[message_size];
			
			if(type_buffer[0] == 'F')
			{
				
				SERVER_STATUS = read(SERVER_CONNECT_ID, buffer_position, 2);
				buffer_position[2]='\0';
				
				if( SERVER_STATUS<0 )
					perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");

				SERVER_STATUS = read(SERVER_CONNECT_ID, content_buffer, message_size);
				content_buffer[message_size] = '\0';

				if( SERVER_STATUS<0 )
					perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			}
			else
			{
				cout<<"ERROR reading"<<endl;
				break;
			}

			string temp(name_file);
			string temp2(buffer_position);
			string name_file_saved = temp +'.'+"part"+temp2;
			ofstream myfile;
			myfile.open (name_file_saved);
			myfile << content_buffer;
			myfile.close();

			files.push_back(name_file_saved);
			cout<<"my_files:"<<endl;
			for(auto &a:files)
				cout<<a<<endl;

			break;
		}
		case 'D': //BAJAR UN ARCHIVO
		{
			//reading name of file
			SERVER_STATUS = read(SERVER_CONNECT_ID, message_buffer, message_size);
			message_buffer[message_size] = '\0';
			if( SERVER_STATUS<0 )
				perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
			string name_file(message_buffer);

			int indice = have_file(name_file);
			int indice_file = stoi(files[indice].substr(files[indice].size()-2));
			ifstream file;
			cout<<"opening:"<<files[indice]<<endl;
			file.open(files[indice]);
			string str;
			string file_contents;
			while (std::getline(file, str))
			{
			  file_contents += str;
			  file_contents.push_back('\n');
			}
			//enviando part_of_file  
			int size_file = file_contents.size();
			stringstream ss;
			ss << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << size_file;

			stringstream st;
			st << setfill('0') << setw(2) << indice_file;
			string message = ss.str() + 'U' + st.str()+file_contents;
			write(SERVER_CONNECT_ID, message.c_str(), message.size());
			break;
		}
		
	}
}

void choose_peers(vector<int> &vec, int num_peers)
{
  vector<int>::iterator iter;
  int num = rand() % num_peers;
  vec.push_back(num);

  while(vec.size()!=SIZE)
  { 
    srand (time(NULL));
    num = rand() % num_peers;
    iter =find (vec.begin(), vec.end(), num);
    if(iter != vec.end())
      continue;
    else
      vec.push_back(num);
  }
}
bool peers_correctos(vector<int> peers_choosen, vector<pair<string, string>> vector_peers)
{
	for(auto &peer: peers_choosen)
	{
		if(vector_peers[peer].first == myIp and stoi(vector_peers[peer].second) == myPort)
			return false;
	}
	return true;
}
void get_files(vector<pair<int, string>> vector_files, string name_file)
{
	CLISocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int Res;
		{
			if (-1 == CLISocket)
			{
				perror("cannot create socket");
				exit(EXIT_FAILURE);
			}
			CLIstSockAddr.sin_family = AF_INET;
			CLIstSockAddr.sin_port = htons(PORT_TRACKER);
			Res = inet_pton(AF_INET, "127.0.0.1", &CLIstSockAddr.sin_addr);

			if (0 > Res)    {
				perror("error: first parameter is not a valid address family");
				close(CLISocket);
				exit(EXIT_FAILURE);
			}
			else if (0 == Res)    {
				perror("char string (second parameter does not contain valid ipaddress");
				close(CLISocket);
				exit(EXIT_FAILURE);
			}
			if (-1 == connect(CLISocket, (const struct sockaddr *)&CLIstSockAddr, sizeof(struct sockaddr_in)))
			{
				perror("connect failed for load files");
				close(CLISocket);
				exit(EXIT_FAILURE);
			}
		}
	string temp = "0000P";
	//MAKE THE CONNECTION WITH THE TRACKER

	write(CLISocket, temp.c_str(), temp.size());

	char header_buffer[PROTOCOL_HEADER_SIZE_BITS];
	read(CLISocket, header_buffer, PROTOCOL_HEADER_SIZE_BITS);
	header_buffer[PROTOCOL_HEADER_SIZE_BITS] = '\0';
	
	char type_buffer[2];
	read(CLISocket, type_buffer, 1);
	type_buffer[1] = '\0';
	int size_list = stoi(header_buffer);
	char list_buffer[size_list];
	if(type_buffer[0]=='W')
	{
		read(CLISocket, list_buffer, size_list);
		list_buffer[size_list] = '\0';
	}

	else
		cout<<"Error receiving the list"<<endl;	
	string str(list_buffer);
	vector<pair<string, string> > vector_peers = list_peers_to_vector(str);
	vector<pair<int, string> > total_file;
	for(auto &peer: vector_files)
	{	
		string IP = vector_peers[peer.first].first;
		string Port = vector_peers[peer.first].second;

		const char* IP_char = IP.c_str();
		int Port_int = stoi(Port);

		int CLISocket2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (-1 == CLISocket2)
		{
			perror("cannot create socket");
			exit(EXIT_FAILURE);
		}
		CLIstSockAddr.sin_family = AF_INET;
		CLIstSockAddr.sin_port = htons(Port_int);
		int Res = inet_pton(AF_INET, IP_char, &CLIstSockAddr.sin_addr);

		if (0 > Res)    {
			perror("error: first parameter is not a valid address family");
			close(CLISocket2);
			exit(EXIT_FAILURE);
		}
		else if (0 == Res)    {
			perror("char string (second parameter does not contain valid ipaddress");
			close(CLISocket2);
			exit(EXIT_FAILURE);
		}
		if (-1 == connect(CLISocket2, (const struct sockaddr *)&CLIstSockAddr, sizeof(struct sockaddr_in)))
		{
			perror("connect failed create clients");
			close(CLISocket2);
			exit(EXIT_FAILURE);
		}

		int size_name_file = peer.second.size();
		stringstream ss;
		ss << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << size_name_file;
		string temp = ss.str()+'D'+peer.second;
		write(CLISocket2, temp.c_str(), temp.size());

		char header_buff[PROTOCOL_HEADER_SIZE_BITS+1];
		read(CLISocket2, header_buff, PROTOCOL_HEADER_SIZE_BITS);
		header_buff[PROTOCOL_HEADER_SIZE_BITS]='\0';

		char buffer_type[2];
		read(CLISocket2, buffer_type, 1);
		buffer_type[1]='\0';
		if(buffer_type[0] == 'U')
		{
			char buffer_indice[3];
			read(CLISocket2, buffer_indice, 2);
			buffer_indice[2]='\0';
			char buffer_msj[stoi(header_buff)+1];
			read(CLISocket2, buffer_msj, stoi(header_buff));
			buffer_msj[stoi(header_buff)]='\0';
			string temp_buffer_msj(buffer_msj);
			total_file.push_back(make_pair(stoi(buffer_indice), temp_buffer_msj));
		}
		else
		{
			cout<<"error reading part of file"<<endl;
		}
	}
	string name_file_download = name_file + ".descargado";
	ofstream file_download;
	file_download.open(name_file_download);

	for(int i = 0 ; i < total_file.size(); i++)
	{
		for(auto &a:total_file)
		{
			if(a.first == i)
				file_download<<a.second;		
		}
	}
	file_download.close();
}

void thread_menu()
{
	while(1)
	{
		int opcion;
		cout<<"1.List"<<endl;
		cout<<"2.Upload File"<<endl;
		cout<<"3.Download File"<<endl;
		cin>>opcion;
		CLISocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		int Res;
				{
					if (-1 == CLISocket)
					{
						perror("cannot create socket");
						exit(EXIT_FAILURE);
					}
					CLIstSockAddr.sin_family = AF_INET;
					CLIstSockAddr.sin_port = htons(PORT_TRACKER);
					Res = inet_pton(AF_INET, "127.0.0.1", &CLIstSockAddr.sin_addr);

					if (0 > Res)    {
						perror("error: first parameter is not a valid address family");
						close(CLISocket);
						exit(EXIT_FAILURE);
					}
					else if (0 == Res)    {
						perror("char string (second parameter does not contain valid ipaddress");
						close(CLISocket);
						exit(EXIT_FAILURE);
					}
					if (-1 == connect(CLISocket, (const struct sockaddr *)&CLIstSockAddr, sizeof(struct sockaddr_in)))
					{
						perror("connect failed for load files");
						close(CLISocket);
						exit(EXIT_FAILURE);
					}
				}
		switch(opcion)
		{	
			case 1:
			{
				string temp = "0000P";
				//MAKE THE CONNECTION WITH THE TRACKER

				write(CLISocket, temp.c_str(), temp.size());

				char header_buffer[PROTOCOL_HEADER_SIZE_BITS];
				read(CLISocket, header_buffer, PROTOCOL_HEADER_SIZE_BITS);
				header_buffer[PROTOCOL_HEADER_SIZE_BITS] = '\0';
				
				char type_buffer[2];
				read(CLISocket, type_buffer, 1);
				type_buffer[1] = '\0';
				if(type_buffer[0]=='W')
				{
					int size_list = stoi(header_buffer);
					char list_buffer[size_list];
					read(CLISocket, list_buffer, size_list);
					list_buffer[size_list] = '\0';
					cout<<"List: \n"<<list_buffer<<endl;
				}
				else
					cout<<"Error receiving the list"<<endl;	
				
				shutdown(CLISocket, SHUT_RDWR);
				close(CLISocket);
				break;
			}
			case 2:
			{
				string name_file;
				cout << "Enter file's name: " << endl ;
				cin >> name_file;

				string temp = "0000W";

				write(CLISocket, temp.c_str(), temp.size());

				char header_buffer[PROTOCOL_HEADER_SIZE_BITS];
				read(CLISocket, header_buffer, PROTOCOL_HEADER_SIZE_BITS);
				header_buffer[PROTOCOL_HEADER_SIZE_BITS] = '\0';
				
				char type_buffer[2];
				read(CLISocket, type_buffer, 1);
				type_buffer[1] = '\0';
				int size_list = stoi(header_buffer);
				char list_buffer[size_list];

				if(type_buffer[0]=='W')
				{
					read(CLISocket, list_buffer, size_list);
					list_buffer[size_list] = '\0';
				}
				else
				{
					cout<<"Error receiving the list of peers"<<endl;	
					break;
				}


				string str(list_buffer);
				vector<pair<string, string>> vector_peers = list_peers_to_vector(str);
				
				int num_peers = vector_peers.size(); 
				cout<<"Number of peers: "<<num_peers-1<<endl; //menos el peer mismo q esta mandando

			//SPLIT FILE
				ifstream file;
				file.open(name_file, ios::in | ios::binary);

				file.seekg (0, ios::end);
				double length = file.tellg();

				file.seekg (0, ios::beg);
				int size_each_part = ceil(length/(num_peers-1));
				char buffer_part[size_each_part];
				vector<int> peers_choosen;

				choose_peers(peers_choosen, num_peers);
				while(!peers_correctos(peers_choosen, vector_peers))
				{
					peers_choosen.clear();
					choose_peers(peers_choosen, num_peers);
				}
				int indice = 0;
				
				//enviando el nombre del archivo al tracker
				int size_name_file = name_file.size();
				stringstream ss;
				ss << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << size_name_file;

				stringstream ss2;
				ss2 << setfill('0') << setw(2) << peers_choosen.size();

				string temp2 = ss.str()+'N'+ss2.str()+name_file;
				const char * total = new char[PROTOCOL_HEADER_SIZE_BITS+1+2+size_name_file];
				total = temp2.c_str();	

				int SERVER_STATUS = write(CLISocket, total, PROTOCOL_HEADER_SIZE_BITS+1+2+size_name_file);
				if( SERVER_STATUS<0 )
					perror("[ERROR] Reading PROTOCOL_SIZE_BITS from socket");
				cout<<"write: "<<total<<endl;

			    for(auto &peer: peers_choosen)
			    {
			    	cout<<"PEERS CHOOSEN"<<endl;
			    	cout<<vector_peers[peer].first << "," << vector_peers[peer].second<<endl;
					file.read(buffer_part, size_each_part);
					buffer_part[size_each_part]='\0';
					string string_buffer_part(buffer_part);
					thread_create_clients(string_buffer_part, name_file, vector_peers[peer].first, vector_peers[peer].second, length, indice, vector_peers);				    	
					indice++;

			    }
			    peers_choosen.clear();		
				break;	
			}
			case 3:
			{

				bool flag = 0;
				string name_file;
				cout << "Enter file's name: " << endl ;
				cin >> name_file;
				int size_name_file = name_file.size();
				stringstream ss;
				ss << setfill('0') << setw(PROTOCOL_HEADER_SIZE_BITS) << size_name_file;
				string message = ss.str()+'D'+name_file;
				const char * total = new char[PROTOCOL_HEADER_SIZE_BITS+1+size_name_file];
				total = message.c_str();	
				write(CLISocket, total, message.size());
				char buffer_head[PROTOCOL_HEADER_SIZE_BITS+1];

				read(CLISocket, buffer_head, PROTOCOL_HEADER_SIZE_BITS);
				buffer_head[PROTOCOL_HEADER_SIZE_BITS]='\0';
				char type_message[2];
				read(CLISocket,type_message,1);
				type_message[1]='\0';
				if(type_message[0] == 'X')
				{
					char buffer_check[2];
					read(CLISocket, buffer_check,1);
					buffer_check[1]='\0';
					if(stoi(buffer_check))
					{
						char buffer[stoi(buffer_head)+1];
						read(CLISocket, buffer, stoi(buffer_head));
						buffer[stoi(buffer_head)]='\0';

						vector<pair<int, string>> vector_part_files;
						//cout<<"NAME FILE"<<endl;
						for(int i=0; i<stoi(buffer);i++)
						{
							bzero(buffer_head,PROTOCOL_HEADER_SIZE_BITS+1);
							read(CLISocket, buffer_head, PROTOCOL_HEADER_SIZE_BITS);
							buffer_head[PROTOCOL_HEADER_SIZE_BITS]='\0';
							int size_message = stoi(buffer_head);
							bzero(type_message,2);
							read(CLISocket, type_message, 1);
							type_message[1]='\0';
							if(type_message[0]=='M')
							{
								char buffer_message[size_message+1];
								read(CLISocket, buffer_message, size_message);
								buffer_message[size_message]='\0';
								int indice_archivo = stoi(buffer_message);

								bzero(buffer_head,PROTOCOL_HEADER_SIZE_BITS+1);
								read(CLISocket, buffer_head, PROTOCOL_HEADER_SIZE_BITS);
								buffer_head[PROTOCOL_HEADER_SIZE_BITS]='\0';
								size_message = stoi(buffer_head);
								bzero(type_message,2);
								read(CLISocket, type_message, 1);
								type_message[1]='\0';

								if(type_message[0] == 'L')
								{
									char buffer_name_file[size_message+1];
									read(CLISocket, buffer_name_file, size_message);
									cout<<buffer_name_file<<endl;
									buffer_name_file[size_message]='\0';
									string temp_name_file(buffer_name_file);
									vector_part_files.push_back(make_pair(indice_archivo, temp_name_file));
									flag = 1;
								}
								else
									cout<<"Fail reading index"<<endl;

							}
							else
								cout<<"Fail reading a part"<<endl;
						}
						if(flag)
							get_files(vector_part_files, name_file);
						else
							break;

					}
					else
					{
						cout<<"File not found"<<endl;
						break;
					}
				}
				else
				{
					cout<<"Fail getting file"<<endl;
					break;
				}
				break;
			}
		}
		shutdown(CLISocket, SHUT_RDWR);
		close(CLISocket);
	}
	
}

int main(int argc, char *argv[])
{
	
	struct sockaddr_in SERstSockAddr;
	int SERSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	char buffer[256];
	int Res;

	if(-1 == SERSocket)
	{
		perror("can not create socket");
		exit(EXIT_FAILURE);
	}
	cout<<"Create Socket"<<endl;

	sign_in_option();
	SERstSockAddr.sin_family = AF_INET;
	SERstSockAddr.sin_port = htons(myPort);
	SERstSockAddr.sin_addr.s_addr = INADDR_ANY; 

	if(-1 == bind(SERSocket,(const struct sockaddr *)&SERstSockAddr, sizeof(struct sockaddr_in)))
	{
		perror("error bind failed---");
		close(SERSocket);
		exit(EXIT_FAILURE);
	}

	if(-1 == listen(SERSocket, 10))
	{
		perror("error listen failed");
		close(SERSocket);
		exit(EXIT_FAILURE);
	}
	
	thread (thread_menu).detach();	

	while(1)
	{
		int NEWCLISocket = accept(SERSocket, NULL, NULL);
		if(0 > NEWCLISocket) {
			perror("error accept failed");
			close(SERSocket);
			exit(EXIT_FAILURE);
		}
		thread(receive_request, NEWCLISocket).detach();

	}
}
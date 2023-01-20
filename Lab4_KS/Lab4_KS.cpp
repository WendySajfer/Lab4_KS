#include <iostream>
#include <string>
#include <winsock.h>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

string directory_to_write = "G:\\My\\Proga\\Labs-KS\\lab4\\";
int http_port[2] = { 80,443 };
string http_version[2] = { "HTTP/1.1","HTTP/1.1" };
bool use_secured_connection_ = false;

void parse_url(string* url, string* protocol, string* hostname, string* locator, bool* use_secured_connection = NULL) {
	*protocol = "";
	*hostname = "";
	*locator = "/";

	int hostname_index = url->find("://");
	if (hostname_index == -1)
	{
		hostname_index = 0;
	}
	else {
		hostname_index += 3;
	}
	*protocol = url->substr(0, hostname_index);
	*hostname = url->substr(hostname_index);

	hostname_index = hostname->find("/");
	if (hostname_index != -1) {
		*locator = hostname->substr(hostname_index);
		*hostname = hostname->substr(0, hostname_index);
	}

	if (use_secured_connection != NULL) {
		*use_secured_connection = (*protocol == "https://");
	}

	//cout << protocol << "^^^" << hostname << "^^^" << locator << endl;
}

string constructGETRequest(string* url) {

	string protocol, hostname, locator;
	bool use_secured_connection = false;
	parse_url(url, &protocol, &hostname, &locator, &use_secured_connection);
	use_secured_connection_ = use_secured_connection;

	string request = "";
	request += "GET " + locator + " " + http_version[use_secured_connection] + "\r\n";
	request += "Host: " + hostname + "\r\n";
	request += "\r\n";

	return request;
};

string constructHEADRequest(string* url) {

	string protocol, hostname, locator;
	bool use_secured_connection = false;
	parse_url(url, &protocol, &hostname, &locator, &use_secured_connection);
	use_secured_connection_ = use_secured_connection;

	string request = "";
	request += "HEAD " + locator + " " + http_version[use_secured_connection] + "\r\n";
	request += "Host: " + hostname + "\r\n";
	request += "\r\n";

	return request;
};

int find_head(string* answer) {
	int head_length = answer->find("\r\n\r\n");
	if (head_length != -1) {
		head_length += 4;
	}
	return head_length;
}

int find_head_and_get_size_transfer_method(string* answer, int* answer_size) {
	int head_length = find_head(answer);
	if (head_length == -1) {
		*answer_size = -1;
		return -1;
	}

	int pos = answer->find("Transfer-Encoding: chunked", 0);
	if (pos != -1 && pos < head_length) {
		*answer_size = 0;
		return 1;
	}

	int content_lengh = answer->find("Content-Length: ", 0);
	if (content_lengh != -1 && content_lengh < head_length) {
		content_lengh += 16;
		*answer_size = atoi(answer->substr(content_lengh, answer->find("\r\n\r\n", content_lengh)).c_str());
		return 2;
	}

	*answer_size = -1;
	return 0;
}


string send_request(string* url, string* request) {

	/*string host_name;
	int hostname_index = url->find("://");
	if (hostname_index == -1)
	{
		hostname_index = 0;
	}
	else {
		hostname_index += 3;
	}
	host_name = url->substr(hostname_index);
	hostname_index = host_name.find("/");
	if (hostname_index != -1)
	{
		host_name = host_name.substr(0, hostname_index);
	}*/
	string protocol, hostname, locator;
	bool use_secured_connection = false;
	parse_url(url, &protocol, &hostname, &locator, &use_secured_connection);

	WSADATA lpWSAData;
	SOCKET sock;
	WSAStartup(MAKEWORD(1, 1), &lpWSAData);

	hostent* hp;
	hp = gethostbyname(hostname.c_str());
	if (hp == NULL) {
		cout << "cannot resolve host\n";
		return NULL;
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cout << WSAGetLastError() << endl;
		return NULL;
	}

	sockaddr_in s_addr_;
	ZeroMemory(&s_addr_, sizeof(s_addr_));
	s_addr_.sin_family = AF_INET;
	s_addr_.sin_addr.S_un.S_un_b.s_b1 = hp->h_addr[0];
	s_addr_.sin_addr.S_un.S_un_b.s_b2 = hp->h_addr[1];
	s_addr_.sin_addr.S_un.S_un_b.s_b3 = hp->h_addr[2];
	s_addr_.sin_addr.S_un.S_un_b.s_b4 = hp->h_addr[3];
	s_addr_.sin_port = htons(http_port[use_secured_connection]);
	if (connect(sock, (sockaddr*)&s_addr_, sizeof(s_addr_)) == SOCKET_ERROR) {
		cout << WSAGetLastError() << endl;
		return NULL;
	}

	if (send(sock, request->c_str(), strlen(request->c_str()), NULL) == SOCKET_ERROR)
	{
		cout << WSAGetLastError() << endl;
		return NULL;
	};

	int numRd = 0;
	int content_lengh = -1;
	int head_length = -1;
	string answer = "";
	int recv_buff_size = 4096;
	char* recv_buff = new char[recv_buff_size + 1]{};
	int size_transfer_method = -1;

	int sock_msg_size = 1;

	while (size_transfer_method == -1) {
		int sock_msg_size = recv(sock, recv_buff, recv_buff_size, MSG_PEEK);

		if (sock_msg_size == 0) {
			break;
		}
		else if (sock_msg_size < 0)
		{
			cout << WSAGetLastError() << endl;
			return NULL;
		}

		answer.append(recv_buff, sock_msg_size);
		size_transfer_method = find_head_and_get_size_transfer_method(&answer, &content_lengh);
		head_length = find_head(&answer);
		answer.clear();
	}

	while (answer.size() < head_length) {
		int num_readed = recv(sock, recv_buff, min(head_length, recv_buff_size), 0);
		numRd += num_readed;
		answer.append(recv_buff, num_readed);
	}

	switch (size_transfer_method)
	{
	case 1:
		while (true) {
			char* recv_buff_2 = new char[2]{};
			string temp = "";
			while (true)
			{
				int sock_msg_size = recv(sock, recv_buff_2, 1, 0);
				if (sock_msg_size == 0) {
					break;
				}
				else if (sock_msg_size < 0)
				{
					cout << WSAGetLastError() << endl;
					return NULL;
				}
				if (*recv_buff_2 == *"\r") {
					int sock_msg_size = recv(sock, recv_buff_2, 1, 0);
					if (temp.size() == 0) {
						continue;
					}
					else {
						break;
					}
				}
				temp.append(recv_buff_2, sock_msg_size);
			}
			int chunk_size = stoi(temp, nullptr, 16);
			if (chunk_size == 0) {
				break;
			}

			char* recv_buff_3 = new char[chunk_size + 1]{};
			int num_readed = recv(sock, recv_buff_3, chunk_size, 0);
			if (num_readed < 0 && errno != EINTR)
			{
				fprintf(stderr, "Exit %d\n", __LINE__);
				exit(1);
			}
			else if (num_readed > 0)
			{
				numRd += num_readed;
				answer.append(recv_buff_3, num_readed);
			}
		}
		break;
	case 2:
		while (true)
		{
			int num_readed = recv(sock, recv_buff, min(sock_msg_size, recv_buff_size), 0);
			if (num_readed == 0)
			{
				break;
			}
			else if (num_readed < 0 && errno != EINTR)
			{
				fprintf(stderr, "Exit %d\n", __LINE__);
				exit(1);
			}
			else if (num_readed > 0)
			{
				numRd += num_readed;
				answer.append(recv_buff, num_readed);
			}

			if (head_length == -1)
			{
				size_transfer_method = find_head_and_get_size_transfer_method(&answer, &content_lengh);
				head_length = find_head(&answer);
			}

			if (numRd >= content_lengh + head_length) {
				break;
			}
		}
		break;
	case 0:
	default:
		while (true)
		{
			int num_readed = recv(sock, recv_buff, min(sock_msg_size, recv_buff_size), 0);
			if (num_readed == 0)
			{
				break;
			}
			else if (num_readed < 0 && errno != EINTR)
			{
				fprintf(stderr, "Exit %d\n", __LINE__);
				exit(1);
			}
			else if (num_readed > 0)
			{
				numRd += num_readed;
				answer.append(recv_buff, num_readed);
			}

			if (head_length == -1)
			{
				size_transfer_method = find_head_and_get_size_transfer_method(&answer, &content_lengh);
				head_length = find_head(&answer);
			}

			if (answer.find("</html>") != -1 || answer.find("</rss>") != -1) {
				break;
			}
		}
		break;
	}

	delete[] recv_buff;
	closesocket(sock);
	WSACleanup();

	return answer;
}

char get_reverce_bracket(char bracket) {
	switch (bracket) {
	case * "\"":
		return *"\"";
	case * "\'":
		return *"\'";

	case * "(":
		return *")";
	case * ")":
		return *"(";
	case * "{":
		return *"}";
	case * "}":
		return *"{";
	case * "[":
		return *"]";
	case * "]":
		return *"[";
	case * "<":
		return *">";
	case * ">":
		return *"<";

	default:
		return NULL;
	}
}

void writeToFile(string filePath, string* data) {
	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hFile == NULL) {
		cout << GetLastError() << endl;
		return;
	}
	HANDLE hFileMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, data->length(), NULL);
	if (hFileMapping == NULL) {
		cout << GetLastError() << endl;
		return;
	}
	char* file = (char*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, data->length());
	if (file == NULL) {
		cout << GetLastError() << endl;
		return;
	}

	CopyMemory(file, data->c_str(), data->length());
	if (!FlushViewOfFile(file, sizeof(file)))
	{
		printf("Could not flush memory to disk (%d).\n", GetLastError());
	}
	UnmapViewOfFile(file);
	CloseHandle(hFileMapping);
	CloseHandle(hFile);
}

bool get_list_of_items(string html_text, vector<vector<string>>* list_of_items) {
	int i = 0, j = 0, buf = 0;
	while (i != html_text.size()) {
		vector<string> item;
		buf = html_text.find("<item>", i);
		if (buf == string::npos) break;
		i = buf + 6;
		buf = html_text.find("<title>", i);
		if (buf != string::npos)
		{
			i = buf + 7;
			buf = html_text.find("</title>", i);
			if (buf != string::npos) {
				j = buf;
				item.push_back(html_text.substr(i, j - i - 1));
				i = j + 8;
				buf = html_text.find("<link>", i);
				if (buf != string::npos) {
					i = buf + 6;
					buf = html_text.find("</link>", i);
					if (buf != string::npos) {
						j = buf;
						item.push_back(html_text.substr(i, j - i - 1));
						i = j + 7;
						buf = html_text.find("<description>", i);
						if (buf != string::npos) {
							i = buf + 13;
							buf = html_text.find("</description>", i);
							if (buf != string::npos) {
								j = buf;
								item.push_back(html_text.substr(i, j - i - 1));
								i = j + 14;
								(*list_of_items).push_back(item);
							}
						}
					}
				}
			}
		}
	}
	if ((*list_of_items).size() == 0) return false;
	return true;
}

string construct_webpage(vector<vector<string>>* list_of_items) {
	string webpage = "<!DOCTYPE html><html lang=\"ru\">\n<style>\n.rss_ {\nborder-bottom: 1px solid #e6e6e6;\nmargin-top: 30px;\npadding-bottom: 20px;\n}\n.rss_head {\nmargin-bottom: 25px;\nmargin-top: 5px;\n}\n.rss_text {\nmargin-bottom: 25px;\nmargin-top: 5px;\n}\n.module-grid2 {\nposition: relative;\n}\ndiv {display: block;}\n</style>\n<head>\n<meta charset=\"UTF-8\">\n<title>RSS</title>\n</head>\n<body display=\"flex\">\n";


	for (int i = 0; i < list_of_items->size(); i++)
	{
		webpage += "<div class=\"rss_ module - grid2\" id=\"1rss\">\n<h2 class=\"rss_head\">\n";
		webpage += "<a href=\"" + (*list_of_items)[i][1] + "\"/>" + (*list_of_items)[i][0] + "</a></h2>\n";
		webpage += "<div class=\"rss_text\"><p>" + (*list_of_items)[i][2] + "</p></div></div>\n";
	}

	webpage += "</body></html>";

	return webpage;
}

int main(int argc, char** argv)
{
	setlocale(LC_ALL, "rus");

	string url;
	cin >> url;

	string request = constructGETRequest(&url);
	string answer = send_request(&url, &request);

	int head_end = answer.find("\r\n\r\n");
	int body_begin;
	if (head_end == -1) {
		head_end = answer.length();
		body_begin = head_end;
	}
	else {
		body_begin = head_end + 4;
	}

	string temp_str = answer.substr(0, head_end);
	writeToFile(directory_to_write + "head.txt", &temp_str);
	temp_str = answer.substr(body_begin);
	writeToFile(directory_to_write + "body.html", &temp_str);

	cout << endl << answer.substr(0, head_end) << endl << endl;

	vector<vector<string>> list_of_items;
	bool flag = get_list_of_items(answer, &list_of_items);

	string webpage = construct_webpage(&list_of_items);
	writeToFile(directory_to_write + "rss.html", &webpage);

	ShellExecuteA(NULL, "open", (directory_to_write + "rss.html").c_str(), NULL, directory_to_write.c_str(), SW_SHOWNORMAL);

	system("pause");
	return 0;
}

//http://forums.kuban.ru/external.php?type=RSS2
//http://stadium.ru/rss
/*
 * HTTP GET v0.2
 * Usage: ./http <host> <path>
 *
 * v0.1: 06/11/17
 *   Initial release
 * v0.2: 06/12/17
 *   Optimize reading content
 */

/*
struct hostent {
	char        *h_name;
	char        **h_aliases;
	int	        h_addrtype;
	int	        h_length;
	char        **h_addr_list;
struct sockaddr_in {                                                                
    __uint8_t   sin_len;                                                            
    sa_family_t sin_family;                                                         
    in_port_t   sin_port;                                                                                                            
    struct      in_addr sin_addr;                                                       
    char        sin_zero[8];                                                        
};
struct sockaddr {
	__uint8_t	sa_len;
	sa_family_t	sa_family;
	char		sa_data[14];
};
 */
#include <unistd.h>     // close
#include <sys/socket.h> // socket
#include <netdb.h>      // hostent, gethostbyname
#include <arpa/inet.h>  // htons
#include <iostream>
#include <string>
using namespace std;

void usage();
void error(string s);
int read_more(int sockfd, string& response);
void get_response(int sockfd, string& header, string& body);

int main(int argc, char** argv) {
    hostent* server;
    sockaddr_in serv_addr;
    int sockfd;
    int i = 0, j = 0, bytes = 0, cur = 0, total = 0;

    string host = "google.com";
    int port = 80;
    string path = "/";

    if (argc <= 1) {
        usage();
    }
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        path = argv[2];
    }

    // server addr and socket addr
    server = gethostbyname(host.c_str());
    if (!server) {
        error("resolve host");
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    cout << "*  Trying " << inet_ntoa(serv_addr.sin_addr) << "..." << endl;

    // socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));

	string request;
    request += "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "User-Agent: http\r\n";
    request += "Accept: */*\r\n";
    request += "\r\n";

    cur = 0;
    total = request.size();
    do {
        bytes = write(sockfd, &request[cur], total);
        if (bytes < 0) {
            error("request");
        }
        cur += bytes;
    } while (cur < total && bytes > 0);

    i = 0, j = 0;
    while (i < request.size()) {
        j = request.find("\r\n", i);
        if (j == request.npos) {
            j = request.size();
        }
        cout << "> " << request.substr(i, j - i) << endl;
        i = j + 2;
    }

    string header, body;
    get_response(sockfd, header, body);

    close(sockfd);
    return 0;
}

void usage() {
    printf("Usage: http <host> <path>\n");
    exit(1);
}

void error(string s) {
    s.insert(0, "ERROR: ");
    perror(s.c_str());
    exit(1);
}

int read_more(int sockfd, string& response) {
    string buf(256, 0);
    int bytes = read(sockfd, &buf[0], buf.size());
    if (bytes > 0) {
        response += buf.substr(0, bytes);
    }
    return bytes;
}

void get_response(int sockfd, string& header, string& body) {
    string response, line, transfer_encoding;
    int i = 1, j = 0, bytes = 0, header_size = 0;
    int content_length = -1, chunked_size = 0;
    // 0 reading header, 1 reading chunked size, 2 reading content
    int reading_state = 0;
    while (true) {
        if (reading_state == 2) {
            while (i < response.size() && i - j < chunked_size + 1) {
                ++i;
            }
        } else {
            while (i < response.size() && (response[i-1] != '\r' || response[i] != '\n')) {
                ++i;
            }
        }
        if (i >= response.size()) {
            if (read_more(sockfd, response) <= 0) {
                return;
            } else {
                continue;
            }
        }
        if (i == j + 1) {
            // end of header
            reading_state = 1;
            cout << "<" << endl;
            header_size = j = i = i + 1;
            if (content_length >= 0) {
                while (response.size() < header.size() + content_length) {
                    if (read_more(sockfd, response) <= 0) {
                        return;
                    }
                }
                body = response.substr(header_size);
                cout << body << endl;
                break;
            }
            continue;
        }
        line = response.substr(j, i - j - 1);
        if (reading_state == 0) {
            cout << "< " << line << endl;
            header += line + ";";
            if (content_length < 0 && transfer_encoding.empty()) {
                int pos = line.find("Content-Length");
                if (pos != line.npos) {
                    while (!isdigit(line[pos])) {
                        ++pos;
                    }
                    content_length = stoi(line.substr(pos));
                } else {
                    pos = line.find("Transfer-Encoding");
                    if (pos != line.npos) {
                        while (!isalpha(line[pos])) {
                            ++pos;
                        }
                        transfer_encoding = line.substr(pos);
                    }
                }
            }
        } else if (reading_state == 1) {
            chunked_size = stoi(line, NULL, 16);
            if (chunked_size == 0) {
                break;
            }
            reading_state = 2;
        } else {
            body += line;
            reading_state = 1;
            cout << line << endl;
        }
        // ready for next line
        j = i = i + 1;
    }
}

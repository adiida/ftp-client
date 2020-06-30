#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

//change the host order to network byte order (16bit)
void packi16(char * buf, unsigned short int i) { 
    i = htons(i);
    memcpy(buf, & i, 2);
}

int main() {
    boost::asio::io_service service;
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 21);
    boost::asio::ip::tcp::socket sock(service), data_sock(service);
    sock.open(boost::asio::ip::tcp::v4());
    sock.connect(ep);

    bool end = true;
    int iter = 0;
    std::string ip_address = "";
    std::string temp = "";
    unsigned short port = 0;
    unsigned short port_sec = 0;
    size_t i = 0;

    char buff[1024];
    sock.read_some(boost::asio::buffer(buff));
    std::cout << buff << std::endl;

    char user_name[] = "USER username\r\n";
    size_t user_name_size = (sizeof(user_name) / sizeof(*user_name)); // size of command variable
    sock.write_some(boost::asio::buffer(user_name, user_name_size - 1)); // write command in socket size-1 because \0 don't send in socket
    sock.read_some(boost::asio::buffer(buff)); // read answer from socket
    std::cout << buff << std::endl;

    char password[] = "PASS password\r\n";
    size_t password_size = (sizeof(password) / sizeof(*password));
    sock.write_some(boost::asio::buffer(password, password_size - 1));
    sock.read_some(boost::asio::buffer(buff));
    std::cout << buff << std::endl;

    char conn_type[] = "TYPE I\r\n";
    size_t conn_type_size = (sizeof(conn_type) / sizeof(*conn_type));
    sock.write_some(boost::asio::buffer(conn_type, conn_type_size - 1));
    sock.read_some(boost::asio::buffer(buff));
    std::cout << buff << std::endl;

    char ftp_mode[] = "PASV\r\n";
    size_t ftp_mode_size = (sizeof(ftp_mode) / sizeof(*ftp_mode));
    sock.write_some(boost::asio::buffer(ftp_mode, ftp_mode_size - 1));
    sock.read_some(boost::asio::buffer(buff));
    std::cout << buff << std::endl;

    i = 27; // position where ip address start
    while (end) {
        if (buff[i] == ',') {
            iter++;
            if (iter == 4) break;
            buff[i] = '.'; // if we find , we replace .
        }
        if (iter == 4) break;
        ip_address += buff[i]; // write in variable
        i++;
    }

    std::cout << "ip address" << ip_address << std::endl;
    while (true) {
        i++;
        if (buff[i] == ',' || buff[i] == ')') break;
        temp += buff[i]; // write port
    }

    port = boost::lexical_cast < unsigned int > (temp);
    temp = "";
    if (buff[i] == ',') { // if , exist then we have second part of port
        while (true) {
            i++;
            if (buff[i] == ')') break;
            temp += buff[i];
        }
    }

    port_sec = boost::lexical_cast < unsigned int > (temp);
    std::cout << "port " << port << std::endl;
    std::cout << "port sec " << port_sec << std::endl;

    port = (port << 8) + port_sec; // get port
    std::cout << "final port " << port << std::endl;
    data_sock.open(boost::asio::ip::tcp::v4());
    data_sock.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port)); // create new socket for get data

    char download_file_command[] = "RETR filename.ext\r\n";
    size_t download_file_command_size = (sizeof(download_file_command) / sizeof(*download_file_command));
    sock.write_some(boost::asio::buffer(download_file_command, download_file_command_size - 1));

    std::ofstream outf("./local_filename.ext", std::ios::binary);
    std::cout << "bytes available " << data_sock.available() << std::endl;
    size_t available_bytes = data_sock.available();
    char * recv_buf = new char[available_bytes];

    data_sock.read_some(boost::asio::buffer(recv_buf, available_bytes));
    while (i < available_bytes) {
        std::cout << recv_buf[i];
        outf << recv_buf[i];
        i++;
    }

    char shutdown_command[] = "QUIT\r\n";
    size_t shutdown_command_size = (sizeof(shutdown_command) / sizeof(*shutdown_command));
    sock.write_some(boost::asio::buffer(shutdown_command, shutdown_command_size - 1));
    sock.read_some(boost::asio::buffer(buff));
    std::cout << buff << std::endl;

    sock.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
    data_sock.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
    sock.close();
    data_sock.close();

    return 0;
}
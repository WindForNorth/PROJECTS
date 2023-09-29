#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>
const std::string ok_200type = "OK";
const std::string client_err_400type = "Bad Request";
const std::string client_err_400longtype = "Your Request has sytax error";
const std::string client_err_403type = "Forbidden";
const std::string client_err_403longtype = "You have not permission to access the source";
const std::string client_err_404type = "Not Found";
const std::string client_err_404longtype = "Server not found the source";
const std::string client_err_408type = "Request Timeout";
const std::string client_err_408longtype = "Request Timeout";
const std::string server_err_500type = "Internal Server Error";
const std::string server_err_500longtype = "A Server error happend";
const std::string http_version = "HTTP/1.1";
#endif
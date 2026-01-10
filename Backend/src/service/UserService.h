#ifndef USERSERVICE_H_
#define USERSERVICE_H_

#include <string>

class UserService {
public:
    static int Register(
        const std::string& user, const std::string& email,
        const std::string& passwd, const std::string& confirm,
        const std::string& varify_code);

        static int ResetPass(const std::string&name, const std::string& email, const std::string& new_pass, const std::string &varify_code);

};


#endif   // USERSERVICE_H_

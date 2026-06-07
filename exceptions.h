#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include<stdexcept>
#include<cstring>
#include<string>
#include "Comment.h"
using namespace std;

//-- ======= Dont cut off these libraries if you need any other put that just dont cut off these
//------- ================================================================================

class SocialException:public exception{
    public:
    string msg;
    SocialException(string m):msg(m){}
    const char* what() const noexcept override{return msg.c_str();}

};

class InvalidCredentialsEx : public SocialException {
public: InvalidCredentialsEx() : SocialException("Invalid username or password.") {}
};

class DuplicateUsernameEx : public SocialException{
    public:
    DuplicateUsernameEx(): SocialException("Username Already exists make a new one."){}
};

class FriendRequestEx : public SocialException {
public: FriendRequestEx() : SocialException("Friend request already sent.") {}
};

class PostTooLongEx : public SocialException {
public: PostTooLongEx() : SocialException("Post exceeeds characters limit.") {}
};

class FileNotFoundException : public SocialException {
public: FileNotFoundException(string f) : SocialException("File not found: " + f) {}
}; // takes filename as input


#endif
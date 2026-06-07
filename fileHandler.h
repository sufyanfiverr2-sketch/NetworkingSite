#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include<iostream>
#include <fstream>    // gives you ability to READ/WRITE files
#include <sstream>    // gives you ability to split a line into pieces
#include <vector>     // gives you a list that can grow
#include <string>
#include "exceptions.h"
#include "globalUp.h"
#include "Post.h"
 // gives you your custom errors





//static means you don't need to create a FileHandler object to use these functions. 
class FileHandler{
    public:
    static void writeLog(string message) { // log file is storing the activity of account.
    ofstream log("log.txt", ios::app);
    if (!log.is_open()) throw FileNotFoundException("log.txt");
    log << "[LOG] " << message << "\n";
    log.close();
    }
//store is a read-only reference to a list of Credential objects.
    static void saveUsers(const  vector<Credential>& store){
        ofstream log("users.txt");
        if(!log.is_open()) throw FileNotFoundException("users.txt");
        for (const Credential& c : store){
            log << c.username << "|" << c.email << "|"
                 << c.passwordHash << "|" << roleToString(c.role) << "\n";
        }
        log.close();


    }

    // ---- Load credentials from users.txt ----
    static void loadUsers(vector<Credential>& store) {
        ifstream file("users.txt");
        if (!file.is_open()) throw FileNotFoundException("users.txt");
        store.clear();
        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string uname, email, hashStr, roleStr;
            getline(ss, uname,   '|');
            getline(ss, email,   '|');
            getline(ss, hashStr, '|');
            getline(ss, roleStr, '|');
            Credential c;
            c.username     = uname;
            c.email        = email;
            c.passwordHash = stoul(hashStr);
            c.role         = stringToRole(roleStr);
            store.push_back(c);
        }
        file.close();
    }

    // ---- Save posts to posts.txt ----
    static void savePosts(Post posts[], int count) {
        ofstream file("posts.txt");
        if (!file.is_open()) throw FileNotFoundException("posts.txt");
        for (int i = 0; i < count; i++)
            posts[i].saveToFile(file);
        file.close();
        writeLog("Posts saved to posts.txt");
    }

};


#endif
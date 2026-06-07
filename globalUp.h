#ifndef GLOBALUP_H
#define GLOBALUP_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <conio.h>
#include "Comment.h"
#else
#include <termios.h>
#include <unistd.h>
#endif
#include "Post.h"
using namespace std;

extern Post postArray[];
extern int postCount;

unsigned long hashPassword(const string& password) {
    unsigned long pass = 5381;
    for (char c : password)
        pass = ((pass << 5) + pass) + c;
    return pass;
}


string getPasswordMasked() {
    string password = "";
    char ch;
#ifdef _WIN32
    while ((ch = _getch()) != '\r') {
        if (ch == '\b') {
            if (!password.empty()) { cout << "\b \b"; password.pop_back(); }
        } else { password += ch; cout << '*'; }
    }
#else
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    while ((ch = getchar()) != '\n') {
        if (ch == 127 || ch == '\b') {
            if (!password.empty()) { cout << "\b \b"; password.pop_back(); }
        } else { password += ch; cout << '*'; }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    cout << "\n";
    return password;
}


enum Role { GUEST, USER, ADMIN };

string roleToString(Role r) {
    if (r == ADMIN) return "ADMIN";
    if (r == USER)  return "USER";
    return "GUEST";
}

Role stringToRole(const string& s) {
    if (s == "ADMIN") return ADMIN;
    if (s == "USER")  return USER;
    return GUEST;
}


struct Credential {
    string        username;
    string        email;
    unsigned long passwordHash;
    Role          role;
};


vector<Credential> credentialStore;
const string USERS_FILE = "users.txt";



void saveAllToFile() {
    ofstream file(USERS_FILE);
    if (!file) { cout << "[Error] Could not open users.txt for writing.\n"; return; }
    for (const Credential& c : credentialStore)
        file << c.username << "|"
             << c.email    << "|"
             << c.passwordHash << "|"
             << roleToString(c.role) << "\n";
    file.close();
}

void loadFromFile() {
    ifstream file(USERS_FILE);
    if (!file) return;   

    credentialStore.clear();
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
        credentialStore.push_back(c);
    }
    file.close();
}


bool usernameExists(const string& username) {
    for (const Credential& c : credentialStore)
        if (c.username == username) return true;
    return false;
}

bool emailExists(const string& email) {
    for (const Credential& c : credentialStore)
        if (c.email == email) return true;
    return false;
}

bool findCredential(const string& username,
                    const string& email,
                    const string& password,
                    Role&         outRole) {
    unsigned long inputHash = hashPassword(password);
    for (const Credential& c : credentialStore) {
        if (c.username     == username &&
            c.email        == email    &&
            c.passwordHash == inputHash) {
            outRole = c.role;
            return true;
        }
    }
    return false;
}

void addCredential(const string& username,
                   const string& email,
                   const string& password,
                   Role          role) {
    Credential c;
    c.username     = username;
    c.email        = email;
    c.passwordHash = hashPassword(password);
    c.role         = role;
    credentialStore.push_back(c);
    saveAllToFile();
}

class Permission {
public:
    static bool canviewpost          (Role r) { return true; }
    static bool cansearch            (Role r) { return true; }
    static bool canpost              (Role r) { return r == USER || r == ADMIN; }
    static bool canLike              (Role r) { return r == USER || r == ADMIN; }
    static bool cancomment           (Role r) { return r == USER || r == ADMIN; }
    static bool canSendFriendRequest (Role r) { return r == USER || r == ADMIN; }
    static bool canDeletePost        (Role r) { return r == ADMIN; }
    static bool canDeleteAccount     (Role r) { return r == ADMIN; }
    static bool canDeleteUser        (Role r) { return r == ADMIN; }
    static bool canPromote           (Role r) { return r == ADMIN; }
    static bool canViewStats         (Role r) { return r == ADMIN; }
    static bool canCreateAdmin       (Role r) { return r == ADMIN; }
};

class User {
protected:
    string name;
    Role   role;
public:
    User(string n, Role r) : name(n), role(r) {}

    void viewpost() {
        if (!Permission::canviewpost(role)) {
            cout << name << " is not allowed to view posts\n";
            return;
        }
        if (postCount == 0) {
            cout << "No posts available.\n";
            return;
        }
        cout << "\n--- All Posts ---\n";
        for (int i = 0; i < postCount; i++) {
            cout << "[Post " << i << "]\n";
            postArray[i].displayPost();
        }
    }

    void search() {
        if (!Permission::cansearch(role)) {
            cout << name << " is not allowed to search\n";
            return;
        }
        if (postCount == 0) {
            cout << "No posts available.\n";
            return;
        }
        cin.ignore();
        string searchUsername;
        cout << "Enter username to search: ";
        getline(cin, searchUsername);
        
        bool found = false;
        cout << "\n--- Posts by " << searchUsername << " ---\n";
        for (int i = 0; i < postCount; i++) {
            if (postArray[i].getUsername() == searchUsername) {
                cout << "[Post " << i << "]\n";
                postArray[i].displayPost();
                found = true;
            }
        }
        if (!found) {
            cout << "No posts found by user: " << searchUsername << "\n";
        }
    }

    void createPost() {
        if (!Permission::canpost(role)) {
            cout << name << " is not allowed to create a post\n";
            return;
        }
        if (postCount >= 100) {
            cout << "Post limit reached. Cannot create more posts.\n";
            return;
        }
        cin.ignore();
        string caption;
        cout << "Enter post caption: ";
        getline(cin, caption);
        
        postArray[postCount] = Post(name, caption);
        postCount++;
        cout << "Post created successfully by " << name << "\n";
    }

    void likePost() {
        if (!Permission::canLike(role)) {
            cout << name << " is not allowed to like posts\n";
            return;
        }
        if (postCount == 0) {
            cout << "No posts available to like.\n";
            return;
        }
        cout << "\n--- Posts ---\n";
        for (int i = 0; i < postCount; i++) {
            cout << "[Post " << i << "]\n";
            postArray[i].displayPost();
        }
        int index;
        cout << "Enter post index to like: ";
        cin >> index;
        
        if (index >= 0 && index < postCount) {
            postArray[index].likePost();
            cout << name << " liked post " << index << "\n";
        } else {
            cout << "Invalid post index.\n";
        }
    }

    void commentPost() {
        if (!Permission::cancomment(role)) {
            cout << name << " is not allowed to comment\n";
            return;
        }
        if (postCount == 0) {
            cout << "No posts available to comment on.\n";
            return;
        }
        cout << "\n--- Posts ---\n";
        for (int i = 0; i < postCount; i++) {
            cout << "[Post " << i << "]\n";
            postArray[i].displayPost();
        }
        int index;
        cout << "Enter post index to comment on: ";
        cin >> index;
        
        if (index >= 0 && index < postCount) {
            cin.ignore();
            string commentText;
            cout << "Enter comment: ";
            getline(cin, commentText);
            postArray[index].addComment(name, commentText);
            cout << "Comment added successfully.\n";
        } else {
            cout << "Invalid post index.\n";
        }
    }

    void sendFriendRequest() {
        if (!Permission::canSendFriendRequest(role)) {
            cout << name << " is not allowed to send friend requests\n";
            return;
        }
        cin.ignore();
        string friendUsername;
        cout << "Enter username to send friend request: ";
        getline(cin, friendUsername);
        
        bool found = false;
        for (int i = 0; i < credentialStore.size(); i++) {
            if (credentialStore[i].username == friendUsername) {
                found = true;
                break;
            }
        }
        
        if (found) {
            cout << name << " sent a friend request to " << friendUsername << "\n";
        } else {
            cout << "User not found.\n";
        }
    }

    void deletePost() {
        if (!Permission::canDeletePost(role)) {
            cout << name << " is not allowed to delete posts\n";
            return;
        }
        if (postCount == 0) {
            cout << "No posts available to delete.\n";
            return;
        }
        cout << "\n--- Posts ---\n";
        for (int i = 0; i < postCount; i++) {
            cout << "[Post " << i << "]\n";
            postArray[i].displayPost();
        }
        int index;
        cout << "Enter post index to delete: ";
        cin >> index;
        
        if (index >= 0 && index < postCount) {
            for (int i = index; i < postCount - 1; i++) {
                postArray[i] = postArray[i + 1];
            }
            postCount--;
            cout << "Post deleted successfully.\n";
        } else {
            cout << "Invalid post index.\n";
        }
    }

    void DeleteAccount() {
        if (!Permission::canDeleteAccount(role)) {
            cout << name << " is not allowed to delete accounts\n";
            return;
        }
        cin.ignore();
        string userToDelete;
        cout << "Enter username to delete: ";
        getline(cin, userToDelete);
        
        for (int i = 0; i < credentialStore.size(); i++) {
            if (credentialStore[i].username == userToDelete) {
                credentialStore.erase(credentialStore.begin() + i);
                saveAllToFile();
                cout << "Account '" << userToDelete << "' deleted successfully.\n";
                return;
            }
        }
        cout << "User not found.\n";
    }

    void DeleteUser() {
        if (!Permission::canDeleteUser(role)) {
            cout << name << " is not allowed to delete users\n";
            return;
        }
        cin.ignore();
        string userToDelete;
        cout << "Enter username to delete: ";
        getline(cin, userToDelete);
        
        for (int i = 0; i < credentialStore.size(); i++) {
            if (credentialStore[i].username == userToDelete) {
                credentialStore.erase(credentialStore.begin() + i);
                saveAllToFile();
                cout << "User '" << userToDelete << "' deleted successfully.\n";
                return;
            }
        }
        cout << "User not found.\n";
    }

    void promoteUser() {
        if (!Permission::canPromote(role)) {
            cout << name << " is not allowed to promote users\n";
            return;
        }
        cin.ignore();
        string userToPromote;
        cout << "Enter username to promote to Admin: ";
        getline(cin, userToPromote);
        
        for (int i = 0; i < credentialStore.size(); i++) {
            if (credentialStore[i].username == userToPromote) {
                if (credentialStore[i].role == ADMIN) {
                    cout << "User is already an Admin.\n";
                    return;
                }
                credentialStore[i].role = ADMIN;
                saveAllToFile();
                cout << "User '" << userToPromote << "' promoted to Admin.\n";
                return;
            }
        }
        cout << "User not found.\n";
    }

    void ViewStats() {
        if (!Permission::canViewStats(role)) {
            cout << name << " is not allowed to view stats\n";
            return;
        }
        cout << "\n--- System Statistics ---\n";
        cout << "Total Users: " << credentialStore.size() << "\n";
        cout << "Total Posts: " << postCount << "\n";
    }

    void createAdminAccount() {
        if (!Permission::canCreateAdmin(role)) {
            cout << name << " is not allowed to create admin accounts\n";
            return;
        }
        string newUsername, newEmail, newPassword, confirmPassword;
        cin.ignore();

        cout << "\n--- Create New Admin Account ---\n";

        cout << "New admin username : ";
        getline(cin, newUsername);
        if (usernameExists(newUsername)) { cout << "Username already taken.\n"; return; }

        cout << "New admin email    : ";
        getline(cin, newEmail);
        if (emailExists(newEmail)) { cout << "Email already registered.\n"; return; }

        cout << "New admin password : ";
        newPassword = getPasswordMasked();

        cout << "Confirm password   : ";
        confirmPassword = getPasswordMasked();

        if (newPassword != confirmPassword) { cout << "Passwords do not match.\n"; return; }
        if (newPassword.length() < 6)       { cout << "Password must be at least 6 characters.\n"; return; }

        addCredential(newUsername, newEmail, newPassword, ADMIN);
        cout << "Admin account '" << newUsername << "' created and saved successfully.\n";
    }
};

class Guest       : public User { public: Guest()               : User("Guest", GUEST) {} };
class regularUser : public User { public: regularUser(string n)  : User(n,       USER)  {} };
class Admin       : public User { public: Admin(string n)        : User(n,       ADMIN) {} };


void userMenu(User& u, Role role) {
    int choice;
    do {
        cout << "\n----- MENU -----\n"
             << "1.  View Posts\n"
             << "2.  Search User\n"
             << "3.  Create Post\n"
             << "4.  Like Post\n"
             << "5.  Comment\n"
             << "6.  Send Friend Request\n"
             << "7.  Delete Post\n"
             << "8.  Delete Account\n"
             << "9.  Delete User\n"
             << "10. Promote User\n"
             << "11. View Stats\n";
        if (role == ADMIN)
             cout << "12. Create Admin Account\n";
        cout << "0.  Logout\n"
             << "Enter choice: ";
        cin >> choice;

        switch (choice) {
            case 1:  u.viewpost();           break;
            case 2:  u.search();             break;
            case 3:  u.createPost();         break;
            case 4:  u.likePost();           break;
            case 5:  u.commentPost();        break;
            case 6:  u.sendFriendRequest();  break;
            case 7:  u.deletePost();         break;
            case 8:  u.DeleteAccount();      break;
            case 9:  u.DeleteUser();         break;
            case 10: u.promoteUser();        break;
            case 11: u.ViewStats();          break;
            case 12:
                if (role == ADMIN) u.createAdminAccount();
                else cout << "Invalid choice\n";
                break;
            case 0:  cout << "Logging out...\n"; break;
            default: cout << "Invalid choice\n";
        }
    } while (choice != 0);
}


void registerUser() {
    string username, email, password, confirmPassword;
    cin.ignore();

    cout << "\n--- New User Registration ---\n";

    cout << "Choose a username  : ";
    getline(cin, username);
    if (username.empty())          { cout << "Username cannot be empty.\n";          return; }
    if (usernameExists(username))  { cout << "Username already taken.\n";            return; }

    cout << "Enter your email   : ";
    getline(cin, email);
    if (email.empty())             { cout << "Email cannot be empty.\n";             return; }
    if (emailExists(email))        { cout << "Email already registered.\n";          return; }

    cout << "Choose a password  : ";
    password = getPasswordMasked();
    if (password.length() < 6)    { cout << "Password must be at least 6 characters.\n"; return; }

    cout << "Confirm password   : ";
    confirmPassword = getPasswordMasked();
    if (password != confirmPassword) { cout << "Passwords do not match.\n";          return; }

    addCredential(username, email, password, USER);
    cout << "Registration successful! You can now log in as '" << username << "'.\n";
}


void loginAndEnter() {
    string username, email, password;
    cin.ignore();

    cout << "\n--- Login ---\n";
    cout << "Username : ";  getline(cin, username);
    cout << "Email    : ";  getline(cin, email);
    cout << "Password : ";  password = getPasswordMasked();

    Role role;
    if (!findCredential(username, email, password, role)) {
        cout << "Invalid credentials. Access denied.\n";
        return;
    }

    cout << "Login successful! Welcome, " << username << ".\n";

    if (role == ADMIN) { Admin a(username);       userMenu(a, ADMIN); }
    else               { regularUser u(username); userMenu(u, USER);  }
}

void firstTimeAdminSetup() {
    cout << "-----------------------------------------\n";
    cout << "   FIRST TIME SETUP - Create Super Admin\n";
    cout << "-----------------------------------------\n";
    cout << "No users found. You must create the first Admin account.\n\n";
    
    string username, email, password, confirmPassword;
    cin.ignore(); 
    
    cout << "Admin username : "; 
    getline(cin, username);
    if (username.empty()) { 
        cout << "Username cannot be empty.\n"; 
        exit(1); 
    }
    
    cout << "Admin email    : "; 
    getline(cin, email);
    if (email.empty()) { 
        cout << "Email cannot be empty.\n"; 
        exit(1); 
    }
    
    cout << "Admin password : "; 
    password = getPasswordMasked();
    if (password.length() < 6) { 
        cout << "Password must be at least 6 characters.\n"; 
        exit(1); 
    }
    
    cout << "Confirm        : "; 
    confirmPassword = getPasswordMasked();
    if (password != confirmPassword) { 
        cout << "Passwords do not match.\n"; 
        exit(1); 
    }
    
    addCredential(username, email, password, ADMIN);
    cout << "\nSuper Admin '" << username << "' created successfully!\n";
    cout << "You can now log in.\n";
    cout << "-----------------------------------------\n\n";
}

#endif

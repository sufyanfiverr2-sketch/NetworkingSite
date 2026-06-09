#ifndef GLOBALUP_H
#define GLOBALUP_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <cstdlib>
using namespace std;

// Forward declaration for Post
#include "Post.h"

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
    while ((ch = _getch()) != '\r' && ch != '\n') {
        if (ch == '\b') {
            if (!password.empty()) {
                cout << "\b \b";
                cout.flush();
                password.pop_back();
            }
        } else {
            password += ch;
            cout << '*';
            cout.flush();
        }
    }
#else
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    while ((ch = getchar()) != '\n') {
        if (ch == 127 || ch == '\b') {
            if (!password.empty()) {
                cout << "\b \b";
                cout.flush();
                password.pop_back();
            }
        } else {
            password += ch;
            cout << '*';
            cout.flush();
        }
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

// Friend Request struct
struct FriendRequest {
    string fromUser;
    string toUser;
    bool pending;
};

vector<Credential> credentialStore;
vector<FriendRequest> friendRequests;
const string USERS_FILE = "users.txt";
string currentUsername = "";

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

// ========== PRACTICAL FUNCTION 1: VIEW POSTS ==========
void viewPosts() {
    extern Post postArray[100];
    extern int postCount;
    
    if (postCount == 0) {
        cout << "\nNo posts available.\n";
        return;
    }
    
    cout << "\n========== ALL POSTS ==========\n";
    for (int i = 0; i < postCount; i++) {
        cout << "\n[Post #" << i << "]\n";
        postArray[i].displayPost();
    }
    cout << "================================\n";
}

// ========== PRACTICAL FUNCTION 2: SEARCH USER ==========
void searchUser() {
    string searchName;
    cin.ignore();
    
    cout << "\nEnter username to search: ";
    getline(cin, searchName);
    
    bool found = false;
    cout << "\n========== SEARCH RESULTS ==========\n";
    for (const Credential& c : credentialStore) {
        if (c.username == searchName) {
            cout << "Username: " << c.username << "\n";
            cout << "Email: " << c.email << "\n";
            cout << "Role: " << roleToString(c.role) << "\n";
            found = true;
            break;
        }
    }
    
    if (!found) {
        cout << "User '" << searchName << "' not found.\n";
    }
    cout << "===================================\n";
}

// ========== PRACTICAL FUNCTION 3: LIKE POST ==========
void likePost() {
    extern Post postArray[100];
    extern int postCount;
    
    if (postCount == 0) {
        cout << "\nNo posts to like.\n";
        return;
    }
    
    viewPosts();  // Show posts first
    
    int postIndex;
    cout << "\nEnter post number to like: ";
    cin >> postIndex;
    
    if (postIndex < 0 || postIndex >= postCount) {
        cout << "Invalid post number.\n";
        return;
    }
    
    postArray[postIndex].likePost();
    cout << "Post liked! New like count: " << postArray[postIndex].getLikes() << "\n";
}

// ========== PRACTICAL FUNCTION 4: COMMENT ON POST ==========
void commentOnPost() {
    extern Post postArray[100];
    extern int postCount;
    
    if (postCount == 0) {
        cout << "\nNo posts to comment on.\n";
        return;
    }
    
    viewPosts();  // Show posts first
    
    int postIndex;
    cout << "\nEnter post number to comment on: ";
    cin >> postIndex;
    
    if (postIndex < 0 || postIndex >= postCount) {
        cout << "Invalid post number.\n";
        return;
    }
    
    cin.ignore();
    cout << "Enter your comment (max 150 chars): ";
    string comment;
    getline(cin, comment);
    
    if (comment.length() > 150) {
        cout << "Comment too long. Max 150 characters.\n";
        return;
    }
    
    if (comment.empty()) {
        cout << "Comment cannot be empty.\n";
        return;
    }
    
    postArray[postIndex].addComment(currentUsername, comment);
    cout << "Comment added successfully!\n";
}

// ========== PRACTICAL FUNCTION 5: SEND FRIEND REQUEST ==========
void sendFriendRequest() {
    string targetUser;
    cin.ignore();
    
    cout << "\nEnter username to send friend request: ";
    getline(cin, targetUser);
    
    if (targetUser == currentUsername) {
        cout << "You cannot send friend request to yourself.\n";
        return;
    }
    
    if (!usernameExists(targetUser)) {
        cout << "User '" << targetUser << "' not found.\n";
        return;
    }
    
    // Check if already friends or request pending
    for (const FriendRequest& fr : friendRequests) {
        if ((fr.fromUser == currentUsername && fr.toUser == targetUser) ||
            (fr.fromUser == targetUser && fr.toUser == currentUsername)) {
            cout << "Friend request already exists or you are already friends.\n";
            return;
        }
    }
    
    FriendRequest fr;
    fr.fromUser = currentUsername;
    fr.toUser = targetUser;
    fr.pending = true;
    friendRequests.push_back(fr);
    
    cout << "Friend request sent to " << targetUser << "!\n";
}

// ========== PRACTICAL FUNCTION 6: DELETE POST (ADMIN) ==========
void deletePostAdmin() {
    extern Post postArray[100];
    extern int postCount;
    
    if (postCount == 0) {
        cout << "\nNo posts to delete.\n";
        return;
    }
    
    viewPosts();
    
    int postIndex;
    cout << "\nEnter post number to delete: ";
    cin >> postIndex;
    
    if (postIndex < 0 || postIndex >= postCount) {
        cout << "Invalid post number.\n";
        return;
    }
    
    // Shift posts left
    for (int i = postIndex; i < postCount - 1; i++) {
        postArray[i] = postArray[i + 1];
    }
    postCount--;
    
    cout << "Post deleted successfully!\n";
}

// ========== PRACTICAL FUNCTION 7: DELETE ACCOUNT (ADMIN) ==========
void deleteAccountAdmin() {
    string userToDelete;
    cin.ignore();
    
    cout << "\nEnter username to delete: ";
    getline(cin, userToDelete);
    
    if (userToDelete == currentUsername) {
        cout << "You cannot delete your own account.\n";
        return;
    }
    
    for (int i = 0; i < credentialStore.size(); i++) {
        if (credentialStore[i].username == userToDelete) {
            credentialStore.erase(credentialStore.begin() + i);
            saveAllToFile();
            cout << "User '" << userToDelete << "' deleted successfully!\n";
            return;
        }
    }
    
    cout << "User not found.\n";
}

// ========== PRACTICAL FUNCTION 8: DELETE USER (ADMIN) ==========
void deleteUser() {
    deleteAccountAdmin();  // Same functionality
}

// ========== PRACTICAL FUNCTION 9: PROMOTE USER TO ADMIN ==========
void promoteUser() {
    string userToPromote;
    cin.ignore();
    
    cout << "\nEnter username to promote to Admin: ";
    getline(cin, userToPromote);
    
    for (Credential& c : credentialStore) {
        if (c.username == userToPromote) {
            if (c.role == ADMIN) {
                cout << "User is already an Admin.\n";
                return;
            }
            c.role = ADMIN;
            saveAllToFile();
            cout << "User '" << userToPromote << "' promoted to Admin!\n";
            return;
        }
    }
    
    cout << "User not found.\n";
}

// ========== PRACTICAL FUNCTION 10: VIEW STATISTICS ==========
void viewStats() {
    extern Post postArray[100];
    extern int postCount;
    
    cout << "\n========== SYSTEM STATISTICS ==========\n";
    cout << "Total Users: " << credentialStore.size() << "\n";
    
    int adminCount = 0, userCount = 0, guestCount = 0;
    for (const Credential& c : credentialStore) {
        if (c.role == ADMIN) adminCount++;
        else if (c.role == USER) userCount++;
    }
    
    cout << "  - Admins: " << adminCount << "\n";
    cout << "  - Regular Users: " << userCount << "\n";
    cout << "Total Posts: " << postCount << "\n";
    cout << "Pending Friend Requests: " << friendRequests.size() << "\n";
    cout << "=====================================\n";
}

// ========== PRACTICAL FUNCTION 11: CREATE NEW POST ==========
void createNewPost() {
    if (currentUsername == "") {
        cout << "Error: No user logged in.\n";
        return;
    }
    
    string caption;
    cin.ignore();
    
    cout << "\n--- Create New Post ---\n";
    cout << "Enter your post (max 280 chars): ";
    getline(cin, caption);
    
    if (caption.length() > 280) {
        cout << "Post exceeds 280 characters. Not posted.\n";
        return;
    }
    
    if (caption.empty()) {
        cout << "Post cannot be empty.\n";
        return;
    }
    
    extern Post postArray[100];
    extern int postCount;
    
    if (postCount >= 100) {
        cout << "Post limit reached. Cannot create more posts.\n";
        return;
    }
    
    postArray[postCount] = Post(currentUsername, caption);
    postCount++;
    
    cout << "Post created successfully!\n";
}

class User {
protected:
    string name;
    Role   role;
public:
    User(string n, Role r) : name(n), role(r) {}

    void viewpost()          { viewPosts(); }
    void search()            { searchUser(); }
    void createPost()        { createNewPost(); }
    void likePost()          { ::likePost(); }
    void commentPost()       { commentOnPost(); }
    void sendFriendRequest() { ::sendFriendRequest(); }
    void deletePost()        { deletePostAdmin(); }
    void DeleteAccount()     { deleteAccountAdmin(); }
    void DeleteUser()        { deleteUser(); }
    void promoteUser()       { promoteUser(); }
    void ViewStats()         { viewStats(); }

    void createAdminAccount() {
        if (role != ADMIN) {
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
        cout << "\n----- MENU (Logged in as: " << currentUsername << ") -----\n"
             << "1.  View Posts\n"
             << "2.  Search User\n"
             << "3.  Create Post\n"
             << "4.  Like Post\n"
             << "5.  Comment on Post\n"
             << "6.  Send Friend Request\n";
        
        if (role == ADMIN) {
            cout << "7.  Delete Post\n"
                 << "8.  Delete Account\n"
                 << "9.  Delete User\n"
                 << "10. Promote User to Admin\n"
                 << "11. View Statistics\n"
                 << "12. Create Admin Account\n";
        }
        
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
            case 7:  
                if (role == ADMIN) u.deletePost();
                else cout << "Admin only function.\n";
                break;
            case 8:  
                if (role == ADMIN) u.DeleteAccount();
                else cout << "Admin only function.\n";
                break;
            case 9:  
                if (role == ADMIN) u.DeleteUser();
                else cout << "Admin only function.\n";
                break;
            case 10: 
                if (role == ADMIN) u.promoteUser();
                else cout << "Admin only function.\n";
                break;
            case 11: 
                if (role == ADMIN) u.ViewStats();
                else cout << "Admin only function.\n";
                break;
            case 12:
                if (role == ADMIN) u.createAdminAccount();
                else cout << "Invalid choice\n";
                break;
            case 0:  
                cout << "Logging out...\n"; 
                currentUsername = ""; 
                break;
            default: 
                cout << "Invalid choice\n";
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
    currentUsername = username;

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
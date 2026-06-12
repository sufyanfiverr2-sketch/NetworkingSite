#ifndef GLOBALUP_H
#define GLOBALUP_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <cstdlib>
#include "Post.h"
using namespace std;

// Forward declaration for Post
class Post;

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

struct FriendRequest {
    string fromUser;
    string toUser;
    bool pending;
};

struct Message {
    string fromUser;
    string toUser;
    string content;
    bool read;
    
    Message() : read(false) {}
    Message(string f, string t, string c) : fromUser(f), toUser(t), content(c), read(false) {}
};

vector<Credential> credentialStore;
vector<FriendRequest> friendRequests;
vector<Message> messageStore;
const string USERS_FILE = "users.txt";
const string FRIENDS_FILE = "friends.txt";      // ← NEW
const string MESSAGES_FILE = "messages.txt";    // ← NEW
string currentUsername = "";

// ========== NEW: SAVE FRIENDS TO FILE ==========
void saveFriendsToFile() {
    ofstream file(FRIENDS_FILE);
    if (!file) { 
        cout << "[Error] Could not open friends.txt for writing.\n"; 
        return; 
    }
    for (const FriendRequest& fr : friendRequests) {
        file << fr.fromUser << "|" 
             << fr.toUser << "|" 
             << (fr.pending ? "1" : "0") << "\n";
    }
    file.close();
}

// ========== NEW: LOAD FRIENDS FROM FILE ==========
void loadFriendsFromFile() {
    ifstream file(FRIENDS_FILE);
    if (!file) return;
    
    friendRequests.clear();
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string fromUser, toUser, pendingStr;
        
        getline(ss, fromUser, '|');
        getline(ss, toUser, '|');
        getline(ss, pendingStr, '|');
        
        FriendRequest fr;
        fr.fromUser = fromUser;
        fr.toUser = toUser;
        fr.pending = (pendingStr == "1");
        friendRequests.push_back(fr);
    }
    file.close();
}

// ========== NEW: SAVE MESSAGES TO FILE ==========
void saveMessagesToFile() {
    ofstream file(MESSAGES_FILE);
    if (!file) { 
        cout << "[Error] Could not open messages.txt for writing.\n"; 
        return; 
    }
    for (const Message& m : messageStore) {
        file << m.fromUser << "|" 
             << m.toUser << "|" 
             << m.content << "|" 
             << (m.read ? "1" : "0") << "\n";
    }
    file.close();
}

// ========== NEW: LOAD MESSAGES FROM FILE ==========
void loadMessagesFromFile() {
    ifstream file(MESSAGES_FILE);
    if (!file) return;
    
    messageStore.clear();
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string fromUser, toUser, content, readStr;
        
        getline(ss, fromUser, '|');
        getline(ss, toUser, '|');
        getline(ss, content, '|');
        getline(ss, readStr, '|');
        
        Message m;
        m.fromUser = fromUser;
        m.toUser = toUser;
        m.content = content;
        m.read = (readStr == "1");
        messageStore.push_back(m);
    }
    file.close();
}

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

bool areFriends(const string& user1, const string& user2) {
    for (const FriendRequest& fr : friendRequests) {
        if (!fr.pending) {
            if ((fr.fromUser == user1 && fr.toUser == user2) ||
                (fr.fromUser == user2 && fr.toUser == user1)) {
                return true;
            }
        }
    }
    return false;
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

void likePost() {
    extern Post postArray[100];
    extern int postCount;
    
    if (postCount == 0) {
        cout << "\nNo posts to like.\n";
        return;
    }
    
    viewPosts();
    
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

void commentOnPost() {
    extern Post postArray[100];
    extern int postCount;
    
    if (postCount == 0) {
        cout << "\nNo posts to comment on.\n";
        return;
    }
    
    viewPosts();
    
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
    
    for (const FriendRequest& fr : friendRequests) {
        if ((fr.fromUser == currentUsername && fr.toUser == targetUser && fr.pending) ||
            (fr.fromUser == targetUser && fr.toUser == currentUsername && fr.pending)) {
            cout << "Friend request already pending.\n";
            return;
        }
        if ((fr.fromUser == currentUsername && fr.toUser == targetUser && !fr.pending) ||
            (fr.fromUser == targetUser && fr.toUser == currentUsername && !fr.pending)) {
            cout << "You are already friends with " << targetUser << ".\n";
            return;
        }
    }
    
    FriendRequest fr;
    fr.fromUser = currentUsername;
    fr.toUser = targetUser;
    fr.pending = true;
    friendRequests.push_back(fr);
    saveFriendsToFile();  // ← SAVE immediately
    
    cout << "Friend request sent to " << targetUser << "!\n";
}

void viewIncomingRequests() {
    cout << "\n========== INCOMING FRIEND REQUESTS ==========\n";
    
    vector<int> pendingIndices;
    for (int i = 0; i < friendRequests.size(); i++) {
        if (friendRequests[i].toUser == currentUsername && friendRequests[i].pending) {
            cout << "[Request #" << pendingIndices.size() << "] From: " 
                 << friendRequests[i].fromUser << "\n";
            pendingIndices.push_back(i);
        }
    }
    
    if (pendingIndices.empty()) {
        cout << "No pending friend requests.\n";
        cout << "============================================\n";
        return;
    }
    
    cout << "============================================\n";
    
    int choice;
    cout << "\nEnter request number to handle (or -1 to skip): ";
    cin >> choice;
    
    if (choice == -1) return;
    
    if (choice < 0 || choice >= pendingIndices.size()) {
        cout << "Invalid request number.\n";
        return;
    }
    
    int requestIndex = pendingIndices[choice];
    
    cout << "Accept friend request from " << friendRequests[requestIndex].fromUser << "? (y/n): ";
    char response;
    cin >> response;
    
    if (response == 'y' || response == 'Y') {
        friendRequests[requestIndex].pending = false;
        saveFriendsToFile();  // ← SAVE immediately
        cout << "Friend request accepted! " << friendRequests[requestIndex].fromUser 
             << " is now your friend.\n";
    } else {
        friendRequests.erase(friendRequests.begin() + requestIndex);
        saveFriendsToFile();  // ← SAVE immediately
        cout << "Friend request rejected.\n";
    }
}

void viewFriends() {
    cout << "\n========== YOUR FRIENDS LIST ==========\n";
    
    vector<string> friends;
    for (const FriendRequest& fr : friendRequests) {
        if (!fr.pending) {
            if (fr.fromUser == currentUsername) {
                friends.push_back(fr.toUser);
            } else if (fr.toUser == currentUsername) {
                friends.push_back(fr.fromUser);
            }
        }
    }
    
    if (friends.empty()) {
        cout << "You have no friends yet.\n";
    } else {
        cout << "Total Friends: " << friends.size() << "\n";
        for (int i = 0; i < friends.size(); i++) {
            cout << (i + 1) << ". " << friends[i] << "\n";
        }
    }
    cout << "=======================================\n";
}

void sendMessage() {
    string recipientUser;
    cin.ignore();
    
    cout << "\n--- Send Message ---\n";
    cout << "Enter friend's username: ";
    getline(cin, recipientUser);
    
    if (recipientUser == currentUsername) {
        cout << "You cannot message yourself.\n";
        return;
    }
    
    if (!usernameExists(recipientUser)) {
        cout << "User '" << recipientUser << "' not found.\n";
        return;
    }
    
    if (!areFriends(currentUsername, recipientUser)) {
        cout << "You can only message your friends!\n";
        cout << "Add '" << recipientUser << "' as a friend first.\n";
        return;
    }
    
    cout << "Enter your message (max 500 chars): ";
    string messageText;
    getline(cin, messageText);
    
    if (messageText.length() > 500) {
        cout << "Message too long. Max 500 characters.\n";
        return;
    }
    
    if (messageText.empty()) {
        cout << "Message cannot be empty.\n";
        return;
    }
    
    Message msg(currentUsername, recipientUser, messageText);
    messageStore.push_back(msg);
    saveMessagesToFile();  // ← SAVE immediately
    
    cout << "Message sent to " << recipientUser << "!\n";
}

void viewInbox() {
    cout << "\n========== YOUR INBOX ==========\n";
    
    vector<int> messageIndices;
    int unreadCount = 0;
    
    for (int i = 0; i < messageStore.size(); i++) {
        if (messageStore[i].toUser == currentUsername) {
            messageIndices.push_back(i);
            if (!messageStore[i].read) unreadCount++;
        }
    }
    
    if (messageIndices.empty()) {
        cout << "No messages.\n";
        cout << "===============================\n";
        return;
    }
    
    cout << "Total Messages: " << messageIndices.size() << "\n";
    cout << "Unread: " << unreadCount << "\n\n";
    
    for (int j = 0; j < messageIndices.size(); j++) {
        int i = messageIndices[j];
        string status = messageStore[i].read ? "[Read]" : "[UNREAD]";
        cout << "[Message #" << j << "] " << status << " From: " << messageStore[i].fromUser << "\n";
    }
    
    cout << "===============================\n";
    
    cout << "\nEnter message number to read (or -1 to skip): ";
    int choice;
    cin >> choice;
    
    if (choice == -1) return;
    
    if (choice < 0 || choice >= messageIndices.size()) {
        cout << "Invalid message number.\n";
        return;
    }
    
    int msgIndex = messageIndices[choice];
    messageStore[msgIndex].read = true;
    saveMessagesToFile();  // ← SAVE immediately
    
    cout << "\n========== MESSAGE ==========\n";
    cout << "From: " << messageStore[msgIndex].fromUser << "\n";
    cout << "Message: " << messageStore[msgIndex].content << "\n";
    cout << "==============================\n";
    
    cout << "\nReply to " << messageStore[msgIndex].fromUser << "? (y/n): ";
    char response;
    cin >> response;
    
    if (response == 'y' || response == 'Y') {
        string recipientUser = messageStore[msgIndex].fromUser;
        cin.ignore();
        
        cout << "Enter your reply (max 500 chars): ";
        string replyText;
        getline(cin, replyText);
        
        if (replyText.length() > 500) {
            cout << "Message too long.\n";
            return;
        }
        
        if (replyText.empty()) {
            cout << "Message cannot be empty.\n";
            return;
        }
        
        Message reply(currentUsername, recipientUser, replyText);
        messageStore.push_back(reply);
        saveMessagesToFile();  // ← SAVE immediately
        
        cout << "Reply sent!\n";
    }
}

void viewConversation() {
    string friendName;
    cin.ignore();
    
    cout << "\nEnter friend's username to view conversation: ";
    getline(cin, friendName);
    
    if (friendName == currentUsername) {
        cout << "Cannot view conversation with yourself.\n";
        return;
    }
    
    if (!areFriends(currentUsername, friendName)) {
        cout << "You are not friends with " << friendName << ".\n";
        return;
    }
    
    cout << "\n========== CONVERSATION WITH " << friendName << " ==========\n";
    
    vector<Message> conversation;
    for (int i = 0; i < messageStore.size(); i++) {
        if ((messageStore[i].fromUser == currentUsername && messageStore[i].toUser == friendName) ||
            (messageStore[i].fromUser == friendName && messageStore[i].toUser == currentUsername)) {
            conversation.push_back(messageStore[i]);
            if (messageStore[i].toUser == currentUsername && !messageStore[i].read) {
                messageStore[i].read = true;  // Mark as read
            }
        }
    }
    
    if (conversation.empty()) {
        cout << "No messages with " << friendName << ".\n";
        cout << "=====================================\n";
        return;
    }
    
    for (const Message& m : conversation) {
        cout << m.fromUser << ": " << m.content << "\n";
    }
    
    cout << "=====================================\n";
    saveMessagesToFile();  // ← SAVE immediately
}

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
    
    for (int i = postIndex; i < postCount - 1; i++) {
        postArray[i] = postArray[i + 1];
    }
    postCount--;
    
    cout << "Post deleted successfully!\n";
}

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
            
            for (int j = friendRequests.size() - 1; j >= 0; j--) {
                if (friendRequests[j].fromUser == userToDelete || 
                    friendRequests[j].toUser == userToDelete) {
                    friendRequests.erase(friendRequests.begin() + j);
                }
            }
            
            for (int j = messageStore.size() - 1; j >= 0; j--) {
                if (messageStore[j].fromUser == userToDelete || 
                    messageStore[j].toUser == userToDelete) {
                    messageStore.erase(messageStore.begin() + j);
                }
            }
            
            saveAllToFile();
            saveFriendsToFile();     // ← SAVE
            saveMessagesToFile();    // ← SAVE
            cout << "User '" << userToDelete << "' deleted successfully!\n";
            return;
        }
    }
    
    cout << "User not found.\n";
}

void deleteUser() {
    deleteAccountAdmin();
}

void promoteUserAdmin() {
    string userToPromote;
    cin.ignore();
    
    cout << "\nEnter username to promote to Admin: ";
    getline(cin, userToPromote);
    
    if (userToPromote == currentUsername) {
        cout << "You are already an admin.\n";
        return;
    }
    
    for (Credential& c : credentialStore) {
        if (c.username == userToPromote) {
            if (c.role == ADMIN) {
                cout << "User is already an Admin.\n";
                return;
            }
            c.role = ADMIN;
            saveAllToFile();
            cout << "User '" << userToPromote << "' promoted to Admin successfully!\n";
            return;
        }
    }
    
    cout << "User not found.\n";
}

void viewStatsAdmin() {
    extern Post postArray[100];
    extern int postCount;
    
    cout << "\n========== SYSTEM STATISTICS ==========\n";
    cout << "Total Users: " << credentialStore.size() << "\n";
    
    int adminCount = 0, userCount = 0;
    for (const Credential& c : credentialStore) {
        if (c.role == ADMIN) adminCount++;
        else if (c.role == USER) userCount++;
    }
    
    cout << "  - Admins: " << adminCount << "\n";
    cout << "  - Regular Users: " << userCount << "\n";
    cout << "Total Posts: " << postCount << "\n";
    
    int pendingRequests = 0;
    for (const FriendRequest& fr : friendRequests) {
        if (fr.pending) pendingRequests++;
    }
    cout << "Pending Friend Requests: " << pendingRequests << "\n";
    cout << "Total Messages: " << messageStore.size() << "\n";
    cout << "=====================================\n";
}

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
    void commentPost()       { ::commentOnPost(); }
    void sendFriendRequest() { ::sendFriendRequest(); }
    void viewFriendRequests() { viewIncomingRequests(); }
    void viewMyFriends()     { viewFriends(); }
    void sendMsg()           { sendMessage(); }
    void viewMsg()           { viewInbox(); }
    void viewConvo()         { viewConversation(); }
    void deletePost()        { deletePostAdmin(); }
    void DeleteAccount()     { deleteAccountAdmin(); }
    void DeleteUser()        { deleteUser(); }
    void promoteUser()       { promoteUserAdmin(); }
    void ViewStats()         { viewStatsAdmin(); }

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
             << "6.  Send Friend Request\n"
             << "7.  View Friend Requests\n"
             << "8.  View Friends List\n"
             << "9.  Send Message to Friend\n"
             << "10. View Inbox\n"
             << "11. View Conversation\n";
        
        if (role == ADMIN) {
            cout << "12. Delete Post\n"
                 << "13. Delete Account\n"
                 << "14. Delete User\n"
                 << "15. Promote User to Admin\n"
                 << "16. View Statistics\n"
                 << "17. Create Admin Account\n";
        }
        
        cout << "0.  Logout\n"
             << "Enter choice: ";
        cin >> choice;

        switch (choice) {
            case 1:  u.viewpost();            break;
            case 2:  u.search();              break;
            case 3:  u.createPost();          break;
            case 4:  u.likePost();            break;
            case 5:  u.commentPost();         break;
            case 6:  u.sendFriendRequest();   break;
            case 7:  u.viewFriendRequests();  break;
            case 8:  u.viewMyFriends();       break;
            case 9:  u.sendMsg();             break;
            case 10: u.viewMsg();             break;
            case 11: u.viewConvo();           break;
            case 12: 
                if (role == ADMIN) u.deletePost();
                else cout << "Admin only function.\n";
                break;
            case 13: 
                if (role == ADMIN) u.DeleteAccount();
                else cout << "Admin only function.\n";
                break;
            case 14: 
                if (role == ADMIN) u.DeleteUser();
                else cout << "Admin only function.\n";
                break;
            case 15: 
                if (role == ADMIN) u.promoteUser();
                else cout << "Admin only function.\n";
                break;
            case 16: 
                if (role == ADMIN) u.ViewStats();
                else cout << "Admin only function.\n";
                break;
            case 17:
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
// main.cpp
#include "exceptions.h"
#include "fileHandler.h"
#include "Post.h"
#include <iostream>
#include <vector>
#include "globalUp.h"
using namespace std;

// Global post array
Post postArray[100];
int postCount = 0;

int main() {

//===============================================

    // STARTUP — load all files inside try/catch
    cout<<"---------------------------------------------------------------\n";
    cout << "===================  SOCIAL CONNECT APP  ===================\n";
    cout<<"---------------------------------------------------------------\n";

    try {
        FileHandler::loadUsers(credentialStore);
        cout << "[" << credentialStore.size() << " users loaded]\n";
    }
    catch (FileNotFoundException& e) {
        cout << e.what() << "\n";
        cout << "[Starting with empty user database]\n";
    }

    // ========== NEW: LOAD FRIENDS ==========
    try {
        loadFriendsFromFile();
        cout << "[" << friendRequests.size() << " friend connections loaded]\n";
    }
    catch (...) {
        cout << "[No friend data found, starting fresh]\n";
    }

    // ========== NEW: LOAD MESSAGES ==========
    try {
        loadMessagesFromFile();
        cout << "[" << messageStore.size() << " messages loaded]\n";
    }
    catch (...) {
        cout << "[No messages found, starting fresh]\n";
    }

    // CHECK IF NO USERS EXIST — FIRST TIME SETUP
    if (credentialStore.empty()) {
        cout << "\n";
        firstTimeAdminSetup();
    }

    // Load posts (optional)
    try {
        // loadPosts(postArray, postCount);
    }
    catch (FileNotFoundException& e) {
        cout << "[No posts file found, starting fresh]\n";
    }

    // MAIN MENU LOOP
    int choice;
    do {
        cout << "\n----- SOCIAL NETWORK SITE -----\n"
             << "1. Enter as Guest\n"
             << "2. Register\n"
             << "3. Login\n"
             << "0. Exit\n"
             << "Choice: ";
        cin >> choice;

        try {
            switch (choice) {
                case 1: { Guest g; userMenu(g, GUEST); break; }
                case 2: { registerUser();               break; }
                case 3: { loginAndEnter();              break; }
                case 0:   cout << "Goodbye!\n";         break;
                default:  cout << "Invalid choice.\n";
            }
        }
        catch (InvalidCredentialsEx& e)  { cout << e.what() << "\n"; }
        catch (DuplicateUsernameEx& e)   { cout << e.what() << "\n"; }
        catch (FileNotFoundException& e) { cout << e.what() << "\n"; }
        catch (SocialException& e)       { cout << e.what() << "\n"; }
        catch (exception& e)             { cout << "Error: " << e.what() << "\n"; }

    } while (choice != 0);

    // SHUTDOWN — save everything
    try {
        FileHandler::saveUsers(credentialStore);
        FileHandler::savePosts(postArray, postCount);
        saveFriendsToFile();           // ← NEW: SAVE FRIENDS
        saveMessagesToFile();          // ← NEW: SAVE MESSAGES
        FileHandler::writeLog("Program exited cleanly.");
        cout << "[All data saved successfully]\n";
    }
    catch (FileNotFoundException& e) {
        cout << "Save error: " << e.what() << "\n";
    }

    return 0;
}
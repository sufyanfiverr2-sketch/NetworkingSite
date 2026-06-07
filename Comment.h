#ifndef COMMENT_H
#define COMMENT_H

#include<iostream>
#include<string>
#include<fstream>
using namespace std;

class Comment {
private:
    string username;
    string text;
public:
Comment(){
    username = "";
    text = "";
    }
Comment(string u, string t){
    username = u;
    text = t;
}

void inputComment(string u) {
    username = u;
    cout << "Enter comment: ";
    getline(cin, text);
}

void displayComment(){
    cout << username << ": " << text << endl;

}

string getUsername() {
    return username;
}

string getText() {
    return text;
}

void saveToFile(ofstream& out) {
    out << username << "|" << text << "\n";
}
};
#endif // COMMENT_H
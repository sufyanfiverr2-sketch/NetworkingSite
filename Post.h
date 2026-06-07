#ifndef POST_H
#define POST_H

#include<fstream>
#include<iostream>
#include<string>
#include "Comment.h"
using namespace std;

class Post {
private:
string username;
string caption;
int likes;

Comment comment[10];
int commentCount;

public:
Post(){
    username = "";
    caption = "";
    likes = 0;
    commentCount = 0;
}

Post(string u, string c){
    username = u;
    caption = c;
    likes = 0;
    commentCount = 0;
}

void likePost(){
    likes++;
}

void addComment(string user , string text){
    if(commentCount < 10){
        comment[commentCount] = Comment(user, text);
        commentCount++;
    } else {
        cout << "Comment limit reached!" << endl;
    }

}
void displayPost(){
    cout << "Username: " << username << endl;
    cout << "Caption: " << caption << endl;
    cout << "Likes: " << likes << endl;
    cout << "Comments: " << endl;
    for(int i = 0; i < commentCount; i++){
        comment[i].displayComment();
    }
cout<<"-----------------------------"<<endl;
}

string getUsername() {
    return username;
}

void saveToFile(ofstream& outFile) {
    outFile << username << "\n";
    outFile << caption << "\n";
    outFile << likes << "\n";
    outFile << commentCount << "\n";
    for (int i = 0; i < commentCount; i++)
        comment[i].saveToFile(outFile);  // now saves real comment data
}

Post operator+(Post P){
    Post temp;
    temp.caption = this->caption + " " + P.caption;
    return temp;
}


};


#endif // POST_H
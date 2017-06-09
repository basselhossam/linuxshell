#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <cstring>
#include <signal.h>
#include <dirent.h>
#include <termios.h>
using namespace std;

#define CHECK(x) if(!(x)) { perror(#x " failed"); abort(); }

vector<string> arguments;
int ParentPID;
bool intsig = false;
vector<string> samefiles;
string data;
vector<string> CommandHistory;

void inthandler(int signum){
	if(ParentPID == getpid()){
		printf("^C\n");
		intsig = true;
	}
	else
		raise(SIGKILL);
}

void sigchld_hdl (int sig){
	/* Wait for all dead processes.
	 * We use a non-blocking call to be sure this signal handler will not
	 * block if a child was cleaned up in another part of the program. */
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	}
}

string Initialize(char *hostname, int size = HOST_NAME_MAX) {
	string s1(getlogin()); //de ele eshta3alet :D
	int x = gethostname(hostname, size);
	string s2(hostname);
	return s1+"@"+s2+":";
}

char**ConvertStringToChar() {
	char**myargs = new char*[arguments.size()+1];
	myargs[arguments.size()] = NULL;
	for (int i = 0; i < arguments.size(); i++)
	{
		myargs[i] = new char [arguments[i].length()];
		strcpy(myargs[i], const_cast<char*>(arguments[i].c_str()));
	}
	return myargs;
}

string getPath() {
	char Path[1024]; int size2;
	getcwd(Path,sizeof(Path)); //current directory
	string s3(Path);
	return s3;//Required: username@hostname:path
}

//list the files and folders that match the prefix sent in that path
bool getfiles(string path,string pre){
	bool s = false;
	samefiles.clear();
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (path.c_str())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			if (ent->d_type == DT_REG || ent->d_type == DT_DIR){
				string temp = ent->d_name;
				if(pre == temp.substr(0,pre.length())){
					s = true;
					samefiles.push_back(temp);
				}
			}
		}
		closedir (dir);
	} else {
		/* could not open directory */
		return false;
	}
	return s;
}

//function that read char from keyboard whitout wait for enter
int getch(void){ 
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

//gets the new path from a start path using cd commands like (../ or dirname/)
string findnewpath(string startpath,string comm){
	vector<string>paths;
	int start = 0;
	for(int i=0;i<startpath.length();i++){
		if(startpath[i] == '/'){
			paths.push_back(startpath.substr(start,i-start));
			start = i+1;
		}
	}
	string temp;
	for(int i = 0;i<comm.length();i++){
		temp += comm[i];
		if(comm[i] == '/'){
			paths.push_back(temp);
			temp = "";
		}
		else if(temp == ".."){
			paths.pop_back();
			temp = "";
			i++;
		}	
	}
	string finalpath;
	for(int i=0;i<paths.size();i++){
		finalpath += paths[i]+'/';
	}
	if(finalpath[finalpath.length()-2] == '/')
		finalpath = finalpath.substr(0,finalpath.length()-1);
	return finalpath;
}

//Read the command char by char and check for (tab - enter - arrow keys - backspace)
string ReadCommand() {
	int mypointer = CommandHistory.size();
	string p = getPath();	//first get the path and print it
	printf("%s$ ",p.c_str());
	string temp;
	char x;
	bool cdcom = false;		//indecator if the command starts with ./ so i can search for file on tap press
	bool runcom = false;	//indecator if the command starts with ./ so i can search for file on tap press
	int point = 0;	//points where the cursor on the string
	do{	//we get chars util the enter is pressed
		int tx = getch(); //acsii
		x = (char) tx;	//the actual char
		//handling arrow keys it pushes 3 chars starting with \033
		if(tx == '\033') { // if the first value is esc
			getch(); // skip the [
			switch(getch()) { // the real value
				case 'A':// code for arrow up
					for(point;point < temp.length();point++){	//first put the cursor to the end of the string
						printf("%c",temp[point]);
					}
					if(mypointer > 0) {
						for(int i=0;i<temp.length();i++)
							printf("\b \b");
						mypointer--;
						temp = CommandHistory[mypointer];
						point = temp.length();
						printf("%s",temp.c_str());
					}
					break;
				case 'B':// code for arrow down
					for(point;point < temp.length();point++){	//first put the cursor to the end of the string
						printf("%c",temp[point]);
					}
					if(mypointer==CommandHistory.size()-1) {
						for(int i=0;i<temp.length();i++)
							printf("\b \b");
						mypointer++;
						temp = "";
						point = 0;
					}
					else if(mypointer < CommandHistory.size()-1) {
						for(int i=0;i<temp.length();i++)
							printf("\b \b");
						mypointer++;
						temp = CommandHistory[mypointer];
						point = temp.length();
						printf("%s",temp.c_str());
					}
					break;
				case 'C':// code for arrow right
					if(point < temp.size()){	//check if I'm not at the end of the string
						printf("%c",temp[point]);	//print the char again give the illusion of moving the cursor :D
						point++;	//move the cursor right
					}
					break;
				case 'D':// code for arrow left
					if(point >0){	//check if I'm at the beginning of the string
						point--;	//move the cursor to the left
						printf("\b");	//by printing backspace the cursor moves to the left :D
					}
					break;
			}
		}
		else if(x == '\t'){	//check for the tab
			string added;
			if(getfiles(p,temp)){	//if there are matches in the hole command
				for(point;point < temp.length();point++){	//first put the cursor to the end of the string
					printf("%c",temp[point]);
				}
				if(samefiles.size() == 1){	//if there is only one match then add it to the string and complete the word
					added = samefiles[0].substr(temp.length(),samefiles[0].length());
					temp += added;
					printf("%s",added.c_str());
					point+=added.length();
				}
				else{	//if there are multible matches first sort the vector and then print them
					sort(samefiles.begin(),samefiles.end());
					for(int i=0;i<samefiles.size();i++){
						printf("\n%s",samefiles[i].c_str());
					}
					printf("\n%s%s$ %s",data.c_str(),p.c_str(),temp.c_str());	//print the entered string to resume typing
				}
			}
			else if(cdcom){	//if there are matches after the cd command assuming the command is like [cd (dir)] with single space after cd
				int startpoint = temp.length();	//to enable multitap each time I get a complete path I begin search in that path not the original
				for(startpoint;startpoint>3;startpoint--){
					if(temp[startpoint] == '/'){
						startpoint++;
						break;
					}
				}
				string newpath = findnewpath(p+"/",temp.substr(3,temp.length()));
				//printf("\n%s",newpath.c_str());
				if(getfiles(newpath,temp.substr(startpoint,temp.length()))){	//match the string after 3rd char (the space)
					for(point;point < temp.length();point++){
						printf("%c",temp[point]);
					}
					if(samefiles.size() == 1){
						added = samefiles[0].substr(temp.length()-startpoint,samefiles[0].length());
						temp += added;
						point+=added.length();
						printf("%s",added.c_str());
					}
					else{
						sort(samefiles.begin(),samefiles.end());
						for(int i=0;i<samefiles.size();i++){
							printf("\n%s",samefiles[i].c_str());
						}
						printf("\n%s%s$ %s",data.c_str(),p.c_str(),temp.c_str());
					}
				}
			}else if(runcom){	//same as above but now 2 chars (./)
				if(getfiles(p,temp.substr(2,temp.length()))){
					for(point;point < temp.length();point++){
						printf("%c",temp[point]);
					}
					if(samefiles.size() == 1){
						added = samefiles[0].substr(temp.length()-2,samefiles[0].length());
						temp += added;
						point+=added.length();
						printf("%s",added.c_str());
					}
					else{
						sort(samefiles.begin(),samefiles.end());
						for(int i=0;i<samefiles.size();i++){
							printf("\n%s",samefiles[i].c_str());
						}
						printf("\n%s$ %s",p.c_str(),temp.c_str());
					}
				}
			}
		}
		else if(tx == 127 || tx == 8){	//check for the backspace
			if(temp.length()>0 && point == temp.length()){
				printf("\b \b");	//first we move the cursor to the left then put a space and then to the left again
				temp = temp.substr(0,temp.length()-1);	//remove the last char
				point--;	//decrease the pointer
			}
		}
		else if(x != '\n'){		//if it's usual char
			if(point < temp.size()){	//if the cursor in certen postion so we replace that char
				printf("%c",x);
				temp[point] = x;
				point++;
			}
			else{	//else append the string
				printf("%c",x);
				temp += x;
				if(temp == "cd ")
					cdcom = true;
				else if(temp == "./")
					runcom = true;
				point++;	//increase the pointer
			}
		}
		else{
			printf("\n");	//enter is pressed so we enter endl
		}
		if(temp.length() <= 2 && temp!="cd")	//if we delete and the string don't have cd so we make it false
			cdcom = false;
		else if(temp.length() <= 2 && temp!="./")
			runcom = false;
	}while(x != '\n' && !intsig);
	temp += ' ';
	if(intsig)
		return "\n";
	return temp;
}

int ChangeDirectory(char *x) {
	return chdir(x);
}

char** ParseCommand(string input) {
	string temp = "";
	for (int i = 0; i < input.length(); i++)
	{
		if(input[i] != ' ') {
			temp += input[i];
		}
		else {
			arguments.push_back(temp);
			temp="";
		}
	}
	//here we have a vector of words that is the arguments of our command

	//Parsing
	//Second argument in execvp is array of char pointers so we need to convert the vector of strings into char
	return ConvertStringToChar();
}

bool ExecuteCommand(char**arg,bool background) {
	sigset_t mask, omask;
	if(background){
		/* Block SIGINT. */
		sigemptyset(&mask);
		sigaddset(&mask, SIGINT);
		CHECK(sigprocmask(SIG_BLOCK, &mask, &omask) == 0);
	}
	pid_t pid, wpid;
	int status;
	pid = fork();
	if (pid == 0) {
		// Child process
		if(background)
			CHECK(setpgid(0, 0) == 0);	//change process group so ctrl c don't terminate it
		if(execvp(arg[0],arg) == -1) {
			perror("ERROR");
		}
		exit(EXIT_FAILURE);
	} 
	else if (pid < 0) {
		// Error forking
		perror("ERROR");
	} 
	else {
		// Parent process
		if(background){
			if (setpgid(pid, pid) < 0 && errno != EACCES)
				abort();
			/* Unblock SIGINT */
			CHECK(sigprocmask(SIG_SETMASK, &omask, NULL) == 0);
		}
		else{
			do {
				wpid = waitpid(pid, &status, WUNTRACED);
			} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		}
		for(int i=0;i<arguments.size();i++)
			delete[] arg[i];
	}
	return true;
}

int main () {
	ParentPID = getpid();
	signal(SIGINT, inthandler);
	signal(SIGCHLD, sigchld_hdl);
	char hostname[HOST_NAME_MAX + 1];
	data = Initialize(hostname);
	while(1){
		printf("\n%s",data.c_str());
		string In = ReadCommand();
		if(In.length() <= 1){
			intsig = false;
			continue;
		}
		//to terminate the custom shell and back to original linux shell
		if(In == "end shell "){
			printf("Mashi khalsana be shiaka\n");
			raise(SIGKILL);
		}
		CommandHistory.push_back(In.substr(0,In.length()-1));
		string temp = In.substr(0,2);
		char**C=ParseCommand(In);
		if(temp == "cd"){
			if(ChangeDirectory(C[1])!=0){
				int err = errno;
				printf("Error: ");
				if(err == ENOENT)
					printf("The directory specified in path does not exist.\n");
				else if(err == EACCES)
					printf("Search permission is denied for one of the components of path.\n");
				else
					printf("Cannot change the path\n");
			}
		}
		else {
			ExecuteCommand(C,In[In.length()-2] == '&');
		}
		arguments.clear();
	}
	return 0;
}

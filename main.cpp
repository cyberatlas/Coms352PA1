/**
 * 12 Oct 2019
 * The main file that has all of the functions and runs the shell
 */
#include <sys/user.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <zconf.h>
#include <cstring>
#include <wait.h>
#include <algorithm>
#include <fcntl.h>
#include <fstream>


using namespace std;

/**
 *  Defining globals here so they can be modified anywhere
 */
//Holds the current working directory
char cwd[PATH_MAX];

// Checks if shell is still alive
bool stillAlive;


/** Takes in a string vector and executes command
 * Makes the args for it and executes it
 * Exits when done
  */
void command(vector<string> user_cms) {

    // extra room for program name and sentinel
    const char **user_in_arr = new const char *[user_cms.size() + 2];

    // by convention, argv[0] is program name
    char *command = const_cast<char *>(user_cms[0].c_str());

    int size = user_cms.size();
    // copy args
    for (int j = 0; j < size + 1; ++j) {
        user_in_arr[j] = user_cms[j].c_str();
    }

    user_in_arr[size+1] = NULL;
    execvp(command, (char **) user_in_arr);
}

/**
 * Parses input string
 * @param input - the input string to be parsed
 * @param delim - the delimiter on which to parse
 * @return vector of the the strings that were parsed
 */
vector<string> parser(string input, char delim) {
    vector<string> tempVector;
    tempVector.reserve(input.size());

    // Replace any tabs or new lines in the string with a space
    std::replace(input.begin(), input.end(), '\t', ' ');
    std::replace(input.begin(), input.end(), '\r', ' ');
    std::replace(input.begin(), input.end(), '\n', ' ');

    // String input stream of userInput
    istringstream ss(input);
    // Temp string to hold next word
    string tmpStrToAdd;
    // Loop through the input and save each section based on the delimeter
    while (getline(ss, tmpStrToAdd, delim)) {
        // We do not want to add null strings
        if (tmpStrToAdd == "") { continue; }
        tempVector.push_back(tmpStrToAdd);
    }
    // Return the string
    return tempVector;
}

/*
// Iterator to go through the string vector, outputs each string, used for testing purposes
void outputVectorTest(std::vector<std::string> vectorin) {
    std::cout << "Outputting the strings in our vector: \n";
    for (std::vector<std::string>::const_iterator i = vectorin.begin(); i != vectorin.end(); ++i)
        std::cout << "str " << *i << " \n";
}
*/

/**
 * Prints out the current working directory and the shell prompt
 * @return the user iput line
 */
string getUserInput() {
    string tempInput;
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, "/myshell");
    setenv("parent", cwd, 1);
    cout << "\n" << cwd << " =>: ";
    // Get user input
    getline(cin, tempInput);
    // Add a new line for cleanliness
    cout << "\n";
    return tempInput;
}

/**
 * Checks for redirect characters
 * @param rd the character to check
 * @return whether it was one or not
 */
bool redir_chars(char rd){
    switch(rd){
        case '<':
        case '>':
            return true;
        default:
            return false;
    }
}
/**
 * Runs the process from the vector string
 * @param inVect the vector string of the command
 * @param toWait whether or not to wait for the child process to return
 * @param toFork whether or not to fork the process
 */
void fgProcess(vector<string> inVect, bool toWait, bool toFork) {

    if (inVect.back() == "&"){
        toWait = false;
        inVect.front() = inVect.back();
    }
    else if (inVect.front() == "&"){
        toWait = false;
    }
    bool toRedirect = false;
    bool app = false;
    string inFile = "";
    string outFile = "";
    for(int j = 0; j < inVect.size(); j++){
        if((inVect[j]=="<<") || (inVect[j] == ">") || (inVect[j] == "<")){
            toRedirect = true;
        }
    }
    if(toRedirect){
        for(int i = 0; i < inVect.size();i++){
            // looking for redirects in.
            if(inVect[i] == "<"){
                inFile = inVect[i + 1];
                inVect.erase(inVect.begin()+i);
                inVect.erase(inVect.begin()+i);
                i--;
            }
            if((inVect[i] == ">")){
                outFile = inVect[i + 1];
                inVect.erase(inVect.begin()+i);
                inVect.erase(inVect.begin()+i);
                i--;
            } else if((inVect[i] == ">>")){
                app = true;
                outFile = inVect[i + 1];
                inVect.erase(inVect.begin()+i);
                inVect.erase(inVect.begin()+i);
                i--;
            }
        }
    }


    int inVectSize = inVect.size();

    // Happens when nothing is entered
    if (inVectSize == 0) {
        return;
    }
    // There are no switches on strings unfortuantely
    if (strcmp(inVect[0].c_str(), "quit") == 0) {
        // TODO do I need to kill any child processes?
        stillAlive = false;
        return;
    }
        // If the user only entered a space
    else if (strcmp(inVect[0].c_str(), " ") == 0) {
        return;
    }
        // If the user just pressed enter and that somehow made it past the first check
    else if (strcmp(inVect[0].c_str(), "\r") == 0) {
        return;
    }
    else if (strcmp(inVect[0].c_str(), "environ") == 0) {
        // Check if there are args - it takes no args
        if (inVect.size() > 1) {
            cout << "ERR environ TAKES NO ARGUMENTS \n";
            return;
        }
        // Remaps it to printenv sys call
        inVect[0] = "printenv";
    }
    else if (inVect[0] == "clr") {
        if (inVectSize > 1) {
            cout << "ERR clr TAKES NO ARGUMENTS \n";
            return;
        }
        //Remaps to a sys call`
        inVect[0] = "clear";
    }
    else if (inVect[0] == "cd") {
        // If the directory does not it throws an error
        // if == -1 then does not exist, otherwise change
        // If the directory argument is not present report the current working directory
        if (inVectSize == 1) {
            cout << cwd << "\n";
            return;
        }
        // Make appropriate size c string to hold the new cwd
        char *temp = static_cast<char *>(malloc(strlen(cwd) + strlen(inVect[1].c_str()) + 1));
        strcpy(temp, cwd);
        strcat(temp, "/");
        strcat(temp, inVect[1].c_str());
        chdir(inVect[1].c_str());
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));
        setenv("PWD", cwd, 1);
        return;
    }

    // According to the spec it takes no args
    else if (inVect[0] == "help") {
        if (inVectSize > 1) {
            cout << "ERR help TAKES NO ARGUMENTS \n";
            return;
        }
        inVect = {"man", "man"};
    }
    else if (inVect[0] == "time") {
        // Abdullah said to use this as output
        cout << "Arbitrary Error! - advised from TAA.\n";
    }
    else if (inVect[0] == "pause") {
        // Takes no args according to spec
        if (inVectSize > 1) {
            cout << "ERR help TAKES NO ARGUMENTS \n";
            return;
        }
        // Read -r should just wait until user inputs enter
        inVect={"read", "-r"};
    }

    // Execute if you need to fork
    if (toFork) {

        pid_t pid = fork();
        // Deal with errors
        if (pid < 0) {
            cout << "ERR COULD NOT CREATE NEW PROCESS";
            return;
        }
        // If in the child process
        else if (pid == 0) {
            if(inFile.compare("") != 0){
                int ifd;
                if(!(ifd = open(inFile.c_str(), O_RDWR) < 0)){
                    dup2(ifd,STDIN_FILENO);
                    close(ifd);
                }
            }
            if(outFile.compare("") != 0){
                int ofd;
                if(!app){
                    ofd = open(outFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
                    dup2(ofd,STDOUT_FILENO);
                    close(ofd);
                } else{
                    ofd = open(outFile.c_str(), O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
                    dup2(ofd,STDOUT_FILENO);
                    close(ofd);
                }
            }
            command(inVect);
        }

        if (toWait) {
            // Waits until child process has finished executing
            waitpid(pid, NULL, 0);
        }
    }
    if (!toFork){
        command(inVect);
    }

}


/**
 * Read in old fds change them to be stdin and close them
 * Run the command
 * @param fdin file descriptor for input
 * @param fdout file descriptor for output
 * @param cmd_in string vector of the command to be executed
 */
void changeFDsAndRun(int fdin, int fdout, vector<string>cmd_in){
    pid_t pid ;
    pid = fork();
    if (pid == 0) {
        // Change fd in to STDIN
        if (fdin != STDIN_FILENO) {
            dup2(fdin, STDIN_FILENO);
            close(fdin);
        }
        // Change fdout to STDOUT
        if (fdout != STDOUT_FILENO) {
            dup2(fdout, STDOUT_FILENO);
            close(fdout);
        }
//        outputVectorTest(cmd_in);
        fgProcess(cmd_in, false, false);
    }
}
/**
 * Handles the pipes when they exist
 * @param args string vector of commands that were separated by pipes
 */
void pipeHandler(vector<string> incmds) {
        int incmdsize = incmds.size();
        // pid of the process
        pid_t pid;
        // pid of the fork
        pid = fork();
        // If in the child
        if(pid == 0) {
            int pipefd[2];
            int fdin = 0;
            unsigned int i;

            for(i = 0; i < incmdsize - 1; ++i) {
                cout << "incmd: " << incmds[i] << "\n";
                pipe(pipefd);
                // Input file descriptor, output file descriptior, commands
                changeFDsAndRun(fdin, pipefd[1], (parser(incmds[i], ' ')));
                // Close the pipe
                close(pipefd[1]);
                // Input comes from the read of the pipe
                fdin = pipefd[0];
            }
            // If not using stdin use it
            if(fdin != 0) {
                dup2(fdin, 0);
            }

            // Runs the final command
            vector<string> commandVector = parser(incmds[incmdsize-1], ' ');
            fgProcess(commandVector, true, true);
            exit(0);
        }
        // Have the parent wait for child to finish
        else{
            waitpid(pid, NULL, 0);
        }
}


 /**
  * Main function that runs the program
  * @param unused
  * @param argv user command line input
  * @param envp unused
  * @return exit code
  */
int main(int argc, char **argv, char **envp) {

    // Checks if batch file was passed in
    bool isBatch = (argv[1]!=NULL) ? true : false;

    //Sets the the parent env variable
     getcwd(cwd, sizeof(cwd));
     strcat(cwd, "/myshell");
     setenv("SHELL", cwd, 1);

    string user_Input;
    string line;
    stillAlive = true;


     string batLine;
     vector<string> batInVect;

     ifstream batFileStream(argv[1]);

     if (isBatch) {
        // If we were able to open the file, then it is a batch
        bool batFSOpen = (batFileStream.is_open()) ? true : false;

        if(batFSOpen){
           while (getline(batFileStream, batLine)){
               batInVect.push_back(batLine);
           }
           // Close FS after is returned
           batFileStream.close();
       }
        else {
           cout << "Unable to open file";
           exit(1);
       }
    }


    // Main loop of the program
     while (stillAlive) {
         // Holds the user input
         string user_Input;
         // Handle batch input differently
         if(isBatch){
             if (batInVect.size() >= 1){
                 user_Input = batInVect.at(0);
                 batInVect.erase(batInVect.begin());
             }
             else{
                 stillAlive = false;
                 exit(0);
             }
         }
         else {
             user_Input = getUserInput();
         }
         // The string of commands the user put in
        vector<string> user_cms = parser(user_Input, ';');


        // Loop throough the string of commands looking for pipes
        for (int i = 0; i < user_cms.size(); i++) {
            if (user_cms[i] == "") { continue; }

            // Parse looking for pipes
            if (user_cms[i].find('|') != string::npos) {
                // If a pipe is found

                // Sequentially get the commands that pipe into each other
                vector<string> cmd_with_pipe = parser(user_cms[i], '|');
                pipeHandler(cmd_with_pipe);
                continue;
            }
            // Only gets here if there is not pipe
            vector<string> noPipe_cmd = parser(user_cms[i], ' ');
            fgProcess(noPipe_cmd, true, true);
        }

    }


}




// dup 0 1 2 - stdin stdout stderr

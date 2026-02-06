
#include<iostream>
#include<fstream>   //file stream
#include<string>
#include<cstdlib>   //exit
#include<cerrno>    //"errno" show the error
#include<cstring>   //"strerror" translate the error 
#include<unistd.h>  //fork , execv , pid_t
#include<sys/ptrace.h>  //ptrace , PTRACE_TRACEME , ptrace_CONT , PTRACE_KILL
#include<sys/wait.h>    //waitpid , WIFISTOPPED

void print_how_to_use(const char* program_name)
{
    using namespace std;
    cerr<<"Error "<<program_name<<" need the target path"<<endl;
    cerr<<"For example : "<<program_name<<" ./hello"<<endl;
    return ;
}

pid_t start_child_and_father_program(const char*target_path)  //pid_t = int
{
    using namespace std;
    pid_t pid = fork(); //it will make two program 
                        //one is the child program it pid is 0
                        //the other is the father program it pid is not 0
    
    if(pid == -1){  //pid = -1 means error
        cerr<<"Fork error"<<strerror(errno)<<endl;
        return -1;
    }

    else if(pid == 0){      //child program
        ptrace(PTRACE_TRACEME , 0 , nullptr , nullptr);   // "PTRACE_TRACEME" let the father program trace
        char*args[] = {const_cast<char*>(target_path) , nullptr};  //const_cast change "const char" to "char" tempory
                                                                   //args[0] = target path , args[1] = nullptr means the args end 
        execv(target_path , args);  //jump to target program   "args" will be placed in the main(args[0]) 

        //if failed the following will run
        cerr<<"Errno : "<<strerror(errno)<<endl;
        exit(1);
    }

    else{   //itʼs father program pid is the child id
        int status;
        waitpid(pid , &status , 0); //waitpid will wait until the child program pause
                                    //"0" means the function will return when the child program pause or end
                                    // & will convey the location so the function can modift the value
        if(WIFSTOPPED(status)){
                return pid; //child pid
        }
        else{
            return -1;
        }
    }
}

void print_memory_maps(pid_t pid)
{
    using namespace std;
    string path = "/proc/" + to_string(pid) + "/maps";  //file path
    ifstream file(path);    //open file

    if(!file){
        cerr<<"Error : "<<strerror(errno)<<" "<<path<<endl;
        return ;
    }
    string line;
    while(getline(file , line)){
        cout<<line<<endl;
    }
    file.close();
}

void clean_child_program(pid_t child_pid)
{
    using namespace std;
    ptrace(PTRACE_KILL ,child_pid , nullptr , nullptr);
    //kill the child program
    int status;
    waitpid(child_pid , &status , 0);
    cout<<"child_program_end"<<endl;
    return ;
}

int main(int argc , char*argv[])
{
    using namespace std;
    if(argc != 2){
        print_how_to_use(argv[0]);  //argv[0] = program name
        exit(1);
    }
    const char* target_path = argv[1];
    pid_t child_pid = start_child_and_father_program(target_path);
    if(child_pid == -1){
        cerr<<"Error"<<endl;
        exit(1);
    }
    print_memory_maps(child_pid);
    clean_child_program(child_pid);
    return 0;
}

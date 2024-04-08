// Trivial Torrent


//chmod 744 reference_binary/ttorrent


// TODO: some includes here


#include "file_io.h"
#include "logger.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
//more includes


// TODO: hey!? what is this?


/**
 * This is the magic number (already stored in network byte order).
 * See https://en.wikipedia.org/wiki/Magic_number_(programming)#In_protocols
 */


 //Define constants to reduce hard code
#define ERROR_VALUE -1
#define ARGUMENT_ONE 1
#define ARGUMENT_THREE 3
#define FOUR_BYTE_VALUE 0x1234ABCD
#define TWO_BYTE_VALUE 0xABCD
#define BUFFER_SIZE 65536
#define PORT 8080
#define SHIFT_VALUE 8
#define MESSAGE_CODE_INDEX 4
#define TRUE 1
#define ARRAY_SIZE 100
#define CLIENT 2
#define SERVER 4
#define CONTINUE 1
#define BACKLOG 10
#define FAIL 1


//Define static constants to reduce hard code
static const uint32_t MAGIC_NUMBER = 0xde1c3233; // = htonl(0x33321cde);
static const uint8_t MSG_REQUEST = 0;
static const uint8_t MSG_RESPONSE_OK = 1;
static const uint8_t MSG_RESPONSE_NA = 2;


//Definition of enums in order to reduce hard code (Structure recomended by teacher)
enum { RAW_MESSAGE_SIZE = 13 };


//Subfunciton Declaration for "haveEveryBlock"
//PARAMETERS:
// t : torrent structure declared in "file_io.h".
//FUNCTION:
// Checks if every block in the torrent structure has been downloaded.
//RETURN VALUE:
// 1: Every block has been downloaded.
// 0: Not every block has been downloaded.
//AUTHOR(S):
// Nedal Benelmekki & Marc Serra
int haveEveryBlock(struct torrent_t t);


//Subfunction Implementation for "haveEveryBlock"
int haveEveryBlock(struct torrent_t t)
{
    uint64_t nBlock = t.block_count;


    uint64_t i = 0;


    int cond = 1;


    while ((i < nBlock) && (cond == 1))
    {
        if (t.block_map[i] == 0)
            cond = 0;


        else
            i++;
    }


    return cond;
}


//Subfunciton Declaration for "strip_ext"
//PARAMETERS:
// filepath: extension that we want to delete.
//FUNCTION:
// Strips the extension from the file name
//RETURN VALUE:
// -
//AUTHOR(S):
// "Ad Abdusrdum", collaborator in StackOverFlow
void strip_ext(char* filepath);


//Subfunction Implementation for "strip_ext"
void strip_ext(char* filepath)
{
    char* end = filepath + strlen(filepath);


    while (end > filepath && *end != '.') {
        --end;
    }


    if (end > filepath) {
        *end = '\0';
    }
}


//Subfunciton Declaration for "client"
//PARAMETERS:
// argc: number of arguments
// argv: arguments passed in main
//FUNCTION:
// Client section of client-server.
//RETURN VALUE:
// -
//AUTHOR(S):
// Nedal Benelmekki & Marc Serra
int client(int argc, char** argv);


//Subfunction Implementation for "client"
int client(int argc, char** argv) {
    //CLIENT-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
   
    //Declare the torrent variable
    struct torrent_t torrent;


    /*  //Determine the size of the char array
    uint64_t sze = strlen(argv[argc - 1]);
    //Subtract 9, as we will have to delete ".ttorrent"
    sze = sze - 9;
    //Create actual variable
    char downloaded_file[sze];
    //Fill out the variable
    for (uint64_t i = 0; i < sze; i++)
    {
        downloaded_file[i] = argv[argc - 1][i];
    }
        downloaded_file[sze] = '\0';
    */


    //Declare the downloadaded file variable
    char* filepath = argv[argc - 1];


    //Declare new variable for the downloadaded file variable without the file termination
    char downloadedFilepath[ARRAY_SIZE];


    //Copy the contents into the desired structure
    memcpy(downloadedFilepath, filepath, ARRAY_SIZE);


    //Use declared function to delete/strip corresponding termination/extension.
    strip_ext(filepath);


    log_printf(LOG_INFO, "Creates file name and removes '.ttorrent' extension");


    //Error control and logic: Check if file exists in directory, using F_OK for existance test
    int exists = access((const char*)downloadedFilepath, F_OK);
    if (exists == ERROR_VALUE)
    {
        perror("ERROR: File does not exist in directoy.");


        return FAIL;
    }


    log_printf(LOG_INFO, "File is existant in the directory");


    //We proceed to create a torrent from the downloaded file
    int metaInfoFile = create_torrent_from_metainfo_file(downloadedFilepath, &torrent, filepath);
    //Corresponding error logic
    if (metaInfoFile == ERROR_VALUE)
    {
        perror("ERROR: Could not create the torrent file.");


        return FAIL;
    }


    log_printf(LOG_INFO, "Torrent structure was succefully created.");


    if (haveEveryBlock(torrent) == 1)
    {
        return 1;
    }


    log_printf(LOG_INFO, "Checked if all blocks were already downloaded: NO. We proceed with the code.");


    //Create a 4 byte and a 2 byte variable with the desired value
    //int val_4_bytes = FOUR_BYTE_VALUE; //htonl() --> host to network long || ntohl()
    //short val_2_bytes = TWO_BYTE_VALUE; //htons() --> host to network short || ntohs()


    /*
    PEERS
    1.
    2.
    3.
    */


    //Create a buffer, large enough to be used without any issues
    uint8_t buffer[MAX_BLOCK_SIZE]; //move outside of for in order to optimize and not create in each iteraton. 0x10000; 65535 = 0xffff (+1 = MAX_BLOCK_SIZE)


    //Determine the number of peers in the torrent structure created above in order to control the external for structure.
    uint64_t nPeers = torrent.peer_count;


    //Determine the number of blocks in the torrent structure created above in order to control the internal for structure.
    uint64_t nBlocks = torrent.block_count;


    //EXTERNAL FOR: Connect to each peer to request block information
    for (uint64_t i = 0; i < nPeers; i++)
    {
        log_printf(LOG_INFO, "Entering loop 1: For each peer.");


        //Initialize a TCP Socket: int socket(int domain, int type, int protocol)
        int s = socket(AF_INET, SOCK_STREAM, (int)0);
        //Error logic and control
        if (s == ERROR_VALUE)
        {
            perror("ERROR: The socket could not be created");


            return FAIL;
        }


        log_printf(LOG_INFO, "Socket was succesfully created.");


        //Generating the sockaddr structure for the function's parameters
        struct sockaddr_in address;


        //Set address family
        address.sin_family = AF_INET;


        //Assign port according to actual peer. Cannot afford to generalize port.
        address.sin_port = torrent.peers[i].peer_port;//address.sin_port = htons(PORT);


        //addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        //inet_pton(AF_INET, "127.0.0.1", &address.sin_addr.s_addr);


        //Initialize address to zero.
        address.sin_addr.s_addr = 0;


        /*
        for (int x = 3; x >= 0; x--)
        {
            address.sin_addr.s_addr = (uint32_t) (address.sin_addr.s_addr | torrent.peers[i].peer_address[x] << (8 * x)); //SHIFT_VALUE = 8


            //Shift address 8 bits
            address.sin_addr.s_addr  = address.sin_addr.s_addr << SHIFT_VALUE;
            //Introduce indexed peer address
            address.sin_addr.s_addr = torrent.peers[i].peer_address[x];
        }*/


        //In case loop does not work properly:
        address.sin_addr.s_addr = (uint32_t)torrent.peers[i].peer_address[3] << 24 | (uint32_t)torrent.peers[i].peer_address[2] << 16 | (uint32_t)torrent.peers[i].peer_address[1] << 8 | (uint32_t)torrent.peers[i].peer_address[0] << 0;


        log_printf(LOG_INFO, "Individual Peer Address was created succesfully.");


        //Calculating and storing the size of the structure
        socklen_t len = sizeof(address);


        //Connecting the socket to the peer in the "addr" address
        int c = connect(s, (const struct sockaddr*)&address, (socklen_t)len);
        //Error logic and control
        if (c == ERROR_VALUE)
        {
            perror("ERROR: The socket could not be connected with the provided parameters.");


            //Close the socket
            ssize_t cs = close(s);
            //Error logic and control: Closing the socket
            if (cs == ERROR_VALUE)
            {
                perror("ERROR: Could not close the socket correctly.");


                return FAIL;
            }


            continue;
        }


        log_printf(LOG_INFO, "Socket connect successfully");


        //INTERNAL FOR: For each block
        for (uint64_t j = 0; j < nBlocks; j++)
        {
            log_printf(LOG_INFO, "Starts seccond loop");


            if (torrent.block_map[j] == 1)
            {
                continue;
            }


            log_printf(LOG_INFO, "Check block");


            //Instruction from Professor Chow: from buffer[0] to buffer[3] put magic number
            //For structure to fill out buffer[0] to buffer [3]
            for (int z = 0; z < 4; z++)
            {
                //Formula or way to insert MAGIC_NUMBER
                // Proposal:
                    //Shift Right to shift value --> x = MAGIC_NUMBER >> 8 * (3 - z) --> x = MAGIC_NUMBER >> SHIFT_VALUE * (z - 3)
                    //Insert magic number --> buffer[z] = (char) x
                buffer[z] = (uint8_t)(MAGIC_NUMBER >> 8 * (3 - z));
            }


            //Instruction from Professor Chow: buffer[4] message code (Request)
            buffer[MESSAGE_CODE_INDEX] = MSG_REQUEST;


            //Instruction from Professor Chow: buffer[5] to buffer[12] put block number
            //For structure to fill out buffer[5] to buffer [12]
            for (int z = 5; z < 13; z++)
            {
                //Formula or way to insert block number
                // Proposal:
                    //Shift Right to shift value --> x = j >> 8 * (12 - z) --> x = j >> SHIFT_VALUE * (12 - z)
                    //Insert magic number --> buffer[z] = (char) x
                buffer[z] = (uint8_t)(j >> 8 * (12 - z));
            }


            log_printf(LOG_INFO, "Buffer filled out correctly.");


            //Sending the buffer to the server
            ssize_t se = send(s, &buffer, (size_t)RAW_MESSAGE_SIZE, 0); // SIZE = 13. We don't want to use hard code. Use declared enum.
            //Error logic and control
            if (se == ERROR_VALUE)
            {
                perror("ERROR: The socket could not be sent.");


                //Close the socket
                ssize_t cs = close(s);
                //Error logic and control
                if (cs == ERROR_VALUE)
                {
                    perror("ERROR: Could not close the socket correctly.");


                    return FAIL;
                }
                continue;
            }


            log_printf(LOG_INFO, "The send fuction was executed correctly.");


            ssize_t re = recv(s, &buffer, (size_t)RAW_MESSAGE_SIZE, MSG_WAITALL); // SIZE = 13. We don't want to use hard code. Use declared enum.
            //Error logic and control
            if (re == ERROR_VALUE)
            {
                perror("ERROR: The socket could not be received");


                continue;
            }


            log_printf(LOG_INFO, "First receive was achieved.");


            //Reconstruct magic number from received buffer
            uint32_t magicNumber = (uint32_t)buffer[0] << 24 | (uint32_t)buffer[1] << 16 | (uint32_t)buffer[2] << 8 | (uint32_t)buffer[3];


            //Obtain message code from received buffer
            uint8_t messageCode = (uint8_t)buffer[MESSAGE_CODE_INDEX];


            //Reconstruct block number from received buffer
            uint64_t blockNumber = (uint64_t)buffer[5] << 56 | (uint64_t)buffer[6] << 48 | (uint64_t)buffer[7] << 40 | (uint64_t)buffer[8] << 32 | (uint64_t)buffer[9] << 24 | (uint64_t)buffer[10] << 16 | (uint64_t)buffer[11] << 8 | (uint64_t)buffer[12];


            log_printf(LOG_INFO, "Magic Number, Message Code, and Block Number are updated after first receive.");


            //Condition in order to analyze if data was received corretly or not
            if ((magicNumber == MAGIC_NUMBER) && (messageCode == MSG_RESPONSE_OK))
            {
                log_printf(LOG_INFO, "Condition for second receive are achieved.");


                //Create block structure.
                struct block_t b;


                //Obtain block size
                b.size = get_block_size(&torrent, blockNumber);
                //Error logic and control
                if ((int)b.size == ERROR_VALUE)
                {
                    perror("ERROR: Could not get the size of the block in the downloaded file.");


                    return FAIL;
                }


                log_printf(LOG_INFO, "Block was successfully created, and its size has been calculated.");


                ssize_t reInFor = recv(s, (char*)buffer, b.size, MSG_WAITALL);
                //Error logic and control
                if (reInFor == ERROR_VALUE)
                {
                    perror("ERROR: The socket could not be received");


                    continue;
                }


                log_printf(LOG_INFO, "Second receive was achieved.");


                //Copy the data into the structure.
                memcpy(b.data, buffer, b.size);


                log_printf(LOG_INFO, "Data was copied into structure.");


                //Store the block in the downloaded file.
                int st_bl = store_block((struct torrent_t* const)&torrent, blockNumber, (const struct block_t* const)&b);
                //Error logic and control
                if (st_bl == ERROR_VALUE)
                {
                    perror("ERROR: block could not be stored");
                }
                else
                {
                    torrent.block_map[j] = 1;
                }


                log_printf(LOG_INFO, "Block has been stored.");
            }
        }


        //Close the socket
        ssize_t cs = close(s);
        //Error logic and control
        if (cs == ERROR_VALUE)
        {
            perror("ERROR: Could not close the socket correctly.");


            return FAIL;
        }


        //Double-check if every block has been downloaded. In case they have, we exit the loop.
        if (haveEveryBlock(torrent) == 1)
        {
            break;
        }
    }


    log_printf(LOG_INFO, "Client section has finished without any issues.");


    //DESTROY TORRENT
    int destroy = destroy_torrent(&torrent);
    //Error logic and control
    if(destroy == ERROR_VALUE)
    {
        perror("ERROR: Torrent could not be closed properly.");
        return FAIL;
    }
    log_printf(LOG_INFO, "Torrent could be closed properly.");


    return 0;
}


//Subfunciton Declaration for "server"
//PARAMETERS:
// argc: number of arguments
// argv: arguments passed in main
//FUNCTION:
// Server section of client-server.
//RETURN VALUE:
// -
//AUTHOR(S):
// Nedal Benelmekki & Marc Serra
int server(int argc, char** argv);


//Subfunction Implementation for "client"
int server(int argc, char** argv)
{
    //Inital part: very similar to client


    //Create torrent structure
    struct torrent_t torrent;


    //Obtain Filepath, formating accordingly:


    //Declare the downloadaded file variable
    char* filepath = argv[argc - 1];


    //Declare new variable for the downloadaded file variable without the file termination
    char downloadedFilepath[ARRAY_SIZE];


    //Copy the contents into the desired structure
    memcpy(downloadedFilepath, filepath, ARRAY_SIZE);


    //Use declared function to delete/strip corresponding termination/extension.
    strip_ext(filepath);


    log_printf(LOG_INFO, "Creates file name and removes '.ttorrent' extension");


    //Create the torrent from the metainfo file.
    int metaInfoFile = create_torrent_from_metainfo_file(downloadedFilepath, &torrent, filepath);
    //Error logic
    if (metaInfoFile == ERROR_VALUE)
        {
            perror("ERROR: create_torrent_from_metainfo_file function error.");
            log_printf(LOG_INFO,"ERROR: create_torrent_from_metainfo_file function error.");
            return FAIL;
        }
    log_printf(LOG_INFO,"Torrent was created correctly.");


    //Initialize a TCP Socket: int socket(int domain, int type, int protocol)
    int s = socket(AF_INET, SOCK_STREAM, (int)0);
    //Error logic and control
    if (s == ERROR_VALUE)
        {
            perror("ERROR: socket function error.");
            log_printf(LOG_INFO,"ERROR: socket function error.");
            return FAIL;
        }
    log_printf(LOG_INFO,"Socket was created successfully");


    //Generating the sockaddr structure for the function's parameters
    struct sockaddr_in sa;


    //Set address family
    sa.sin_family = AF_INET;


    //Assign port accordingly
    sa.sin_port = htons(PORT);


    //Initialize address.
    sa.sin_addr.s_addr = INADDR_ANY;


    log_printf(LOG_INFO,"Address was created successfully");


    //Obtain size of address in a socklen_t structure (to avoid bad representation)
    socklen_t len = sizeof(sa);


    //Bind function: binding previous/first socket and generated/declared address
    int b = bind(s, (struct sockaddr*)&sa, len);
    //Error logic
    if (b == ERROR_VALUE)
        {
            perror("ERROR: bind function error.");
            log_printf(LOG_INFO,"ERROR: bind function error.");
            ssize_t cs = close(s);
            //Error logic and control: Closing the socket
            if (cs == ERROR_VALUE)
            {
                perror("ERROR: Could not close the socket correctly.");


                return FAIL;
            }
           
            return FAIL;
        }
    log_printf(LOG_INFO,"Bind successful");


    //Listen function: Listen to the socket (first socket)
    int l = listen(s, BACKLOG/*backlog*/);
    //Error logic
    if (l == ERROR_VALUE)
        {
            perror("ERROR: listen function error.");
            log_printf(LOG_INFO,"ERROR: listen function error.");
            ssize_t cs = close(s);
            //Error logic and control: Closing the socket
            if (cs == ERROR_VALUE)
            {
                perror("ERROR: Could not close the socket correctly.");


                return FAIL;
            }
           
            return FAIL;
        }
    log_printf(LOG_INFO,"Listen successful");
   
    //Outer Loop
    while (CONTINUE)
    {
        log_printf(LOG_INFO,"Entering first loop");


        //Accept function: Accept first socket, store into s2
        int s2 = accept(s, (struct sockaddr*)&sa, &len);
        //Error logic
        if (s2 == ERROR_VALUE)
        {
            perror("ERROR: accept function error.");
            log_printf(LOG_INFO,"ERROR: accept function error.");
            return FAIL;
        }
        log_printf(LOG_INFO,"Accept successful");


        //Fork function: Begin of fork
        int pid = fork();
        log_printf(LOG_INFO,"Fork done");


        //In case of pid refering to child process
        if(pid == 0)//child process
        {
            log_printf(LOG_INFO,"Entering child process");
            ssize_t cs = close(s);
            //Error logic and control: Closing the socket
            if (cs == ERROR_VALUE)
            {
                perror("ERROR: Could not close the socket correctly.");


                return FAIL;
            }
            //uint64_t i;
            //Inner loop
            while(CONTINUE)
            {
                //Declare and initialize buffer
                char buffer[RAW_MESSAGE_SIZE + MAX_BLOCK_SIZE] = {0};


                //Receive function: Receving the buffer through the second socket
                ssize_t r = recv(s2, (char*)buffer, /*sizeof(buffer)*/RAW_MESSAGE_SIZE, 0);


                //Exit condition
                if (r == 0)
                {
                    break;
                }


                //Error logic
                else if (r == ERROR_VALUE)
                {
                    perror("ERROR: Recv function error.");
                    log_printf(LOG_INFO,"ERROR: Recv function error.");
                    continue;
                }
                log_printf(LOG_INFO,"Recv hecho");


                /*magic number revisar*/
                //Reconstruct magic number from received buffer
            uint32_t magicNum = (uint32_t)buffer[0] << 24 | (uint32_t)buffer[1] << 16 | (uint32_t)buffer[2] << 8 | (uint32_t)buffer[3];


            //Reconstruct block number from received buffer
            uint64_t blockNumber = (uint64_t)buffer[5] << 56 | (uint64_t)buffer[6] << 48 | (uint64_t)buffer[7] << 40 | (uint64_t)buffer[8] << 32 | (uint64_t)buffer[9] << 24 | (uint64_t)buffer[10] << 16 | (uint64_t)buffer[11] << 8 | (uint64_t)buffer[12];


            log_printf(LOG_INFO, "Magic Number, Message Code, and Block Number are updated after first receive.");


            //Condition in order to analyze if data was received corretly or not
                if ((magicNum == MAGIC_NUMBER) && (buffer[4] == MSG_REQUEST))
                {
                    struct block_t block;
                    block.size = 0;


                    if(blockNumber < torrent.block_count)
                    {
                        if(torrent.block_map[blockNumber] == 1)
                        {
                            buffer[4] = (char) MSG_RESPONSE_OK;
                            log_printf(LOG_INFO, "Block_number found");


                            int load = load_block((struct torrent_t* const)&torrent, blockNumber, &block);
                            //Error logic and control
                            if(load == ERROR_VALUE)
                            {
                                perror("ERROR: load_block function error.");
                                log_printf(LOG_INFO,"ERROR: load_block function error.");
                                return FAIL;
                            }
                            log_printf(LOG_INFO, "load_block done");


                            memcpy(buffer+RAW_MESSAGE_SIZE, block.data, block.size);
                            log_printf(LOG_INFO,"Memory copied into buffer");


                        }
                        else
                        {
                            log_printf(LOG_INFO, "ERROR: Block_number not found");
                        }
                    }
                    else
                    {
                        log_printf(LOG_INFO, "ERROR: Block_number not valid");
                    }
                    //Sending + error message
                    ssize_t se = send(s2, buffer, RAW_MESSAGE_SIZE + block.size, 0);
                    //Error logic and control
                    if (se == ERROR_VALUE)
                    {
                        perror("ERROR: Send function error.");
                        log_printf(LOG_INFO,"ERROR: Send function error.");
                        return FAIL;
                    }
                    log_printf(LOG_INFO,"Send done");
                }
                else
                {
                    log_printf(LOG_INFO, "ERROR: Data not received correctly");
                }
            } /*end */
            exit(0);
            log_printf(LOG_INFO, "End of child process");
        }
        else /*father process*/
        {
            ssize_t cs = close(s2);
            //Error logic and control: Closing the socket
            if (cs == ERROR_VALUE)
            {
                perror("ERROR: Could not close the socket correctly.");


                return FAIL;
            }
            log_printf(LOG_INFO, "End of father process");
        }
    }


    ssize_t cs = close(s);
    //Error logic and control: Closing the socket
    if (cs == ERROR_VALUE)
    {
        perror("ERROR: Could not close the socket correctly.");


        return FAIL;
    }


    //DESTROY TORRENT
    int destroy = destroy_torrent(&torrent);
    //Error logic and control
    if(destroy == ERROR_VALUE)
    {
        perror("ERROR: Torrent could not be closed properly.");
        return FAIL;
    }
    log_printf(LOG_INFO, "Torrent could be closed properly.");


    return 0;
   
}


/**
 * Main function.
 */
int main(int argc, char** argv) {


    set_log_level(LOG_DEBUG);


    log_printf(LOG_INFO, "Trivial Torrent (build %s %s) by %s", __DATE__, __TIME__, "Nedal Benelmekki and Marc Serra");


    // ==========================================================================
    // Parse command line
    // ==========================================================================


    // TODO: some magical lines of code here that call other functions and do various stuff.


    //START OF OUR CODE
    //Analyze argc in order to determine if we call server or client function.


    //In case of client function:
    if (argc == CLIENT)
    {
        //Create variable to control client function.
        int c = client(argc, argv);


        //In case of a -1 return, there was an error.
        if (c == ERROR_VALUE)
        {
            perror("ERROR: Client function error.");
            log_printf(LOG_INFO,"ERROR: Client function error.");
            return FAIL;
        }
    }
    //In case of server function:
    else if (argc == SERVER)
    {
        //Create variable to control server function.
        int s = server(argc, argv);


        //In case of a -1 return, there was an error.
        if (s == ERROR_VALUE)
        {
            perror("ERROR: Server function error.");
            log_printf(LOG_INFO,"ERROR: Server function error.");
            return FAIL;
        }
    }
    //In case it is none of the two options: ERROR.
    else
    {
        perror("Commands error");


        return FAIL;
    }


    return 0;


    // The following statements most certainly will need to be deleted at some point...
    (void)argc;
    (void)argv;
    (void)MAGIC_NUMBER;
    (void)MSG_REQUEST;
    (void)MSG_RESPONSE_NA;
    (void)MSG_RESPONSE_OK;


}

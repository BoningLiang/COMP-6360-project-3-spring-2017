#include <stdlib.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <fstream>
#include <sstream>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <ctime>
#include <math.h>
#include "config.h"

using namespace std;

// #define NODE 1
#define TIME 1000*1000
#define PACKAGE_GENERATE_TIME 200*1000
#define PACKAGE_SEND_TIME 40*1000

typedef struct{
    int nodeNum;
    string IP;
    int port;
    int x;
    int y;
    vector <int> links;
} node;

typedef struct{
    int neighbors_id;
    string state_of_link;
    int time;
} one_hop_neighbors_entry;

typedef struct{
    int neighbors_id;
    int access_though;
    int time;
} two_hop_neighbors_entry;

typedef struct{
    int from_id;
    vector<int> neighbors;
} hello;

typedef struct{
    int mpr_id;
    int number_of_covered_hops;
} find_mpr_entry;

vector <one_hop_neighbors_entry> one_hop_neighbors_table;
vector <two_hop_neighbors_entry> two_hop_neighbors_table;

vector <node> mpr_table;

vector <node> nodes;
vector <string> packageTable;
int k=1;
int global_port = 0;
int sequence = 1;
int mode=0;
int default_ethernet_loss_probability = 0;
int default_manet_loss_probability = 0;
int ethernetLossProbability = default_ethernet_loss_probability; // default loss rate (0~100)
int manetLossProbability = default_manet_loss_probability;
struct sockaddr_in myAddress;
struct sockaddr_in priorAddress;
struct sockaddr_in terminalAddress;
struct sockaddr_in sourceAddress;
struct sockaddr_in nextAddress;
string localAddress;
string destination_IP;
int destination_port;

int thisX;
int thisY;

string localIP;
int localPort=10010;
int sequenceCache = 0;
int ackCache = 0;
int fdSend;
int fdRecv;
vector <int> broadcastList; // node numbers to broadcast

int heartRate = -1;
double location_x = 999;
double location_y = 999;
double oxygen = -1;
double toxic = -1;

int stoi(string args)
{
    return atoi(args.c_str());
}

double stod(string args)
{
    return atof(args.c_str());
}

string to_string(int i)
{
    stringstream ss;
    ss << i;
    return ss.str();
}

string to_string(double i)
{
    stringstream ss;
    ss << i;
    return ss.str();
}

string to_string(char* c)
{
    stringstream ss;
    ss << c;
    return ss.str();
}

string destinationAddress = destination_IP +":"+ to_string(destination_port);

string getRealIP(string tux_name)
{
    string str;
    ifstream referenceStringFile(REAL_IP_FILE);
    if (!referenceStringFile)
    {
        cout<<"No real ip file found!"<<endl;
    }
    else
    {
        char line[100];
        while (referenceStringFile.getline(line, sizeof(line)))
        {
            stringstream stream(line);
            string item;
            int i=0;
            // cout<<"line: "<<line<<endl;
            string tuxName;
            while(getline(stream, item, ' '))
            {
                if (i==0)
                {
                    tuxName = item;
                    // cout<<"tuxName:"<<tuxName<<"."<<endl;
                    // cout<<"tux_name:"<<tux_name<<"."<<endl;
                }
                if (i==1)
                {
                    // cout<<"tuxName:"<<tuxName<<"."<<endl;
                    // cout<<"tux_name:"<<tux_name<<"."<<endl;
                    // cout<<(tuxName==tux_name)<<endl;
                    if (tuxName == tux_name)
                    {
                        return item;
                    }
                }
                i++;
            }
        }
    }
    return "0";
}

int init_setup()
{
    string str;
    int count = 0;

    ifstream referenceStringFile(MANET_FILE);

    if (!referenceStringFile)
    {
        cout<<"No reference string file found!"<<endl;
        return 0;
    }
    else
    {
        char line[100];
        string lineString;
        int isNullLine = 0;
        while (referenceStringFile.getline(line, sizeof(line)))
        {
            node myNode;
            // sscanf(line,
            //     "Node %d %[^,], %d %d %d links %[^*]",
            //     &myNode.nodeNum, myNode.IP, &myNode.port, &myNode.x, &myNode.y, myNode.links);
            stringstream stream(line);
            lineString = to_string(line);
            string item;
            int i = 0;
            while(getline(stream, item, ' '))
            {
                //cout<<item<<endl;
                if (i == 0)
                {
                    if (item != "Node")
                    {
                        isNullLine = 1;
                    }
                    else if (item == "Node")
                    {
                        isNullLine = 0;
                    }
                }
                if (i == 1)
                {
                    myNode.nodeNum = stoi(item);
                    // cout<<myNode.nodeNum<<endl;
                }
                if (i == 2)
                {
                    // IP
                    item.erase(item.end()-1); // remove ',' at the end of the 'tux78,' for example.
                    myNode.IP = getRealIP(item);
                    //myNode.IP = item;
                    // cout<<"real ip: "<<myNode.IP<<endl;
                    if (myNode.nodeNum == SOURCE_NODE)
                    {
                        localIP = myNode.IP;
                    }
                    if (myNode.nodeNum == DESTINATION_NODE)
                    {
                        destination_IP = myNode.IP;
                    }
                    // cout<<myNode.IP<<endl;
                }
                if (i == 3)
                {
                    myNode.port = stoi(item);
                    if (myNode.nodeNum == SOURCE_NODE)
                    {
                        localPort = stoi(item);
                        localAddress = localIP +":"+ item;
                    }
                    if (myNode.nodeNum == DESTINATION_NODE)
                    {
                        destination_port = myNode.port;
                    }
                }
                if (i == 4) // x
                {
                    myNode.x = stoi(item);
                    if (myNode.nodeNum == SOURCE_NODE)
                    {
                        thisX = myNode.x;
                    }
                }
                if (i == 5) // y
                {
                    myNode.y = stoi(item);
                    if (myNode.nodeNum == SOURCE_NODE)
                    {
                        thisY = myNode.y;
                    }
                    // cout<<myNode.y<<endl;
                }
                if (i >6)
                {
                    myNode.links.push_back(stoi(item));
                    if (myNode.nodeNum == SOURCE_NODE)
                    {
                        broadcastList.push_back(stoi(item));
                    }
                }
                i++;
            }
            //cout<<endl;
            // if(lineString.length()>0)
            if (isNullLine == 0)
            {
                nodes.push_back(myNode);
            }
            else if (isNullLine == 1)
            {
                /* code */
            }
        }
    }
    return 0;
}

int gremlin(int g_mode)
{
    int lossPacket = 0;
    srand(time(NULL));
    if((rand()%100<ethernetLossProbability) && (g_mode == 0))
    {
        return 1; //loss
    }
    if((rand()%100<manetLossProbability) && (g_mode == 1))
    {
        return 1; //loss
    }
    else
    {
        return 0;
    }
}

int gremlin(double p)
{
    int lossPacket = 0;
    srand(time(NULL));
    if(rand()%100<p)
    {
        return 1; //loss
    }
    else
    {
        return 0;
    }
}

// int fastAndSlowFadingLoss(int x1, int y1, int x2, int y2)
// {
//     double distance = sqrt(abs(x1-x2)*abs(x1-x2) + abs(y1-y2)*abs(y1-y2));
//     if (distance<=100)
//     {
//         cout<<"distance: "<<distance<<endl;
//     }
//     int p = 100;
//     if (distance<100)
//     {
//         p = -0.05 * distance + 100 ;
//     }
//     else if((distance>=100) && (distance<=101))
//     {
//         p = -90 * distance +9090;
//     }
//     else if ((distance>101) && (distance<=200))
//     {
//         p = (-5.0/99.0) * distance + (1000.0/99.0);
//     }
//     else
//     {
//         p = 0;
//     }

//     return gremlin(100-p);
// }

int fastAndSlowFadingLoss(int x1, int y1, int x2, int y2)
{
    double distance = sqrt(abs(x1-x2)*abs(x1-x2) + abs(y1-y2)*abs(y1-y2));
    // if (distance<=100)
    // {
    //     cout<<"distance: "<<distance<<endl;
    // }
    int p = 100;
    if (distance<100 && distance > 0)
    {
        return 0;
    }
    else if((distance>=100) && (distance<=101))
    {
        p = -90 * distance +9090;
    }
    else if ((distance>101) && (distance<=200))
    {
        p = (-5.0/99.0) * distance + (1000.0/99.0);
    }
    else
    {
        p = 0;
    }

    return gremlin(100-p);
}

void* generatePackage(void* args)
{
    for(;;)
    {
        string bufString;
        // sequence:1,source:22.22.22.22:3333,destination:33.33.33.33:2222,previous:66.66.66.66:1001,data:heart:80
        // sequence:1,22.22.22.22:3333,33.33.33.33:2222,66.66.66.66:1001,heart:80,location:23.456789:34.2342342,air:98,toxic:1
        // bufString = "sequence:"+to_string(k)+","+localAddress+","+destinationAddress+","+localAddress+",heart:"+to_string(heartRate)
        //     +",location:"+to_string(location_x)+":"+to_string(location_y)+",air:"+to_string(oxygen)+",toxic:"+to_string(toxic);
        bufString = ","+localIP+":"+to_string(localPort)+","+destination_IP+":"+to_string(destination_port)+","+to_string(SOURCE_NODE)+",heart:"+to_string(heartRate)
            +",location:"+to_string(location_x)+":"+to_string(location_y)+",air:"+to_string(oxygen)+",toxic:"+to_string(toxic)+","+"mode:";
        packageTable.push_back(bufString);
        usleep(PACKAGE_GENERATE_TIME);
    }
}

// int generatePackage()
// {
//     // for(;;)
//     // {
//         string bufString;
//         // sequence:1,source:22.22.22.22:3333,destination:33.33.33.33:2222,previous:66.66.66.66:1001,data:heart:80
//         // sequence:1,22.22.22.22:3333,33.33.33.33:2222,66.66.66.66:1001,heart:80,location:23.456789:34.2342342,air:98,toxic:1
//         // bufString = "sequence:"+to_string(k)+","+localAddress+","+destinationAddress+","+localAddress+",heart:"+to_string(heartRate)
//         //     +",location:"+to_string(location_x)+":"+to_string(location_y)+",air:"+to_string(oxygen)+",toxic:"+to_string(toxic);
//         bufString = ","+localIP+":"+to_string(localPort)+","+destination_IP+":"+to_string(destination_port)+","+to_string(SOURCE_NODE)+",heart:"+to_string(heartRate)
//             +",location:"+to_string(location_x)+":"+to_string(location_y)+",air:"+to_string(oxygen)+",toxic:"+to_string(toxic)+","+"mode:";
//         packageTable.push_back(bufString);
//         // cout<<"Generated package: "<<bufString<<endl;
//         // usleep(PACKAGE_GENERATE_TIME);
//         return 0;
//     // }
// }

int print_one_hop_neighbors_table()
{   
    cout<< "Neighbor's ID\t"<<"State of Link"<<endl;
    for (int i = 0; i < one_hop_neighbors_table.size(); i++)
    {
        cout<<one_hop_neighbors_table.at(i).neighbors_id<<"\t"<<one_hop_neighbors_table.at(i).state_of_link<<endl;
    }
    return 0;
}

int print_two_hop_neighbors_table()
{
    cout<<"2-hop ID"<<"\t"<<"Access Though"<<endl;
    for (int i = 0; i < two_hop_neighbors_table.size(); i++)
    {
        cout<<two_hop_neighbors_table.at(i).neighbors_id<<"\t"<<two_hop_neighbors_table.at(i).access_though<<endl;
    }
    cout<<endl;
    return 0;
}

int getHowManyOneHopNodesCoverTheNode(int node_id)
{   
    int count = 0;
    for (int i = 0; i < two_hop_neighbors_table.size(); i++)
    {
        if (two_hop_neighbors_table.at(i).neighbors_id == node_id)
        {
            count++;
        }
    }
    return count;
}

int update_mpr_entry(int update_one_hop_neighbor_id, string state_of_link)
{
    for (int i = 0; i < one_hop_neighbors_table.size(); i++)
    {
        if (one_hop_neighbors_table.at(i).neighbors_id == update_one_hop_neighbor_id)
        {
            if(one_hop_neighbors_table.at(i).state_of_link == "MPR" && state_of_link == "uni-directional")
            {
                one_hop_neighbors_table.at(i).state_of_link = state_of_link;
            }
            break;
        }
    }
    return 0;
}

string getLinkState(int node_id)
{
    for (int i = 0; i < one_hop_neighbors_table.size(); i++)
    {
        if (one_hop_neighbors_table.at(i).neighbors_id == node_id)
        {
            return one_hop_neighbors_table.at(i).state_of_link;
        }
    }

    return NULL;
}


int isCovered(int neighbors_id)
{
    for (int i = 0; i < two_hop_neighbors_table.size(); i++)
    {
        if (two_hop_neighbors_table.at(i).neighbors_id == neighbors_id)
        {
            if(getLinkState(two_hop_neighbors_table.at(i).access_though) == "MPR")
            {
                return 1;
            }
        }
    }
    return 0;
}

int alreadyCovered(int id, vector<two_hop_neighbors_entry> unCovered)
{
    for (int i = 0; i < unCovered.size(); i++)
    {
        if (unCovered.at(i).neighbors_id == id)
        {
            return 1;
        }
    }
    return 0;
}

int addTwoHopNeighbors(int neighbors_id, int access_though)
{
    two_hop_neighbors_entry twoEntry;
    twoEntry.neighbors_id = neighbors_id;
    twoEntry.access_though = access_though;
    twoEntry.time = time(NULL);
    two_hop_neighbors_table.push_back(twoEntry);
    return 0;
}

int addOneHopNeighbors(int neighbors_id, string state_of_link)
{
    one_hop_neighbors_entry oneEntry;
    oneEntry.neighbors_id = neighbors_id;
    oneEntry.state_of_link = state_of_link;
    oneEntry.time = time(NULL);
    one_hop_neighbors_table.push_back(oneEntry);
    return 0;
}

int one_hop_table_update_mpr(int id)
{
    for (int i = 0; i < one_hop_neighbors_table.size(); i++)
    {
        if (one_hop_neighbors_table.at(i).neighbors_id == id)
        {
            one_hop_neighbors_table.at(i).state_of_link = "MPR";
            break;
        }
    }
    return 0;
}

int update_mpr()
{   
    // first step
    vector<int> mpr;
    vector<two_hop_neighbors_entry> allTwoHopNeighbors;
    vector<int> notCovered;
    vector<two_hop_neighbors_entry> unCovered;
    for (int i = 0; i < two_hop_neighbors_table.size(); i++)
    {
        int update_one_hop_neighbor_id = two_hop_neighbors_table.at(i).access_though;
        if (getHowManyOneHopNodesCoverTheNode(two_hop_neighbors_table.at(i).neighbors_id) == 1 )
        {
            update_mpr_entry(update_one_hop_neighbor_id, "MPR");
            mpr.push_back(update_one_hop_neighbor_id);
        }
        else
        {
            update_mpr_entry(update_one_hop_neighbor_id, "uni-directional");
        }
    }
    for (int i = 0; i < two_hop_neighbors_table.size(); ++i)
    {
        int k=1;
        for (int j = 0; j < allTwoHopNeighbors.size(); j++)
        {
            if (allTwoHopNeighbors.at(j).neighbors_id == two_hop_neighbors_table.at(i).neighbors_id)
            {
                k = 0;
            }
        }
        if (k == 1)
        {
            two_hop_neighbors_entry entry = two_hop_neighbors_table.at(i);
            allTwoHopNeighbors.push_back(two_hop_neighbors_table.at(i));
        }
    }
    for (int i = 0; i < allTwoHopNeighbors.size(); ++i)
    {
        if(isCovered(allTwoHopNeighbors.at(i).neighbors_id) == 0)
        {
            two_hop_neighbors_entry entry = allTwoHopNeighbors.at(i);
            unCovered.push_back(entry);
        }
    }

    // print out two-hop neighbors which are not covered by the one-hop neighbors
    // for (int i = 0; i < unCovered.size(); i++)
    // {
    //     cout<<unCovered.at(i).neighbors_id<<endl;
    // }

    // find one-hop neighbors which are connected with the un-covered neighbors
    // vector<find_mpr_entry> one_hop_neighbors_un_covered_neighbors
    vector<int> one_hop_neighbors_un_covered_neighbors;
    for (int i = 0; i < unCovered.size(); i++)
    {
        for (int j = 0; j < two_hop_neighbors_table.size(); j++)
        {
            if (two_hop_neighbors_table.at(j).neighbors_id == unCovered.at(i).neighbors_id)
            {
                // found
                int m = 1;
                for (int k = 0; k < one_hop_neighbors_un_covered_neighbors.size(); k++)
                {
                    if (one_hop_neighbors_un_covered_neighbors.at(k) == two_hop_neighbors_table.at(j).access_though)
                    {
                        m = 0;
                        break;
                    }
                }
                if (m == 1)
                {
                    one_hop_neighbors_un_covered_neighbors.push_back(two_hop_neighbors_table.at(j).access_though);
                }
            }
        }
    }
    // cout<<endl;
    // // print out one-hop neighbors which are connected with the un-covered neighbors
    // for (int i = 0; i < one_hop_neighbors_un_covered_neighbors.size(); i++)
    // {
    //     cout<<one_hop_neighbors_un_covered_neighbors.at(i)<<endl;
    // }

    vector<int> nonMPR = one_hop_neighbors_un_covered_neighbors;
    while(unCovered.size()>0)
    {
        int maxCovered = 0;
        int maxID = -1;
        for (int i = 0; i < nonMPR.size(); i++)
        {
            int ID = nonMPR.at(i);
            int count = 0;
            for (int j = 0; j < two_hop_neighbors_table.size(); j++)
            {
                int twoHopID = two_hop_neighbors_table.at(j).neighbors_id;
                if (two_hop_neighbors_table.at(j).access_though == ID && alreadyCovered(twoHopID, unCovered) == 1)
                {
                    count++;
                }
            }
            if (count>=maxCovered)
            {
                maxID = ID;
                maxCovered = count;
            }
        }
        one_hop_table_update_mpr(maxID);
        for (int i = 0; i < two_hop_neighbors_table.size(); i++)
        {
            int twoHopID = two_hop_neighbors_table.at(i).neighbors_id;
            if (two_hop_neighbors_table.at(i).access_though == maxID && alreadyCovered(twoHopID, unCovered) == 1)
            {
                for (int j = 0; j < unCovered.size(); j++)
                {
                    if (unCovered.at(j).neighbors_id == twoHopID)
                    {
                        unCovered.erase(unCovered.begin() + j);
                    }
                }
            }
        }
    }
    return 0;
    
}


hello parsingHello(string helloString)
{
    stringstream stream(helloString);
    string item;
    string sub_item;
    int isHello = 0;
    int i=0;
    hello helloMsg;
    int from_id;
    vector<int> neighbors_id;
    int neighbor_id;
    while(getline(stream, item, ','))
    {
        if (i==0)
        {
            stringstream sub_stream(item);
            int j=0;
            while(getline(sub_stream, sub_item, ':'))
            {
                if (j==0)
                {
                    if (sub_item == "hello")
                    {
                        isHello = 1;
                    }
                }
                if (j==1)
                {
                    if (isHello == 1)
                    {
                        from_id = stoi(sub_item);
                    }
                }
                j++;
            }
        }
        if (i>0)
        {
            if (item != "")
            {
                neighbor_id = stoi(item);
                neighbors_id.push_back(neighbor_id);
            }
            else
            {
                break;
            }
        }
        i++;
    }
    helloMsg.from_id = from_id;
    helloMsg.neighbors = neighbors_id;
    return helloMsg;
}

int deleteExpiredNeighbors()
{
    // update tables where expire
    for(int j = 0; j < one_hop_neighbors_table.size();)
    {
        int p = time(NULL) - one_hop_neighbors_table.at(j).time;
        // cout<<"p: "<<p<<endl;
        if (p > 5)
        {
            int one_hop_id = one_hop_neighbors_table.at(j).neighbors_id;
            one_hop_neighbors_table.erase(one_hop_neighbors_table.begin()+j);
            for(int t = 0; t < two_hop_neighbors_table.size();)
            {
                if (two_hop_neighbors_table.at(t).access_though == one_hop_id || two_hop_neighbors_table.at(t).neighbors_id == one_hop_id)
                {
                    two_hop_neighbors_table.erase(two_hop_neighbors_table.begin()+t);
                }
                else
                {
                    t++;
                }
            }
        }
        else
        {
            j++;
        }
    }
     for (int i = 0; i < two_hop_neighbors_table.size(); i++)
    {
        int p = time(NULL) - two_hop_neighbors_table.at(i).time;
        if (p>5)
        {
            two_hop_neighbors_table.erase(two_hop_neighbors_table.begin()+i);
        }
        else
        {
            i++;
        }
    }
    return 0;
}

int handleHello(string helloString)
{
    // update tables where expire
    // for(int j = 0;j<one_hop_neighbors_table.size();j++)
    // {
    //     if (time(NULL) - one_hop_neighbors_table.at(j).time > 5)
    //     {
    //         one_hop_neighbors_table.erase(one_hop_neighbors_table.begin()+j);
    //         for(int t = 0; t < two_hop_neighbors_table.size();t++)
    //         {
    //             if (two_hop_neighbors_table.at(t).access_though == one_hop_neighbors_table.at(j).neighbors_id)
    //             {
    //                 two_hop_neighbors_table.erase(two_hop_neighbors_table.begin()+t);
    //                 t--;
    //             }
    //         }
    //         j--;
    //     }
    // }
    hello myHello = parsingHello(helloString);

    int existOneEntry = 0;
    int existTwoentry = 0;
    int isModified = 0;

    for (int i = 0; i < one_hop_neighbors_table.size(); i++)
    {
        if (one_hop_neighbors_table.at(i).neighbors_id == myHello.from_id)
        {
            existOneEntry = 1;
            if (one_hop_neighbors_table.at(i).state_of_link == "uni-directional")
            {
                one_hop_neighbors_table.at(i).state_of_link = "bi-directional";
                isModified = 1;
            }
        }
    }

    if (existOneEntry == 0)
    {
        addOneHopNeighbors(myHello.from_id, "uni-directional");
        isModified = 1;
    }

    for (int i = 0; i < myHello.neighbors.size(); i++)
    {
        existTwoentry = 0;
        for (int j = 0; j < two_hop_neighbors_table.size(); j++)
        {
            if (myHello.neighbors.at(i) == two_hop_neighbors_table.at(j).neighbors_id && myHello.from_id == two_hop_neighbors_table.at(j).access_though)
            {
                existTwoentry = 1;
                break;
            }
        }
        if (existTwoentry == 0)
        {
            addTwoHopNeighbors(myHello.neighbors.at(i), myHello.from_id);
            isModified = 1;
        }
    }

    if (isModified == 1)
    {
        update_mpr();
    }

    // print_one_hop_neighbors_table();
    // cout<<endl;
    // print_two_hop_neighbors_table();

    return 0;
}



// only the hub have this thread
void* threadCreatePackageAndBroadcast(void* args) // send // mode 1 //manet
{
    // hub
    // first send
    // if received the ACK, then broadcast next package
    // if not received the ACK, then re-send the package

    struct sockaddr_in myaddr, remaddr;
    int fdHub;
    unsigned char buf[DATASIZE];
    int recvLen;
    char * server;

    if ((fdHub=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        cout<<"socket created"<<endl;

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);

    if (bind(fdHub, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }

    /* now define remaddr, the address to whom we want to send messages */
    /* For convenience, the host address is expressed as a numeric IP address */
    /* that we will convert to a binary format via inet_aton */

    int isBroadcastHello = 1;
    int forwardingCount = 0;

    for (;; k++) // manet 1
    {
        for (;;)
        {
            //cout<<"manet mode thread: "<<mode<<endl;
            if (forwardingCount>=15)
            {
                isBroadcastHello = 1;
                forwardingCount=0;
                break;
            }
            else if (ethernetLossProbability == 100 && manetLossProbability != 100 )
            {
                mode = 1;
                break;
            }
            else if ( mode ==0 || manetLossProbability == 100)
            {
                if (manetLossProbability == 100)
                {
                    mode = 0;
                }
                forwardingCount++;
                usleep(1000*40);
            }
            else if(mode == 1)
            {
                break;
            }
        }
        
        // sequenceCache = sequence;
        // string bufString;
        // // sequence:1,source:22.22.22.22:3333,destination:33.33.33.33:2222,previous:66.66.66.66:1001,data:heart:80
        // // sequence:1,22.22.22.22:3333,33.33.33.33:2222,66.66.66.66:1001,heart:80,location:23.456789:34.2342342,air:98,toxic:1
        // // bufString = "sequence:"+to_string(k)+","+localAddress+","+destinationAddress+","+localAddress+",heart:"+to_string(heartRate)
        // //     +",location:"+to_string(location_x)+":"+to_string(location_y)+",air:"+to_string(oxygen)+",toxic:"+to_string(toxic);
        // bufString = ","+localAddress+":"+to_string(localPort)+","+destination_IP+":"+to_string(destination_port)+","+to_string(NODE)+",heart:"+to_string(heartRate)
        //     +",location:"+to_string(location_x)+":"+to_string(location_y)+",air:"+to_string(oxygen)+",toxic:"+to_string(toxic)+","+"mode:";
        // packageTable.push_back(bufString);
        string bufSendingString;

        // cout<<"what's in the package table?"<<endl;
        // for (int ii = 0; ii < packageTable.size(); ii++)
        // {
        //     cout<<packageTable.at(ii)<<endl;
        // }
        // cout<<"==============================="<<endl;
                    

        //cout<<"Broadcasting package..." << endl;
        // generatePackage();
        // cout<<"manet generate package."<<endl;
        if (packageTable.size()<=0 && isBroadcastHello == 0)
        {
            usleep(PACKAGE_SEND_TIME);
            continue;
        }
        // cout<<"manet...isBroadcastHello: "<<isBroadcastHello<<" and packageTable size is "<<packageTable.size()<<endl;
        if(packageTable.size()>0 && isBroadcastHello == 0)
        {
            bufSendingString = "sequence:"+to_string(k) + packageTable.at(0)+to_string(mode)+",";
            // for (int i = 0; i < broadcastList.size(); i++)
            // {
            if (mode == 1  && nodes.size()== NUMBER_OF_NODES)
            {
                /* code */
                // mode 1 is manet
                
                for (int j = 0; j < NUMBER_OF_NODES && j < nodes.size(); j++)
                {
                    // if (nodes.at(j).nodeNum == broadcastList.at(i))
                    // {
                        // cout<<nodes.at(j).IP<<":"<<nodes.at(j).port<<endl;
                        // string destinationAddress = nodes.at(j).IP+":"+to_string(nodes.at(j).port);
                        memset((char *) &remaddr, 0, sizeof(remaddr));
                        remaddr.sin_family = AF_INET;
                        remaddr.sin_port = htons(nodes.at(j).port);
                        if (inet_aton(nodes.at(j).IP.c_str(), &remaddr.sin_addr)==0) 
                        {
                            
                            cout<<"inet_aton failed at mode manet."<<endl;
                            for (int jj = 0; jj < NUMBER_OF_NODES && jj < nodes.size(); jj++)
                            {
                                cout<<"IP: "<<nodes.at(jj).IP<<endl;
                            }
                            cout<<"inet_aton fail info: IP: "<<nodes.at(j).IP<<endl;
                            cout<<"inet_aton fail info: size of 'nodes': "<<nodes.size()<<endl;
                            exit(1);
                        }

                        //cout<<bufString<<endl;
                        //sprintf(buf,"%s",(char *)bufString.c_str());
                        // sprintf(buf, "",)
                        //cout<<"sending package to '"+nodes.at(j).IP+":"+to_string(nodes.at(j).port)+"'"<<endl;
                        //cout<< bufSendingString<<endl;
                        //cout<<"mode 1 thread: mode:"<<mode<<endl;
                        // if (manetLossProbability == 100) // manet is on mode 1
                        // {
                        //     mode = 0;
                        // }
                        //cout<<"mode 1 after thread: mode:"<<mode<<endl;
                        if(gremlin(1) == 0)
                        {
                            // cout<<"calc is loss or not.."<<endl;
                            int fs = fastAndSlowFadingLoss(thisX, thisY, nodes.at(j).x, nodes.at(j).y);
                            // cout<<"fs: "<<fs<<endl;
                            if(fs == 0)
                            {
                                
                                // cout<<"sendto "<<bufSendingString<<endl;
                                if (sendto(fdHub, bufSendingString.data(), bufSendingString.size(), 0, (struct sockaddr *)&remaddr, sizeof(remaddr))==-1) 
                                {
                                    cout<<"sendto failed.. sending"<<endl;
                                    exit(1);
                                }
                                
                            }
                        }
                    // }
                }
                
            // }
            }
            forwardingCount++;
        }
        else if(isBroadcastHello == 1  && nodes.size()== NUMBER_OF_NODES)
        {

            // update tables where expire
            // for (int i = 0; i< one_hop_neighbors_table.size();i++)
            // {
            //     for(int j = 0;j<one_hop_neighbors_table.size();j++)
            //     {
            //         if (time(NULL) - one_hop_neighbors_table.at(j).time > 5)
            //         {
            //             one_hop_neighbors_table.erase(one_hop_neighbors_table.begin()+j);
            //             for(int t = 0; t < two_hop_neighbors_table.size();t++)
            //             {
            //                 if (two_hop_neighbors_table.at(t).access_though == one_hop_neighbors_table.at(j).neighbors_id)
            //                 {
            //                     two_hop_neighbors_table.erase(two_hop_neighbors_table.begin()+t);
            //                     t--;
            //                 }
            //             }
            //         }
            //         break;
            //     }
            // }

            string helloString;

            helloString = "hello:"+to_string(SOURCE_NODE)+",";
            vector<one_hop_neighbors_entry> one_hop_table_copy = one_hop_neighbors_table;
            for (int i = 0; i < one_hop_table_copy.size(); i++)
            {
                helloString = helloString + to_string(one_hop_table_copy.at(i).neighbors_id) + ",";
            }

            helloString = helloString +",";
            // cout<<helloString<<endl;
            // print_one_hop_neighbors_table();
            // print_two_hop_neighbors_table();
            for (int j = 0; j < NUMBER_OF_NODES && j < nodes.size(); j++)
            {
                if (nodes.at(j).nodeNum != SOURCE_NODE)
                {
                    memset((char *) &remaddr, 0, sizeof(remaddr));
                    remaddr.sin_family = AF_INET;
                    remaddr.sin_port = htons(nodes.at(j).port);
                    if (inet_aton(nodes.at(j).IP.c_str(), &remaddr.sin_addr)==0) 
                    {
                        // fprintf(stderr, "inet_aton() failed\n");
                        cout<<"inet_aton() failed...myNode:broadcastHello()"<<endl;
                        cout<<"inet_aton fail info: IP: "<<nodes.at(j).IP<<endl;
                        cout<<"inet_aton fail info: size of 'nodes': "<<nodes.size()<<endl;
                        exit(1);
                    }
                    if(fastAndSlowFadingLoss(thisX, thisY, nodes.at(j).x, nodes.at(j).y) == 0)
                    {
                        // cout<<"send "<<helloString<<endl;
                        if (sendto(fdHub, helloString.data(), helloString.size(), 0, (struct sockaddr *)&remaddr, sizeof(remaddr))==-1) 
                        {
                            cout<<"sendto failed at hello broadcast"<<endl;
                            exit(1);
                        }
                    }
                }
            }
            isBroadcastHello = 0;
            // usleep(TIME);
        }
        // manet is 1
        if(ethernetLossProbability!=100)
        {
            mode = 0;
        }
        if (ethernetLossProbability==100)
        {
            mode = 1;
        }
        // sequence++;
        usleep(PACKAGE_SEND_TIME);
    }
    return 0;
}

void* threadHubReceivePackageOrACK(void* args) // receive
{

    int fdHubReceive;
    unsigned char buf[DATASIZE];
    struct sockaddr_in myaddr, priorAddr;
    socklen_t addressLength = sizeof(priorAddr);
    if ((fdHubReceive = socket(AF_INET, SOCK_DGRAM, 0))==-1)
    {
        cout << "socket 'send' created failed." << endl;
    }

    //bind
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(localPort);

    if (bind(fdHubReceive, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
    for (int i = 0;; i++)
    {
        deleteExpiredNeighbors();
        //cout<<"Waiting or receiving packages..."<<endl;
        int recvLen = recvfrom(fdHubReceive, buf, DATASIZE, 0, (struct sockaddr *)&priorAddress, &addressLength);
        if (recvLen>0)
        {
            // cout<<"received package:"<<endl;
            // cout<<buf<<endl;
            stringstream stream((char *)buf);
            string receivedString="";
            receivedString = stream.str();
            string item;
            string sub_item;
            int j=0;
            string packageType;
            int receivedACKsequence;
            while(getline(stream, item, ','))
            {
                if (j == 0)
                {
                    stringstream sub_stream(item);
                    int t=0;
                    while(getline(sub_stream, sub_item, ':'))
                    {
                        if (t==0)
                        {
                            packageType = sub_item;
                            //cout<<"package type:"<<packageType<<endl;
                            if (packageType == "hello")
                            {
                                handleHello(receivedString);
                            }
                        }
                        if (t==1 && packageType != "hello")
                        {
                            receivedACKsequence = stoi(sub_item);
                        }
                        t++;
                    }
                }
                if (j == 1 && packageType != "hello")
                {
                    if (packageType == "ack")
                    {
                        //cout<<"receivedACKsequence:"<<receivedACKsequence<<endl;
                        if (ackCache < receivedACKsequence)
                        {
                            // cout<<"received ack success..."<<endl;
                            // cout<<"Current package size: "<<packageTable.size()<<endl;
                            // cout<<"ackCache:"<<ackCache<<endl;
                            // cout<<item<<endl;
                            if (packageTable.size()>0)
                            {
                                packageTable.erase(packageTable.begin());
                            }
                            // generatePackage();
                            // cout<<"generate package"<<endl;
                            ackCache = receivedACKsequence;
                            //cout<<"ack cache: "<<ackCache<<endl;
                            //cout<<"sequence cache: "<<k<<endl;
                            //sequence++;
                            // first time received ack
                        }
                        else
                        {
                            // already received ack, discard
                        }
                    }
                    if (packageType == "sequence")
                    {
                        //cout<<"package type is sequence, discard."<<endl;
                    }
                    if (packageType == "heart")
                    {
                        // cout<<"received heart rate: "<<stoi(item)<<endl;
                        heartRate = stoi(item);
                    }
                    if (packageType == "location")
                    {

                        // cout<<"received location: "<<item<<endl;
                        stringstream sub_stream(item);
                        int t=0;
                        while(getline(sub_stream, sub_item, ':'))
                        {
                            if (t==0)
                            {
                                stringstream sub_item_stream(sub_item);
                                // sub_item_stream >> location_x;  
                                location_x = stod(sub_item_stream.str());
                            }
                            if (t==1)
                            {
                                stringstream sub_item_stream(sub_item);
                                location_y = stod(sub_item_stream.str());
                                // sub_item_stream >> location_y;  
                            }
                            t++;
                        }
                    }
                    if (packageType == "oxygen")
                    {
                        // cout<<"received oxygen level: "<<item<<endl;
                        stringstream item_stream(item);
                        item_stream >> oxygen;  
                    }
                    if (packageType == "toxic")
                    {
                        // cout<<"received toxic: "<<item<<endl;
                        stringstream item_stream(item);
                        item_stream >> toxic;
                    }
                }
                j++;
            }
        }
    }

    return 0;
}


void* threadEthernetSend(void* args) // sender from vector // mode 0 // ethernet mode
{
    struct sockaddr_in myaddr, remaddr;
    int fdHub;
    unsigned char buf[DATASIZE];
    int recvLen;
    char * server;

    if ((fdHub=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        cout<<"socket created"<<endl;

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);

    if (bind(fdHub, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }

    for (;; k++) //ethernet mode 0
    {
        for (;;)
        {   
            // if mode is 1 manet mode then
            //cout<<"ethernet send thread: "<<mode<<endl;

            if (ethernetLossProbability != 100 && manetLossProbability == 100 )
            {
                mode = 0;
                break;
            }

            if ((mode==1) || (ethernetLossProbability == 100))
            {
                if (ethernetLossProbability == 100)
                {
                    mode = 1;
                }
                usleep(1000*40);
            }
            else if(mode == 0)
            {
                break;
            }
        }
        // string bufString;
        // bufString = ","+localAddress+":"+to_string(localPort)+","+destination_IP+":"+to_string(destination_port)+","+to_string(NODE)+",heart:"+to_string(heartRate)
        //     +",location:"+to_string(location_x)+":"+to_string(location_y)+",air:"+to_string(oxygen)+",toxic:"+to_string(toxic)+",mode:";
        // packageTable.push_back(bufString);
        // generatePackage();
        // cout<<"ethernet generate package."<<endl;
        if (packageTable.size()<=0)
        {
            usleep(PACKAGE_SEND_TIME);
            continue;
        }
        if(packageTable.size()>0)
        {
            string bufSendingString;
            bufSendingString = "sequence:"+to_string(k) + packageTable.at(0)+to_string(mode)+",";

            //cout<<"what's in the package table?"<<endl;
            //for (int ii = 0; ii < packageTable.size(); ii++)
            //{
            //    cout<<packageTable.at(ii)<<endl;
            //}
            //cout<<"==============================="<<endl;
                        

            // cout<<"Broadcasting package..." << endl;

            // cout<<nodes.at(j).IP<<":"<<nodes.at(j).port<<endl;
            // string destinationAddress = nodes.at(j).IP+":"+to_string(nodes.at(j).port);
            memset((char *) &remaddr, 0, sizeof(remaddr));
            remaddr.sin_family = AF_INET;
            remaddr.sin_port = htons(destination_port);
            if (inet_aton(destination_IP.c_str(), &remaddr.sin_addr)==0) 
            {
                cout<<"inet_aton() failed at threadEthernetSend"<<endl;
                exit(1);
            }
            // else
            // {
            //     cout<<"inet_aton() success! "<<endl;
            // }

            //cout<<bufString<<endl;
            //sprintf(buf,"%s",(char *)bufString.c_str());
            // sprintf(buf, "",)
            
            // cout<< bufSendingString<<endl;
            //cout<<"mode 0 thread: mode:"<<mode<<endl;
            // if (ethernetLossProbability == 100) // ethernet is on mode 0
            // {
            //     mode = 1;
            // }
            if (gremlin(0)==0)
            {
                if (mode == 0)
                {
                    
                    if (sendto(fdHub, bufSendingString.data(), bufSendingString.size(), 0, (struct sockaddr *)&remaddr, sizeof(remaddr))==-1) 
                    {
                        perror("sendto");
                        exit(1);
                    }
                }
            }
        }
        if(manetLossProbability != 100)
        {    
            mode = 1;
        }
        // sequence++;
        usleep(PACKAGE_SEND_TIME);

    }
    return 0;
}

void* waitingCinThread(void* args)
{
    // mode 0
    // mode 1
    // gremlin 30  //(0~100)
    for (int i = 0; ; ++i)
    {
        string commandString;
        cout<<">>";
        // cin >> commandString;
        getline(cin, commandString);
        cout << commandString << endl;
        string item;
        stringstream stream(commandString);
        int t=0;
        string commandType;
        while(getline(stream, item, ' '))
        {
            if (t==0)
            {
                commandType = item;
                // cout<<commandType<<endl;
                if (commandType == "package_table")
                {
                    cout<<"what's in the package table?"<<endl;
                    for (int ii = 0; ii < packageTable.size(); ii++)
                    {
                        cout<<packageTable.at(ii)<<endl;
                    }
                    cout<<"====================================================="<<endl;
                }
                if (commandType == "one_hop_neighbors_table")
                {
                    print_one_hop_neighbors_table();
                }
                if (commandType == "two_hop_neighbors_table")
                {
                    print_two_hop_neighbors_table();
                }
                else if (commandType == "get")
                {
                    cout<<"nodes size: "<<nodes.size()<<endl;
                    for (int i = 0; i < nodes.size(); i++)
                    {
                        cout<<nodes.at(i).nodeNum<<" ";
                        if (nodes.at(i).nodeNum == DESTINATION_NODE)
                        {
                            int x2 = nodes.at(i).x;
                            int y2 = nodes.at(i).y;
                            cout<<"distance: "<<sqrt(abs(thisX-x2)*abs(thisX-x2) + abs(thisY-y2)*abs(thisY-y2));
                            break;
                        }
                    }
                    cout<<endl;
                }
                else if (commandType == "gets")
                {
                    cout<<"nodes size: "<<nodes.size()<<endl;
                    for (int i = 0; i < nodes.size(); i++)
                    {
                        int x2 = nodes.at(i).x;
                        int y2 = nodes.at(i).y;
                        double d = sqrt(abs(thisX-x2)*abs(thisX-x2) + abs(thisY-y2)*abs(thisY-y2));
                        if(d < 100)
                        {
                            cout<<nodes.at(i).nodeNum<<": "<<d<<endl;
                        }
                    }
                    cout<<endl;
                }
            }
            if (t==1)
            {
                // cout<<item<<endl;
                if (commandType == "mode")
                {
                    if ((stoi(item)==0) || (stoi(item)==1))
                    {
                        mode = stoi(item);
                    }
                    else
                    {
                        cout<<"mode: args error"<<endl;
                    }
                }
                else if (commandType == "ethernet")
                {
                    if ((stoi(item)>=0) && (stoi(item)<=100))
                    {
                        ethernetLossProbability = stoi(item);
                    }
                    else
                    {
                        cout<<"gremlin: args error"<<endl;
                    }
                }
                else if (commandType == "manet")
                {
                    if ((stoi(item)>=0) && (stoi(item)<=100))
                    {
                        manetLossProbability = stoi(item);
                    }
                    else
                    {
                        cout<<"gremlin: args error"<<endl;
                    }
                }

            }
            t++;
        }

        cout<<"mode: "<<mode<<endl;
        cout<<"package table size: "<<packageTable.size()<<endl;
        cout<<"ethernetLossProbability: "<<ethernetLossProbability<<endl;
        cout<<"manetLossProbability: "<<manetLossProbability<<endl;
    }
}


void* updateLocation(void* args)
{
    string str;
    int count = 0;
    for(;;)
    {
        // cout<<"updating location"<<endl;
        ifstream referenceStringFile(MANET_FILE);

        if (!referenceStringFile)
        {
            cout<<"No reference string file found!"<<endl;
            return 0;
        }
        else
        {
            char line[100];
            vector <node> updateNodes;
            int isNullLine = 0;
            while (referenceStringFile.getline(line, sizeof(line)))
            {
                node myNode;
                stringstream stream(line);
                string item;
                int i = 0;
                while(getline(stream, item, ' '))
                {
                    if (i == 0)
                    {
                        if (item != "Node")
                        {
                            isNullLine = 1;
                        }
                        else if (item == "Node")
                        {
                            isNullLine = 0;
                        }
                        // cout<<"First item: "<<item<<endl;
                    }
                    if (i == 1 && isNullLine == 0)
                    {
                        myNode.nodeNum = stoi(item);
                        // cout<<"Second item: "<<item<<endl;
                    }
                    if (i == 2 && isNullLine == 0)
                    {
                         // IP
                        // cout<<"IP before erase: "<<item<<endl;
                        item.erase(item.end()-1); // remove ',' at the end of the 'tux78,' for example.
                        myNode.IP = getRealIP(item);
                    }
                    if (i == 3 && isNullLine == 0)
                    {
                        myNode.port = stoi(item);
                    }
                    if (i == 4 && isNullLine == 0) // x
                    {
                        myNode.x = stoi(item);
                        if (myNode.nodeNum == SOURCE_NODE)
                        {
                            thisX = myNode.x;
                        }
                    }
                    if (i == 5 && isNullLine == 0) // y
                    {
                        myNode.y = stoi(item);
                        if (myNode.nodeNum == SOURCE_NODE)
                        {
                            thisY = myNode.y;
                        }
                    }
                    if (i > 6 && isNullLine == 0)
                    {
                        myNode.links.push_back(stoi(item));
                        if (myNode.nodeNum == SOURCE_NODE)
                        {
                            broadcastList.push_back(stoi(item));
                        }
                    }
                    i++;
                }
                //cout<<endl;
                if (isNullLine == 0)
                {
                    updateNodes.push_back(myNode);
                }
            }
            nodes = updateNodes;
        }
        usleep(1000*500);
    }
    return 0;
}



int main()
{
    init_setup();
    cout<<"starting..."<<endl;
    cout<<5<<endl;
    usleep(1000*1000);
    cout<<4<<endl;
    usleep(1000*1000);
    cout<<3<<endl;
    usleep(1000*1000);
    cout<<2<<endl;
    usleep(1000*1000);
    cout<<1<<endl;
    usleep(1000*1000);
    cout<<0<<endl;
    pthread_t tids[6];
    pthread_create(&tids[0], NULL, waitingCinThread,NULL);
    pthread_create(&tids[1], NULL, threadEthernetSend,NULL);
    pthread_create(&tids[2], NULL, threadHubReceivePackageOrACK,NULL);
    pthread_create(&tids[3], NULL, threadCreatePackageAndBroadcast,NULL);
    // pthread_create(&tids[4], NULL, changeModeThread, NULL);
    pthread_create(&tids[4], NULL, generatePackage, NULL);
    pthread_create(&tids[5], NULL, updateLocation, NULL);
    // pthread_create(&tids[6], NULL, broadcastHello, NULL);
    //pthread_create(&tids[5], NULL, modeChangingThread, NULL);
    pthread_exit(NULL);

    return 0;
}

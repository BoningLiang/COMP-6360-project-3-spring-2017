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
#define TIME 10000

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
string helloStringToSend;
typedef struct{
    int nodeNum;
    string IP;
    int port;
    int x;
    int y;
    vector <int> links;
} node;

typedef struct{
    int nodeNum;
    int isMPR;
} mpr_entry;

vector <mpr_entry> mpr_table;

typedef struct{
    string packageContent;
    int previousNodeNumber;
} package_struct;

int NODE;
vector <node> nodes;
int locker=0;

vector <package_struct> packageTable;
int thisX;
int thisY;
int global_port = 0;
int sequence = 1;
struct sockaddr_in myAddress;
struct sockaddr_in priorAddress;
struct sockaddr_in terminalAddress;
struct sockaddr_in sourceAddress;
struct sockaddr_in nextAddress;
string localAddress;
// string destinationAddress = "127.0.0.1:10013";
string previousAddressString;
string localIP;
int localPort;
int sequenceCache = 0;
int ackCache = 0;
int fdSend;
int fdRecv;
vector <int> broadcastList; // node numbers to broadcast

int heartRate = -1;
int location_x = 999;
int location_y = 999;
int oxygen = -1;
int toxic = -1;

int stoi(string args)
{
    return atoi(args.c_str());
}

string to_string(int i)
{
    stringstream ss;
    ss << i;
    return ss.str();
}

int isMPR(int nodeNumber)
{
    for (int i = 0; i < mpr_table.size(); i++)
    {
        if (mpr_table.at(i).nodeNum == nodeNumber && mpr_table.at(i).isMPR == 1)
        {
            return 1;
        }
    }
    return 0;
}

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
        int isNullLine = 0;
        while (referenceStringFile.getline(line, sizeof(line)))
        {
            node myNode;
            // sscanf(line,
            //     "Node %d %[^,], %d %d %d links %[^*]",
            //     &myNode.nodeNum, myNode.IP, &myNode.port, &myNode.x, &myNode.y, myNode.links);
            stringstream stream(line);
            string item;
            int i = 0;
            while(getline(stream, item, ' '))
            {
                //cout<<item<<endl;
                if (i == 0)
                {
                    if (item == "Node")
                    {
                        isNullLine = 0;
                    }
                    else if (item != "Node")
                    {
                        isNullLine = 1;
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
                    if (myNode.nodeNum == NODE)
                    {
                        localIP = myNode.IP;
                    }
                    // cout<<myNode.IP<<endl;
                }
                if (i == 3)
                {
                    myNode.port = stoi(item);
                    if (myNode.nodeNum == NODE)
                    {
                        localPort = stoi(item);
                        localAddress = localIP +":"+ item;
                    }
                }
                if (i == 4) // x
                {
                    myNode.x = stoi(item);
                    if (myNode.nodeNum == NODE)
                    {
                        thisX = myNode.x;
                    }
                }
                if (i == 5) // y
                {
                    myNode.y = stoi(item);
                    if (myNode.nodeNum == NODE)
                    {
                        thisY = myNode.y;
                    }
                    // cout<<myNode.y<<endl;
                }
                if (i >6)
                {
                    myNode.links.push_back(stoi(item));
                    if (myNode.nodeNum == NODE)
                    {
                        broadcastList.push_back(stoi(item));
                    }
                }
                i++;
            }
            //cout<<endl;
            if (isNullLine == 0)
            {
                nodes.push_back(myNode);
            }
        }

    }

    for (int i = 0; i < nodes.size(); i++)
    {
        mpr_entry entry;
        entry.nodeNum = nodes.at(i).nodeNum;
        entry.isMPR = 0;
        mpr_table.push_back(entry);
    }

    return 0;
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

int fastAndSlowFadingLoss(int x1, int y1, int x2, int y2)
{
    double distance = sqrt(abs(x1-x2)*abs(x1-x2) + abs(y1-y2)*abs(y1-y2));
    int p = 100;
    if (distance<=100)
    {
        p = 100 ;
    }
    else
    {
        p = 0;
    }

    return gremlin(100-p);
}

int print_one_hop_neighbors_table()
{   
    cout<< "============================================================================"<<endl;
    cout<< "Neighbor's ID\t\tState of Link\t\tTime"<<endl;
    for (int i = 0; i < one_hop_neighbors_table.size(); i++)
    {
        cout<<one_hop_neighbors_table.at(i).neighbors_id<<"\t\t\t"<<one_hop_neighbors_table.at(i).state_of_link<<"\t\t\t"
            <<one_hop_neighbors_table.at(i).time<<endl;
    }
    cout<< "============================================================================"<<endl;
    return 0;
}

int print_two_hop_neighbors_table()
{
    cout<< "============================================================================"<<endl;
    cout<<"2-hop ID"<<"\t\t"<<"Access Though"<<endl;
    for (int i = 0; i < two_hop_neighbors_table.size(); i++)
    {
        cout<<two_hop_neighbors_table.at(i).neighbors_id<<"\t\t\t"<<two_hop_neighbors_table.at(i).access_though<<"\t\t\t"
            <<two_hop_neighbors_table.at(i).time<<endl;
    }
    cout<< "============================================================================"<<endl;
    cout<<endl;
    return 0;
}


// only the hub have this thread
void* threadNodeBroadcastPackageFromCache(void* args) //send // forwarding
{
    // hub
    // first send
    // if received the ACK, then broadcast next package
    // if not received the ACK, then re-send the package
    // cout<<"threadNodeBroadcastPackageFromCache...init"<<endl;
    struct sockaddr_in myaddr, remaddr;
    int fdHub;
    unsigned char buf[DATASIZE];
    int recvLen;
    char * server;
    // cout<<"threadNodeBroadcastPackageFromCache...2"<<endl;
    if ((fdHub=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        cout<<"socket created"<<endl;

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);
    // cout<<"threadNodeBroadcastPackageFromCache...3"<<endl;
    if (bind(fdHub, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
    int sendHello=0;
    int isBroadcastHello = 1;
    int forwardingCount = 0;
    // cout<<"threadNodeBroadcastPackageFromCache...4"<<endl;
    for (int k = 0;; k++)
    {
        forwardingCount++;
        if (forwardingCount>10)
        {
            isBroadcastHello = 1;
            forwardingCount = 0;
        }

        if (packageTable.size()>0 && isBroadcastHello == 0  && nodes.size()== NUMBER_OF_NODES)
        {
            // string bufString;
            // // sequence:1,source:22.22.22.22:3333,destination:33.33.33.33:2222,previous:66.66.66.66:1001,data:heart:80
            // // sequence:1,22.22.22.22:3333,33.33.33.33:2222,66.66.66.66:1001,heart:80,location:23.456789:34.2342342,air:98,toxic:1
            // bufString = "sequence:"+to_string(sequence)+","+localAddress+","+destinationAddress+","+localAddress+",heart:"+to_string(heartRate)
            //     +",location:"+to_string(location_x)+":"+to_string(location_y)+",air:"+to_string(oxygen)+",toxic:"+to_string(toxic);
            // packageTable.push_back(bufString);
            string bufSendingString;
            int previousHopNumber;
            previousHopNumber = packageTable.at(0).previousNodeNumber;
            bufSendingString = packageTable.at(0).packageContent;

            // cout<<"what's in the package table?"<<endl;
            // for (int ii = 0; ii < packageTable.size(); ii++)
            // {
            //     cout<<packageTable.at(ii).packageContent<<endl;
            // }
            // cout<<"==============================="<<endl;

            if (packageTable.size()>0)
            {
                packageTable.erase(packageTable.begin());
            }
            for (int j = 0; j < NUMBER_OF_NODES && j < nodes.size() ; j++)
            {
                if ((nodes.at(j).nodeNum != previousHopNumber) && (nodes.at(j).nodeNum != NODE) && (isMPR(nodes.at(j).nodeNum)==1 ))
                {
                    memset((char *) &remaddr, 0, sizeof(remaddr));
                    remaddr.sin_family = AF_INET;
                    remaddr.sin_port = htons(nodes.at(j).port);
                    if (inet_aton(nodes.at(j).IP.c_str(), &remaddr.sin_addr)==0) 
                    {
                        // fprintf(stderr, "inet_aton() failed\n");
                        cout<<"inet_aton() failed...myNode:threadNodeBroadcastPackageFromCache()"<<endl;
                        exit(1);
                    }

                    if(fastAndSlowFadingLoss(thisX, thisY, nodes.at(j).x, nodes.at(j).y) == 0)
                    {
                        // cout<<"sending to "<<nodes.at(j).nodeNum<<endl;
                        // cout<<"sending... "<<bufSendingString<<endl;
                        if (sendto(fdHub, bufSendingString.data(), bufSendingString.size(), 0, (struct sockaddr *)&remaddr, sizeof(remaddr))==-1) 
                        {
                            cout<<"sendto failed... myNode:threadNodeBroadcastPackageFromCache"<<endl;
                            exit(1);
                        }
                    }
                }
            }
            sendHello = 0;
            usleep(TIME);
        }
        else if(isBroadcastHello == 1  && nodes.size()== NUMBER_OF_NODES)
        {
            // string helloString;

            
            // locker = 1;
            // vector<one_hop_neighbors_entry> one_hop_table_copy = one_hop_neighbors_table;
            // locker = 0;
            // helloString = "hello:"+to_string(NODE)+",";
            // for (int i = 0; i < one_hop_table_copy.size(); i++)
            // {
            //     helloString = helloString + to_string(one_hop_table_copy.at(i).neighbors_id) + ",";
            // }
            // helloString = helloString +",";
            // cout<<helloString<<endl;
            for (int j = 0; j < NUMBER_OF_NODES; j++)
            {
                if (nodes.at(j).nodeNum != NODE)
                {
                    memset((char *) &remaddr, 0, sizeof(remaddr));
                    remaddr.sin_family = AF_INET;
                    remaddr.sin_port = htons(nodes.at(j).port);
                    if (inet_aton(nodes.at(j).IP.c_str(), &remaddr.sin_addr)==0) 
                    {
                        // fprintf(stderr, "inet_aton() failed\n");
                        cout<<"inet_aton() failed...myNode:broadcastHello()"<<endl;
                        exit(1);
                    }
                    if(fastAndSlowFadingLoss(thisX, thisY, nodes.at(j).x, nodes.at(j).y) == 0)
                    {
                        // cout<<"send "<<helloString<<endl;
                        if (helloStringToSend.length()>0)
                        {
                            if (sendto(fdHub, helloStringToSend.data(), helloStringToSend.size(), 0, (struct sockaddr *)&remaddr, sizeof(remaddr))==-1) 
                            {
                                perror("sendto failed...myNode:broadcastHello()");
                                exit(1);
                            }
                        }
                    }
                }
            }
            isBroadcastHello = 0;
            if (sendHello > 0)
            {
                usleep(1000*1000);
            }
            else
            {
                usleep(TIME);
            }
            sendHello++;
        }
    }
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

int update_mpr_entry(int update_one_hop_neighbor_id, string state_of_link)
{
    for (int i = 0; i < one_hop_neighbors_table.size(); i++)
    {
        if (one_hop_neighbors_table.at(i).neighbors_id == update_one_hop_neighbor_id)
        {
            if(one_hop_neighbors_table.at(i).state_of_link == "MPR" && state_of_link == "uni-directional")
            {
                one_hop_neighbors_table.at(i).state_of_link = state_of_link;
                // one_hop_neighbors_table.at(i).time = time(NULL);
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

int one_hop_table_update_mpr(int id)
{
    for (int i = 0; i < one_hop_neighbors_table.size(); i++)
    {
        if (one_hop_neighbors_table.at(i).neighbors_id == id)
        {
            one_hop_neighbors_table.at(i).state_of_link = "MPR";
            // one_hop_neighbors_table.at(i).time = time(NULL);
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
        // cout<<unCovered.at(i).neighbors_id<<endl;
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
    // print out one-hop neighbors which are connected with the un-covered neighbors
    // for (int i = 0; i < one_hop_neighbors_un_covered_neighbors.size(); i++)
    // {
        // cout<<one_hop_neighbors_un_covered_neighbors.at(i)<<endl;
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
        if(maxID != -1)
        {
            one_hop_table_update_mpr(maxID);
        }
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
    // cout<<"parsing hello: "<<helloString<<endl;
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



int updateMPRtable()
{
    for (int i = 0; i < mpr_table.size(); i++)
    {
        for (int j = 0; j < one_hop_neighbors_table.size(); j++)
        {
            if (one_hop_neighbors_table.at(j).state_of_link == "MPR" 
                && mpr_table.at(i).nodeNum == one_hop_neighbors_table.at(j).neighbors_id)
            {
                mpr_table.at(i).isMPR = 1;
                break;
            }
            else if (mpr_table.at(i).nodeNum == one_hop_neighbors_table.at(j).neighbors_id)
            {
                mpr_table.at(i).isMPR = 0;
                break;
            }
        }
    }
    return 0;
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
                one_hop_neighbors_table.at(i).time = time(NULL);
            }
            else
            {
                one_hop_neighbors_table.at(i).time = time(NULL);
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
                two_hop_neighbors_table.at(j).time = time(NULL);
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

    // if (isModified == 1)
    // {
        update_mpr();
    // }

    print_one_hop_neighbors_table();
    cout<<endl;
    print_two_hop_neighbors_table();
    vector<one_hop_neighbors_entry> one_hop_table_copy = one_hop_neighbors_table;

    helloStringToSend = "hello:"+to_string(NODE)+",";
    for (int i = 0; i < one_hop_table_copy.size(); i++)
    {
        helloStringToSend = helloStringToSend + to_string(one_hop_table_copy.at(i).neighbors_id) + ",";
    }
    helloStringToSend = helloStringToSend +",";

    updateMPRtable();

    return 0;
}

void* threadNodeReceivePackageOrACKThenForward(void* args) //receive and push package into cache 
{

    // cout<<"threadNodeReceivePackageOrACKThenForward...1"<<endl;
    int fdHubReceive;
    unsigned char buf[DATASIZE];
    struct sockaddr_in myaddr, priorAddr;
    socklen_t addressLength = sizeof(priorAddr);
    if ((fdHubReceive = socket(AF_INET, SOCK_DGRAM, 0))==-1)
    {
        cout << "socket 'send' created failed." << endl;
    }
    // cout<<"threadNodeReceivePackageOrACKThenForward...2"<<endl;
    //bind
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(localPort);
    // cout<<"local port:"<<localPort<<endl;
    // cout<<"threadNodeReceivePackageOrACKThenForward...3"<<endl;
    if (bind(fdHubReceive, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        cout<<"bind failed"<<endl;
        return 0;
    }
    // cout<<"threadNodeReceivePackageOrACKThenForward...4"<<endl;
    for (int i = 0;; i++)
    {
        deleteExpiredNeighbors();
        // cout<<"c: "<<c<<endl;
        // cout<<"Waiting or receiving packages..."<<endl;
        int recvLen = recvfrom(fdHubReceive, buf, DATASIZE, 0, (struct sockaddr *)&priorAddress, &addressLength);
        if (recvLen>0)
        {
            // cout<<"recvLen: "<<recvLen<<endl;
            // cout<<buf<<endl;
            stringstream stream((char *)buf);
            string receivedString = stream.str();
            string item;
            string sub_item;
            int j=0;
            string packageType;
            string packageContent;
            int isPush_back=0;
            int receivedSequence;
            while(getline(stream, item, ','))
            {
                if (j == 0)
                {
                    stringstream sub_stream(item);
                    int t=0;
                    packageContent = item + ",";
                    while(getline(sub_stream, sub_item, ':'))
                    {
                        if (t==0)
                        {
                            packageType = sub_item;
                            // cout<<"package tyep: "<<packageType<<endl;
                            if (packageType == "hello")
                            {
                                // cout<<"handling hello"<<endl;
                                handleHello(receivedString);
                            }
                        }
                        if (t==1 && packageType != "hello")
                        {
                            receivedSequence = stoi(sub_item);
                        }
                        t++;
                    }
                }
                if (j==1 && packageType != "hello")
                {
                    //source
                    packageContent = packageContent + item+",";
                    // cout<<"@@@@@@@@"<<packageType<<":"<<item<<endl;
                    // cout<<"package type:"<<packageType<<endl;
                    // cout<<"ackCache: "<<ackCache<<endl;
                    // cout<<"sequenceCache: "<<sequenceCache<<endl;
                    // cout<<"receivedSequence: "<<receivedSequence<<endl;
                    if (packageType == "ack")
                    {
                        // cout<<"ackCache: "<<ackCache<<",receivedSequence:"<<receivedSequence<<endl;
                        if (ackCache < receivedSequence)
                        {
                            isPush_back=1;
                            ackCache = receivedSequence;
                            // not yet received ack
                        }
                        else
                        {
                            isPush_back=0;
                            // already received ack, discard
                        }
                    }
                    if (packageType == "sequence")
                    {
                        // cout << "sequenceCache: "<<sequenceCache<<endl;
                        // cout << "sequence: "<<receivedSequence<<endl;
                        if (sequenceCache < receivedSequence)
                        {
                            // cout<<item<<endl;
                            isPush_back=1;
                            sequenceCache = receivedSequence;
                        }
                        else
                        {
                            isPush_back=0;
                            // discard
                        }
                    }
                }
                if (j==2 && packageType != "hello")
                {
                    // destination
                    packageContent = packageContent + item + ",";
                }
                if (j==3 && packageType != "hello")
                {
                    // previous hop number
                    // previous address will be changed
                    previousAddressString = item;
                    // cout<<"previousAddressString: "<<previousAddressString<<endl;
                    packageContent = packageContent + to_string(NODE) + ",";
                }
                if (j==4 && packageType == "sequence" && packageType != "hello")
                {
                    packageContent = packageContent + item + ",";
                }
                if (j==5&& packageType == "sequence"&& packageType != "hello")
                {
                    packageContent = packageContent + item + ",";
                }
                if (j==6&& packageType == "sequence"&& packageType != "hello")
                {
                    packageContent = packageContent + item + ",";
                }
                if (j==7&& packageType == "sequence"&& packageType != "hello")
                {
                    packageContent = packageContent + item + ",";
                }
                if (j==8 && packageType == "sequence"&& packageType != "hello")
                {
                    // mode
                    packageContent = packageContent + item+",";
                }
                j++;
            }
            // cout<<"package content: "<<packageContent<<endl;
            // cout<<"isPush_back: "<<isPush_back<<endl;
            //push_back
            if (isPush_back == 1 && packageType != "hello")
            {
                isPush_back = 0;
                // ready for forward packages
                // cout << "Pushing packet into cache table..." << endl;
                // cout << packageContent << endl;
                package_struct myPackage;
                myPackage.packageContent = packageContent;
                myPackage.previousNodeNumber = stoi(previousAddressString);
                packageTable.push_back(myPackage);
            }

        }
    }

    return 0;
}

void* updateLocation(void* args)
{
    string str;
    int count = 0;
    // cout<<"updateLocation...1"<<endl;
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
            vector <node> updateNodes;  // ?
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
                    }
                    if (i == 1)
                    {
                        myNode.nodeNum = stoi(item);
                    }
                    if (i == 2)
                    {
                         // IP
                        item.erase(item.end()-1); // remove ',' at the end of the 'tux78,' for example.
                        myNode.IP = getRealIP(item);
                    }
                    if (i == 3)
                    {
                        myNode.port = stoi(item);
                    }
                    if (i == 4) // x
                    {
                        myNode.x = stoi(item);
                        if (myNode.nodeNum == NODE)
                        {
                            thisX = myNode.x;
                        }
                    }
                    if (i == 5) // y
                    {
                        myNode.y = stoi(item);
                        if (myNode.nodeNum == NODE)
                        {
                            thisY = myNode.y;
                        }
                    }
                    if (i >6)
                    {
                        myNode.links.push_back(stoi(item));
                        if (myNode.nodeNum == NODE)
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
            if (updateNodes.size() == NUMBER_OF_NODES)
            {
                nodes = updateNodes;
            }

            // nodes = updateNodes;
        }
        // if (updateNodes.size() == NUMBER_OF_NODES)
        // {
        //     nodes = updateNodes;
        // }
        usleep(1000*500);
    }
    return 0;
}



int main(int argc, char const *argv[])
{
    NODE = stoi(argv[1]);
    init_setup();
    usleep(1000*200);
    pthread_t tids[3];
    pthread_create(&tids[0], NULL, threadNodeReceivePackageOrACKThenForward,NULL);
    pthread_create(&tids[1], NULL, threadNodeBroadcastPackageFromCache,NULL);
    pthread_create(&tids[2], NULL, updateLocation,NULL);
    // pthread_create(&tids[3], NULL, broadcastHello,NULL);
    pthread_exit(NULL);

    return 0;
}

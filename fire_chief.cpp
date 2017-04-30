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

#define TIME 1000*100

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

int global_port = 0;
int sequence = 1;
struct sockaddr_in myAddress;
struct sockaddr_in priorAddress;
struct sockaddr_in terminalAddress;
struct sockaddr_in sourceAddress;
struct sockaddr_in nextAddress;
string sourceAddressString;
int mode=0;
string localAddress; // the localAddress will init in init_setup()
// string destinationAddress = "127.0.0.1:10013"; // useless
string localIP; // the localIP will init in init_setup()
string sourceIP;
int localPort;
int source_port;
int sequenceCache = 0;
int ackCache = 0;
int fdSend;
int fdRecv;
vector <int> broadcastList; // node numbers to broadcast
int thisX;
int thisY;
int heartRate = -1;
int location_x = 999;
int location_y = 999;
int oxygen = -1;
int toxic = -1;

string heart_display_IP;
string oxygen_display_IP;
string location_display_IP;
string toxic_display_IP;

int heart_display_port = 10136;
int oxygen_display_port = 10137;
int location_display_port = 10138;
int toxic_display_port = 10139;

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
        char line[2048];
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
        cout<<"No reference string file found on init_setup!"<<endl;
        return 0;
    }
    else
    {
        char line[2048];
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
                if (i == 1 && isNullLine == 0)
                {
                    myNode.nodeNum = stoi(item);
                    // cout<<myNode.nodeNum<<endl;
                }
                if (i == 2 && isNullLine == 0 )
                {
                    // IP
                    item.erase(item.end()-1); // remove ',' at the end of the 'tux78,' for example.
                    myNode.IP = getRealIP(item);
                    //myNode.IP = item;
                    // cout<<"real ip: "<<myNode.IP<<endl;
                    if (myNode.nodeNum == DESTINATION_NODE)
                    {
                        localIP = myNode.IP;
                        heart_display_IP = localIP;
                        oxygen_display_IP=localIP;
                        location_display_IP=localIP;
                        toxic_display_IP=localIP;
                        cout<<"localIP: "<<localIP<<endl;
                    }
                    // cout<<myNode.IP<<endl;
                }
                if (i == 3 && isNullLine == 0)
                {
                    myNode.port = stoi(item);
                    if (myNode.nodeNum == DESTINATION_NODE)
                    {
                        localPort = stoi(item);
                        localAddress = localIP +":"+ item;
                    }
                }
                if (i == 4 && isNullLine == 0) // x
                {
                    myNode.x = stoi(item);
                    if (myNode.nodeNum == DESTINATION_NODE)
                    {
                        thisX = myNode.x;
                    }
                }
                if (i == 5 && isNullLine == 0) // y
                {
                    myNode.y = stoi(item);
                    if (myNode.nodeNum == DESTINATION_NODE)
                    {
                        thisY = myNode.y;
                    }
                    // cout<<myNode.y<<endl;
                }
                if (i >6 && isNullLine == 0)
                {
                    myNode.links.push_back(stoi(item));
                    if (myNode.nodeNum == DESTINATION_NODE)
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
    if (distance<100)
    {
        p = -0.05 * distance + 100 ;
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
    for (int i = 0; i < unCovered.size(); i++)
    {
        cout<<unCovered.at(i).neighbors_id<<endl;
    }

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
    cout<<endl;
    // print out one-hop neighbors which are connected with the un-covered neighbors
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
                    if (unCovered.at(j).neighbors_id == twoHopID && unCovered.size()>0 && j < unCovered.size())
                    {
                        unCovered.erase(unCovered.begin() + j);
                        j--; // maybe
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
        if (i>=myHello.neighbors.size())
        {
            cout<<"error 1"<<endl;
            exit(1);
        }
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
    cout<<endl;
    // print_two_hop_neighbors_table();

    return 0;
}

// only the hub have this thread
void* threadNodeBroadcastPackageFromCache(void* args) //send
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

    for (int k = 0;; k++)
    {
        if (forwardingCount>=10)
        {
            isBroadcastHello = 1;
            forwardingCount=0;
        }
        
        if (packageTable.size()>0 && isBroadcastHello == 0)
        {
            string bufSendingString;
            bufSendingString = packageTable.at(0);
            //cout<<"bufSendingString: "<<endl;
            //cout<<bufSendingString<<endl;
            stringstream stream(bufSendingString);
            int t=0;
            string item;
            while(getline(stream, item, ','))
            {
                if (t==2)
                {
                    // cout<<"destination address: "+item<<endl;
                    stringstream sub_stream(item);
                    int tt=0;
                    string sub_item;
                    while(getline(sub_stream, sub_item, ':'))
                    {
                        if (tt==0)
                        {
                            sourceIP = sub_item;
                            // cout<<"IP: "<<sourceIP<<endl;
                        }
                        if (tt==1)
                        {
                            source_port = stoi(sub_item);
                            // cout<<"Port: "<<source_port<<endl;
                        }
                        tt++;
                    }
                }
                if (t==4)
                {
                    stringstream sub_stream(item);
                    int tt=0;
                    string sub_item;
                    while(getline(sub_stream, sub_item, ':'))
                    {
                        if (tt==1)
                        {
                            // cout<<"broadcast mode:"<<stoi(sub_item)<<endl;
                            mode = stoi(sub_item);
                        }
                        tt++;
                    }
                }
                t++;
            }
            if (packageTable.size()>0)
            {
                packageTable.erase(packageTable.begin());
            }
            
            //cout<<"mode:"<<mode<<endl;
            if (mode == 0)
            {
                memset((char *) &remaddr, 0, sizeof(remaddr));
                remaddr.sin_family = AF_INET;
                remaddr.sin_port = htons(source_port);
                //cout<<sourceIP<<endl;
                if (inet_aton(sourceIP.c_str(), &remaddr.sin_addr)==0) 
                {
                    // fprintf(stderr, "inet_aton() failed 1.\n");
                    cout<<"inet_aton failed 1."<<endl;
                    exit(1);
                }

                cout<<" ethernet sending package..."<< bufSendingString << endl;
                if (sendto(fdHub, bufSendingString.data(), bufSendingString.size(), 0, (struct sockaddr *)&remaddr, sizeof(remaddr))==-1) 
                {
                    // perror("sendto");
                    cout<<"sendto failed 1."<<endl;
                    exit(1);
                }
            }
            else if (mode == 1 && nodes.size() == NUMBER_OF_NODES)
            {
                // string bufString;
                // // sequence:1,source:22.22.22.22:3333,destination:33.33.33.33:2222,previous:66.66.66.66:1001,data:heart:80
                // // sequence:1,22.22.22.22:3333,33.33.33.33:2222,66.66.66.66:1001,heart:80,location:23.456789:34.2342342,air:98,toxic:1
                // bufString = "sequence:"+to_string(sequence)+","+localAddress+","+destinationAddress+","+localAddress+",heart:"+to_string(heartRate)
                //     +",location:"+to_string(location_x)+":"+to_string(location_y)+",air:"+to_string(oxygen)+",toxic:"+to_string(toxic);
                // packageTable.push_back(bufString);

                // cout<<"Broadcasting package..." << bufSendingString << endl;

                // for (int i = 0; i < broadcastList.size(); i++)
                // {
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
                                cout<<"inet_aton() failed 2"<<endl;
                                cout<<"nodes size:"<<NUMBER_OF_NODES<<endl;
                                exit(1);
                            }

                            
                            if(fastAndSlowFadingLoss(thisX, thisY, nodes.at(j).x, nodes.at(j).y) == 0)
                            {
                                cout<<"manet sending package..."<< bufSendingString<<endl;
                                if (sendto(fdHub, bufSendingString.data(), bufSendingString.size(), 0, (struct sockaddr *)&remaddr, sizeof(remaddr))==-1) 
                                {
                                    perror("sendto 2");
                                    exit(1);
                                }
                            }
                        // }
                    }
                // }
                //cout<<"received"<<endl;
            }
            //usleep(TIME);
            forwardingCount++;
        }
        else if (isBroadcastHello == 1 && nodes.size()== NUMBER_OF_NODES)
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
            //     }
            //     j--;
            // }

            string helloString;

            helloString = "hello:"+to_string(DESTINATION_NODE)+",";
            vector<one_hop_neighbors_entry> one_hop_table_copy = one_hop_neighbors_table;
            for (int i = 0; i < one_hop_table_copy.size(); i++)
            {
                helloString = helloString + to_string(one_hop_table_copy.at(i).neighbors_id) + ",";
            }
            helloString = helloString +",";
            // cout<<helloString<<endl;
            // cout<<"nodes size: "<<NUMBER_OF_NODES<<endl;
            for (int j = 0; j < NUMBER_OF_NODES && j < nodes.size(); j++)
            {
                if (nodes.at(j).nodeNum != DESTINATION_NODE)
                {
                    memset((char *) &remaddr, 0, sizeof(remaddr));
                    remaddr.sin_family = AF_INET;
                    remaddr.sin_port = htons(nodes.at(j).port);
                    if (inet_aton(nodes.at(j).IP.c_str(), &remaddr.sin_addr)==0) 
                    {
                        // fprintf(stderr, "inet_aton() failed\n");
                        cout<<"inet_aton() failed...myNode:broadcastHello()"<<endl;
                            for (int jj = 0; jj < NUMBER_OF_NODES && jj < nodes.size(); jj++)
                            {
                                cout<<"IP: "<<nodes.at(jj).IP<<endl;
                            }
                            cout<<"inet_aton fail info: IP: "<<nodes.at(j).IP<<endl;
                            cout<<"inet_aton fail info: size of 'nodes': "<<NUMBER_OF_NODES<<endl;
                        exit(1);
                    }
                    if(fastAndSlowFadingLoss(thisX, thisY, nodes.at(j).x, nodes.at(j).y) == 0)
                    {
                        cout<<"send "<<helloString<<endl;
                        if (sendto(fdHub, helloString.data(), helloString.size(), 0, (struct sockaddr *)&remaddr, sizeof(remaddr))==-1) 
                        {
                            perror("sendto myNode:broadcastHello()");
                            exit(1);
                        }
                    }
                }
            }
            isBroadcastHello = 0;
            // usleep(TIME);
        }
    }
}

void* threadNodeReceivePackageOrACKThenForward(void* args) //receive and push ack package into cache and send data to display
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
    //cout<<"local port:"<<localPort<<endl;

    if (bind(fdHubReceive, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        cout<<"bind failed"<<endl;
        return 0;
    }

    struct sockaddr_in myaddr2, remaddr;
    int fdHub;
    
    int recvLen;

    if ((fdHub=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        cout<<"socket created"<<endl;

    memset((char *)&myaddr2, 0, sizeof(myaddr2));
    myaddr2.sin_family = AF_INET;
    myaddr2.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr2.sin_port = htons(0);

    if (bind(fdHub, (struct sockaddr *)&myaddr2, sizeof(myaddr2)) < 0) {
        perror("bind failed");
        return 0;
    }

    for (int i = 0;; i++)
    {
        deleteExpiredNeighbors();
        //cout<<"Waiting or receiving packages..."<<endl;
        string displayString;
        int recvLen = recvfrom(fdHubReceive, buf, sizeof(buf), 0, (struct sockaddr *)&priorAddress, &addressLength);
        if (recvLen>0)
        {
            cout<<"received string: "<<buf<<endl;
            stringstream stream((char *)buf);
            string receivedString = stream.str();
            string item;
            string sub_item;
            int j=0;
            string packageType;
            string packageContent;
            string coutString;
            string forwardString;
            int isPush_back=0;
            int receivedSequence;
            int isDestination=0;
            while(getline(stream, item, ','))
            {
                if (j == 0)
                {
                    //sequence number
                    stringstream sub_stream(item);
                    int t=0;
                    packageContent = item + ",";   /////////////////
                    while(getline(sub_stream, sub_item, ':'))
                    {
                        if (t==0)
                        {
                            packageType = sub_item;
                            if (packageType == "hello")
                            {
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
                    // source address
                    packageContent = packageContent + item+",";   ///////////////////
                    sourceAddressString = item;
                    if (packageType == "ack")
                    {
                        if (ackCache<receivedSequence)
                        {
                            isPush_back=1;
                            ackCache = receivedSequence;
                        }
                    }
                    if (packageType == "sequence")
                    {
                        if (sequenceCache<receivedSequence)
                        {
                            isPush_back=1;
                            sequenceCache = receivedSequence;
                            // cout<<"sequence cache: "<<sequenceCache<<endl;
                        }
                    }
                }
                if (j==2 && packageType != "hello")
                {
                    // destination address
                    if (item == localAddress)
                    {
                        isDestination = 1;
                        packageContent = "ack:"+to_string(receivedSequence)+ ","+localAddress+","+sourceAddressString+",";   /////////////////
                        // change the package type to ack
                    }
                    else
                    {
                        packageContent = packageContent + item;
                    }
                }
                if (j==3 && packageType != "hello")
                {
                    // previous change
                    // previous hop
                    packageContent = packageContent + to_string(DESTINATION_NODE) + ",";
                }
                if (j==4 && packageType == "sequence" && packageType != "hello")
                {
                    //heart rate
                    //cout << "Heart Rate"<< item<<"    ";
                    coutString = item;
                }
                if (j==5 && packageType == "sequence" && packageType != "hello")
                {
                    // location
                    //cout << "Location: "<< item<<"    ";
                    coutString = coutString +","+item;
                    //packageContent = packageContent + ","+item;
                }
                if (j==6 && packageType == "sequence" && packageType != "hello")
                {
                    //air
                    //cout << "Oxygen Level: "<< item<<"    ";
                    coutString = coutString +","+item;
                }
                if (j==7 && packageType == "sequence" && packageType != "hello")
                {
                    // toxic
                    //cout << "Toxic Level: "<< item<<endl;
                    coutString = coutString +","+item+",";
                }
                if (j==8 && packageType != "hello")
                {
                    //mode
                    stringstream sub_stream(item);
                    int t=0;
                    while(getline(sub_stream, sub_item, ':'))
                    {
                        if (t==1)
                        {
                            //cout<<"recv mode:"<<stoi(sub_item)<<endl;
                            if (mode != stoi(sub_item))
                            {
                                packageTable.clear();
                                mode = stoi(sub_item);
                            }
                        }
                        t++;
                    }
                    packageContent = packageContent + item+",";
                }
                j++;
            }

            //push_back
            if ((isPush_back == 1) && (coutString.length()>0) && packageType != "hello")
            {
                // ready for forward packages
                // cout<<"Pushing packet into packet table..." <<endl;
                // cout<< packageContent << endl;

                // cout<<" ------------------------------------------------------------------------------------"<<endl;
                // cout<<"                                                                                     "<<endl;
                // cout<<"        "<<coutString<<"        "<<endl;
                // cout<<"                                                                                     "<<endl;
                // cout<<" ------------------------------------------------------------------------------------"<<endl;

                //sendToDisplay(coutString);


                stringstream stream(coutString);
                int aa=0;
                string item;
                string destination_IP;
                int destination_port;
                while(getline(stream, item, ','))
                {
                    
                    if(aa==0)
                    {
                        destination_IP = heart_display_IP;
                        destination_port = heart_display_port;
                    }
                    if(aa==1)
                    {
                        destination_IP = location_display_IP;
                        destination_port = location_display_port;
                    }
                    if(aa==2)
                    {
                        destination_IP = oxygen_display_IP;
                        destination_port = oxygen_display_port;
                    }
                    if(aa==3)
                    {
                        destination_IP = toxic_display_IP;
                        destination_port = toxic_display_port;
                    }
                    if (aa<4)
                    {
                        // cout<<"destination_IP: "<<destination_IP<<endl;
                        // cout<<"destination_port: "<<destination_port<<endl;
                        memset((char *) &remaddr, 0, sizeof(remaddr));
                        remaddr.sin_family = AF_INET;
                        remaddr.sin_port = htons(destination_port);
                        if (inet_aton(destination_IP.c_str(), &remaddr.sin_addr)==0) 
                        {
                            cout<<"inet_aton failed at receive on fire_chief."<<endl;
                            for (int jj = 0; jj < NUMBER_OF_NODES && jj < nodes.size(); jj++)
                            {
                                cout<<"IP: "<<nodes.at(jj).IP<<endl;
                            }
                            cout<<"inet_aton fail info: IP: "<<destination_IP<<endl;
                            cout<<"inet_aton fail info: size of 'nodes': "<<NUMBER_OF_NODES<<endl;
                            exit(1);
                        }
                        string sendingItem = item+",";
                        //cout<<"display string: "<<sendingItem<<endl;
                        if (sendto(fdHub, sendingItem.data(), sendingItem.size(), 0, (struct sockaddr *)&remaddr, sizeof(remaddr))==-1) 
                        {
                            cout<<"sendto failed at receive on fire_chief."<<endl;
                            exit(1);
                        }
                    }
                    aa++;
                }

                packageTable.push_back(packageContent);
            }
            
        }
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
            cout<<"No reference string file found on updateLocation!"<<endl;
            return 0;
        }
        else
        {
            char line[2048];
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
                        // cout<<"First item: "<<item<<endl;
                        if (item != "Node")
                        {
                            isNullLine = 1;
                        }
                        else if (item == "Node")
                        {
                            isNullLine = 0;
                        }
                    }
                    if (i == 1 && isNullLine == 0)
                    {
                        // cout<<"Second item: "<<item<<endl;
                        myNode.nodeNum = stoi(item);
                    }
                    if (i == 2 && isNullLine == 0)
                    {
                         // IP
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
                        if (myNode.nodeNum == DESTINATION_NODE)
                        {
                            thisX = myNode.x;
                        }
                    }
                    if (i == 5 && isNullLine == 0) // y
                    {
                        myNode.y = stoi(item);
                        if (myNode.nodeNum == DESTINATION_NODE)
                        {
                            thisY = myNode.y;
                        }
                    }
                    if (i >6 && isNullLine == 0)
                    {
                        myNode.links.push_back(stoi(item));
                        if (myNode.nodeNum == DESTINATION_NODE)
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
            // cout<<"updateNodes"<<endl;
            nodes = updateNodes;
            // cout<<"nodes size"<<NUMBER_OF_NODES<<endl;
            // cout<<"updateNodes size"<<updateNodes.size()<<endl;
        }
        usleep(1000*500);
    }
    return 0;
}



int main()
{
    for (int i = 0;; ++i)
    {
        try
        {
        init_setup();
        pthread_t tids[3];
        pthread_create(&tids[0], NULL, threadNodeReceivePackageOrACKThenForward,NULL);
        pthread_create(&tids[1], NULL, threadNodeBroadcastPackageFromCache,NULL);
        pthread_create(&tids[2], NULL, updateLocation,NULL);
        pthread_exit(NULL);
        }
        catch (exception &e)
        {
            continue;
        }
        for (;;)
        {
            sleep(1);
        }
    }
    
    return 0;
}

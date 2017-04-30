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

typedef struct{
	int nodeNum;
	string IP;
	int port;
	double x;
	double y;
} node_config_file_entry;

typedef struct{
	double x;
	double y;
} coordinate;

vector<node_config_file_entry> node_config;

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
        while (referenceStringFile.getline(line, sizeof(line)))
        {
            string modifiedEntry = "";
            stringstream stream(line);
            string item;
            int i = 0;
            double init_x;
            double init_y;
            node_config_file_entry entry;
            while(getline(stream, item, ' '))
            {
                if (i == 1)
                {
                	// node number
                    entry.nodeNum = stoi(item);
                }
                if (i == 2)
                {
                	// which tux 
                    entry.IP = item; // no need to remove "," from the end of this item
                }
                if (i == 3)
                {
                    entry.port = stoi(item);
                }
                if (i == 4) 
                {
                	// x
                	double init_x = (double)stod(item);
                	entry.x = init_x;
                }
                if (i == 5) 
                {
                	// y
                    double init_y = (double)stod(item);
                    entry.y = init_y;
                }
                i++;
            }
            node_config.push_back(entry);
        }

    }
    return 0;
}

int print_config_file()
{
	for (int i = 0; i < node_config.size(); i++)
	{
		// Node 1 tux206, 10010 188 197 links
		cout<< "Node "<<node_config.at(i).nodeNum<<" "<<node_config.at(i).IP<<" "<<node_config.at(i).port;
		cout<<" "<<node_config.at(i).x<<" "<<node_config.at(i).y<<" links"<<endl;
	}
	return 0;
}

double randSpeed(double x, double y)
{

    srand(time(NULL)*x/y);
    double speed = (double((rand()%10)+1))/10.0;
    return speed;
}

double randDirection(double x, double y)
{
    srand(time(NULL)*x/y);
    double direction = double((rand()%360));
    return direction;
}

coordinate updateLocation(double x, double y)
{
    // double x=31.0;
    // double y=28.0;
    double speed = randSpeed(x, y); // meters per second
    double brng = randDirection(x, y);
    coordinate myCoordinate;
    double distance = 0;
    double duration = 0.5; // second

    srand(time(NULL));
    double randNum = double(rand()%5)-2.5;
    brng = brng + randNum;
    if (brng<0)
    {
        brng = brng +360;
    }
    if (brng>=360)
    {
        brng = brng - 360;
    }
    distance = speed * duration;
    x = x + distance * sin(brng);
    y = y + distance * cos(brng);
    if (x<0)
    {
        x = 0;
    }
    if (y<0)
    {
        y=0;
    }

    // string sendingString;
    // sendingString = "location: "+to_string(x)+", "+to_string(y);
    // cout<<sendingString<<endl;

    myCoordinate.x = x;
    myCoordinate.y = y;

    return myCoordinate;
}

int writeToConfigFile()
{
	ofstream testConfigFile(MANET_FILE);
	if (testConfigFile.is_open())
	{
		string writeString = "";
		for (int i = 0; i < node_config.size(); i++)
		{
			// Node 1 tux206, 10010 188 197 links

			string nodeNum_str = to_string(node_config.at(i).nodeNum);
			string IP_str = node_config.at(i).IP;
			string port_str = to_string(node_config.at(i).port);
			string x_str = to_string(node_config.at(i).x);
			string y_str = to_string(node_config.at(i).y);
			writeString = writeString +"Node "+ nodeNum_str+" "+IP_str+" "+port_str+" "+x_str+" "+y_str+" links\n";
		}
		testConfigFile << writeString;
		testConfigFile.close();
	}
	return 0;
}

int main()
{
	init_setup();
	// print_config_file();

	for (int i = 0;;i++)
	{
		for (int j = 0; j < node_config.size(); j++)
		{
			coordinate myCoordinate = updateLocation(node_config.at(j).x, node_config.at(j).y);
			node_config.at(j).x = myCoordinate.x;
			node_config.at(j).y = myCoordinate.y;
		}
		// print_config_file();
		writeToConfigFile();
        // cout<<"number of node: "<<node_config.size()<<endl;
		usleep(1000*500);
		cout<<endl;
		cout<<endl;
	}
}
//
//  ofxTcpOscReceiver.h
//
//  Created by Koki Nomura on 2014/06/13.
//
//

#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"

#include "ofxTcpOscMessage.h"
#include "ofxTcpOscUtils.h"

class ofxTcpOscReceiver {
public:
    ofxTcpOscReceiver();
    ~ofxTcpOscReceiver();
    
    bool setup(int listen_port);
    void update();
    bool hasWaitingMessages();
    bool getNextMessage(ofxTcpOscMessage *);

    ofxTCPServer server;

    bool sendMessage(ofxTcpOscMessage & message);
    void shutdown();

private:
    void _receiveOscMessages(int clientId, deque<ofPtr<ofxTcpOscMessage> > & messages);
    
    // parse functions
    // those return an offset.
    int _parsePacketSize(char* packet, int& output);
    int _parsePacketAddress(char* packet, string& output);
    int _parsePacketTypes(char* packet, string& output, int offset);
    void _parsePacketArgs(char* packet, ofPtr<ofxTcpOscMessage> m, int offset, string typeString);

    // helper methods
    string makeOscTypeTagString(ofxTcpOscMessage & message);
    void appendStringToOscString(string input, vector<char> & output);
    void makeOscString(string input, vector<char> & output);

    deque<ofPtr<ofxTcpOscMessage> > _messages;
    
    int nBytes = 2048;
    char packet[2048];
    int cycle = 0;
};
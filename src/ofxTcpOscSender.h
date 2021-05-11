//
//  ofxTcpOscSender.h
//
//  Created by Koki Nomura on 2014/06/12.
//
//

// ofxTcpOsc is an addon for OSC-like communication over TCP.
//
// Protocol:
//      (size means a number of chars)
//      4 bytes: size of the address
//      n bytes: address
//      4 bytes: size of arg1
//      n bytes: arg1
//      ...

#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"

#include "ofxTcpOscMessage.h"
#include "ofxTcpOscUtils.h"


class ofxTcpOscSender {
public:
    ofxTcpOscSender();
    ~ofxTcpOscSender();
    
    bool setup(string hostname, int port);
    bool sendMessage(ofxTcpOscMessage & message);
    
    void shutdown();


    // receiver public stuff:
    void update(ofEventArgs & args);
    bool hasWaitingMessages();
    bool getNextMessage(ofxTcpOscMessage *);

private:
    void appendMessage(ofxTcpOscMessage & message);
    
    ofxTCPClient tcpClient;
    string _hostname;
    int _port;
    
    // helper methods
    string makeOscTypeTagString(ofxTcpOscMessage & message);
        
    void appendStringToOscString(string input, vector<char> & output);
    void makeOscString(string input, vector<char> & output);

    // receiver private stuff:
    void _receiveOscMessages(deque<ofPtr<ofxTcpOscMessage> > & messages);
    // parse functions
    // those return an offset.
    int _parsePacketSize(char* packet, int& output);
    int _parsePacketAddress(char* packet, string& output);
    int _parsePacketTypes(char* packet, string& output, int offset);
    void _parsePacketArgs(char* packet, ofPtr<ofxTcpOscMessage> m, int offset, string typeString);

    deque<ofPtr<ofxTcpOscMessage> > _messages;
    int nBytes = 4096;
    char packet[4096];
};









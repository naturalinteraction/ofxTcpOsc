//
//  ofxTcpOscReceiver.cpp
//
//  Created by Koki Nomura on 2014/06/13.
//
//

#include "ofxTcpOscReceiver.h"

using namespace ofxTcpOsc;

ofxTcpOscReceiver::ofxTcpOscReceiver() {
}

ofxTcpOscReceiver::~ofxTcpOscReceiver() {
    server.close();
}

bool ofxTcpOscReceiver::setup(int listen_port) {

    ofxTCPSettings settings(listen_port);

    settings.blocking = false;  // not blocking
    settings.reuse = true;

    if (! server.setup(settings))
        return false;
    return true;
}

void ofxTcpOscReceiver::shutdown() {
    server.close();
}

void ofxTcpOscReceiver::update() 
{
    // allows multiple connections
    // though it is intended to have only one connection

    if (cycle < server.getLastID())
    {
        if (server.isClientConnected(cycle))
            _receiveOscMessages(cycle, _messages);
        cycle++;
    }
    else
        cycle = 0;
    // printf("cycle %d\n", cycle);  // av, may 2024: to avoid performance issues
}

bool ofxTcpOscReceiver::hasWaitingMessages() {
    return !_messages.empty();
}

bool ofxTcpOscReceiver::getNextMessage(ofxTcpOscMessage * m) {
    ofPtr<ofxTcpOscMessage> p = _messages.front();
    _messages.pop_front();
    m->copy(*(p.get()));
    return true;
}

//#pragma mark - Private Methods

void ofxTcpOscReceiver::_receiveOscMessages(int clientId, deque<ofPtr<ofxTcpOscMessage> > &messages) {

    int received = server.receiveRawBytes(clientId, packet, nBytes);
    if (received <= 0) {
        return;
    }

    // parse
    ofPtr<ofxTcpOscMessage> m = ofPtr<ofxTcpOscMessage>(new ofxTcpOscMessage);
    
    int packetSize;
    _parsePacketSize(packet, packetSize);

    string address;
    int offset = _parsePacketAddress(packet, address);
    m->setAddress(address);
    
    string typeString;
    offset = _parsePacketTypes(packet, typeString, offset);
    
    _parsePacketArgs(packet, m, offset, typeString);
    
    string host = server.getClientIP(clientId);
    int port = server.getClientPort(clientId);
    m->setRemoteEndpoint(host, port);

    _messages.push_back(m);

}

int ofxTcpOscReceiver::_parsePacketSize(char *packet, int &output) {
    int int32Size = 4;
    int sum = 0;
    for (int i=0; i<int32Size; i++) {
        int power = int32Size - i - 1;
        sum += packet[i] * ::pow(256, power);
    }
    output = sum;
    
    return int32Size;
}

int ofxTcpOscReceiver::_parsePacketAddress(char *packet, string &output) {
    int int32Size = 4;
    int maxAddressLen = 1024;
    
    output.clear();
    output.append(&packet[int32Size]);
    int i;
    for (i=int32Size; i<maxAddressLen; i++) {
        if (packet[i] == ',') {
            return i;
        }
    }
    return i;
}

int ofxTcpOscReceiver::_parsePacketTypes(char *packet, string &output, int offset) {
    //int maxLen = 1024;
    output.clear();
    output.append(&packet[offset+1]);
    
    int size = output.size();
    size += 4 - size % 4;
    return offset + size;
}

void ofxTcpOscReceiver::_parsePacketArgs(char *packet, ofPtr<ofxTcpOscMessage> m, int offset, string typeString) {

    int size = 0;
    for (int i=0; i<(int)typeString.size(); i++) {
        if (typeString[i] == 'i') {
            size = 4;
            int32_t arg = chars_to_int32(&packet[offset]);
            m->addIntArg(arg);
        } else if (typeString[i] == 'f') {
            size = 4;
            float arg = chars_to_float(&packet[offset]);
            m->addFloatArg(arg);
        } else if (typeString[i] == 'h') {
            size = 8;
            uint64_t arg = chars_to_int64(&packet[offset]);
            m->addInt64Arg(arg);
        } else if (typeString[i] == 's') {
            string arg = "";
            arg.append(&packet[offset]);
            m->addStringArg(arg);
            size = arg.size();
            size += 4 - size % 4;
        }
        offset += size;
    }
}


bool ofxTcpOscReceiver::sendMessage(ofxTcpOscMessage &message) {

    using namespace ofxTcpOsc;
    
    vector<char> output;
    
    // add address
    appendStringToOscString(message.getAddress(), output);

    // add Osc Type Tag String
    string oscTypeTagString = makeOscTypeTagString(message);
    appendStringToOscString(oscTypeTagString, output);

    // add arguments
    using namespace ofxTcpOsc;
    for (int i=0; i<message.getNumArgs(); i++) {
        vector<char> buf;
        if (message.getArgType(i) == OFXTCPOSC_TYPE_INT32) {
            int32_to_chars(message.getArgAsInt32(i), buf);
        } else if (message.getArgType(i) == OFXTCPOSC_TYPE_INT64) {
            int64_to_chars(message.getArgAsInt64(i), buf);
        } else if (message.getArgType(i) == OFXTCPOSC_TYPE_FLOAT) {
            float_to_chars(message.getArgAsFloat(i), buf);
        } else if (message.getArgType(i) == OFXTCPOSC_TYPE_STRING) {
            makeOscString(message.getArgAsString(i), buf);
            
        } else {
            ofLogError("ofxTcpOscSender") << "sendMessage(): bad argument type " << message.getArgType(i);
            assert(false);
            return false;
        }
        for (unsigned int i=0; i<buf.size(); i++) {
            output.push_back(buf[i]);
        }
    }
    
    // add size of the packet
    int numInt32 = 4;
    int32_t size = output.size() + numInt32;
    vector<char> sizeStr;
    int32_to_chars(size, sizeStr);
    
    // prepend the size string
    for (int i=sizeStr.size()-1; i>=0; i--) {
        vector<char>::iterator it = output.begin();
        output.insert(it, sizeStr[i]);
    }
    if (! server.sendRawBytesToAll(output.data(), output.size()))
    {
        return false;
    }
    return true;
}

void ofxTcpOscReceiver::appendStringToOscString(string input, vector<char> &output) {
    int numNulls = 4 - input.size() % 4;
    if (numNulls == 0) {
        numNulls = 4;
    }
    for (unsigned int i=0; i<input.size(); i++) {
        output.push_back(input[i]);
    }
    for (int i=0; i<numNulls; i++) {
        output.push_back(0);
    }
}

void ofxTcpOscReceiver::makeOscString(string input, vector<char> &output) {
    int numNulls = input.size() % 4;
    if (numNulls == 0) {
        numNulls = 4;
    }
    for (unsigned int i=0; i<input.size(); i++) {
        output.push_back(input[i]);
    }
    for (int i=0; i<numNulls; i++) {
        output.push_back(0);
    }
    // return output;
}

string ofxTcpOscReceiver::makeOscTypeTagString(ofxTcpOscMessage &message) {
    string oscTypeTagString = ",";
    
    for (int i=0; i<message.getNumArgs(); i++) {
        if (message.getArgType(i) == OFXTCPOSC_TYPE_INT32) {
            oscTypeTagString.append("i");
        } else if (message.getArgType(i) == OFXTCPOSC_TYPE_INT64) {
            oscTypeTagString.append("h");
        } else if (message.getArgType(i) == OFXTCPOSC_TYPE_FLOAT) {
            oscTypeTagString.append("f");
        } else if (message.getArgType(i) == OFXTCPOSC_TYPE_STRING) {
            oscTypeTagString.append("s");
        } else {
            ofLogError("ofxTcpOscSender") << "makeOscTypeTagString(): bad argument type " << message.getArgType(i);
            assert(false);
        }
    }
    return oscTypeTagString;
}



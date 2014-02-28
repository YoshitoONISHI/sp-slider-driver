#include "testApp.h"

#define LISTEN_PORT 5100
#define SEND_PORT 5000
#define BAUD_RATE 9600
#define HOST_NAME "127.0.0.1"

bool togBusyBlink = false;
//--------------------------------------------------------------
void testApp::setup(){
  current_msg_string = 0;
  
  settingXml.loadFile("settings.xml");
  
  string portName = settingXml.getValue("settings:serial-port", "/dev/tty.usbmodem1411");
  string hostName = settingXml.getValue("settings:host", HOST_NAME);
  int baudRate = settingXml.getValue("settings:baud-rate", BAUD_RATE);
  int listenPort = settingXml.getValue("settings:listen-port", LISTEN_PORT);
  int sendPort = settingXml.getValue("settings:send-port", SEND_PORT);
  run = true;
  busy = false;
  serial.enumerateDevices();
  serial.setup(portName, baudRate);
  
  reciver.setup(listenPort);
  sender.setup(hostName, sendPort);
  
  nTimesRead = 0;
	nBytesRead = 0;
	readTime = 0;
  currentPosition = 0;
	memset(bytesReadString, 0, 7);
  
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC), dispatch_get_current_queue(), ^{//do reset after 5 seconds
  });
  
  font.loadFont("DIN.otf", 14);
  currentStatus = "";
}

//--------------------------------------------------------------
void testApp::update(){
  
  for(int i = 0; i < NUM_MSG_STRINGS; i++){
    msg_strings[i] = "";
	}
  
  while ( reciver.hasWaitingMessages() ) {
    string msg_string;
    ofxOscMessage m;
    reciver.getNextMessage( &m );
    
    if( "/up" == m.getAddress() ) {
      if( false == busy ) {
        int nextLevel = m.getArgAsInt32(0);
        busy = true;
        proceedLevel( 0, nextLevel );
        if( !run ){
          cout << "recived OSC message but not running flag.." << endl;
        }
        
        msg_string += "Message Recived : /up :" ;
        msg_string += ofToString( nextLevel );
      }
    } else if( "/down" == m.getAddress() ){
      if( false == busy ) {
        int nextLevel = m.getArgAsInt32(0);
        busy = true;
        proceedLevel( 1, nextLevel );
        if( !run ){
          cout << "recived OSC message but not running flag.." << endl;
        }
        
        msg_string += "Message Recived : /down :" ;
        msg_string += ofToString( nextLevel );
      }
    }
    
    // add to the list of strings to display
    msg_strings[current_msg_string] = msg_string;
    current_msg_string = (current_msg_string + 1) % NUM_MSG_STRINGS;
    // clear the next line
    msg_strings[current_msg_string] = "";
    
  }
  
  ///////////////// serial read ///////////////////////////////
  
  
  if (bSendSerialMessage) {
		
		nTimesRead = 0;
		nBytesRead = 0;
		int nRead  = 0;  // a temp variable to keep count per read
		
		unsigned char bytesReturned[6];
		
		memset(bytesReadString, 0, 7);
		memset(bytesReturned, 0, 6);
		
		while( (nRead = serial.readBytes(bytesReturned, 6)) > 0){
			nTimesRead++;
			nBytesRead = nRead;
		};

		memcpy(bytesReadString, bytesReturned, 6);
    if ('X' == bytesReturned[0] && 'X' == bytesReturned[1]) {
      unsigned char stateByte = bytesReturned[2];
      if ('N' == stateByte) {
        cout << "status normal" << endl;
      } else if ('F' ==stateByte) {
        cout << "--------- status LIMIT ------" << endl;
      }
      int h_pos = (unsigned int) bytesReturned[3];
      int l_pos = (unsigned int) bytesReturned[4];
      currentPosition = h_pos * 256 + l_pos;
      
      cout << currentPosition << endl;
      bSendSerialMessage = false;
      busy = false;
    }
		
		readTime = ofGetElapsedTimef();
	}
  
  /////////////////////////////////////////////////////////////
  
  if (busy) {
    currentStatus = "BUSY.";
  } else {
    currentStatus = "READY.";
  }
}

//--------------------------------------------------------------
void testApp::draw(){
  
  ofBackground(0);
  
	// draw mouse state
  ofPushStyle();
  ofSetColor(255, 255, 255);
  
	for(int i = 0; i < NUM_MSG_STRINGS; i++){
		ofDrawBitmapString(msg_strings[i], 430, 15 + 15 * i);
	}
  
  ofPopStyle();
  
  if (nBytesRead > 0 && ((ofGetElapsedTimef() - readTime) < 0.5f)){
		ofSetColor(0);
	} else {
		ofSetColor(220);
	}
	string msg;
  msg += "Current Status : " + currentStatus + "\n";
	msg += "Current Position : " + ofToString(currentPosition) +"\n";
	msg += "nBytes read " + ofToString(nBytesRead) + "\n";
	msg += "nTimes read " + ofToString(nTimesRead) + "\n";
	msg += "read packet : " + ofToString(bytesReadString) + "\n";
	msg += "(time to pass : " + ofToString(readTime, 3) + ")";
	font.drawString(msg, 10, 100);
}


//--------------------------------------------------------------
void testApp::proceedLevel( int upDown , int _nextLevel ) {
  if( run ){
    int next = _nextLevel;
    if( 9999 != next  && -1 < next ){ //in case error 9999
      
      
      if( next >= 0 ) {
        
        if( 0 == upDown ){
          unsigned char buf[3] = "XF";
          serial.writeBytes( buf , sizeof(buf) / sizeof(buf[0]) );
          
        } else if ( 1 == upDown ) {
          unsigned char buf[3] = "XB";
          serial.writeBytes( buf , sizeof(buf) / sizeof(buf[0]) );
          
        }

      } else {
        busy = false;
      }
      
      
    } else {
      reset();
      
    }
  } else {
    busy = false;
  }
};



//--------------------------------------------------------------
void testApp::reset() {
  unsigned char speed = 25;
  unsigned int distenation = 0;
  ofResetElapsedTimeCounter();
  busy = true;
  
  unsigned char buf[7];
  
  unsigned char h_dist = (unsigned char)(distenation >> 8);
  unsigned char l_dist = (unsigned char)(distenation % 256);
  
  buf[0] = 'x';
  buf[1] = 'x';
  buf[2] = 'r';
  buf[3] = speed;
  buf[4] = h_dist;
  buf[5] = l_dist;
  buf[6] = '\n';
  
  int hvoer =((int)h_dist * 256 + (int)l_dist);
  cout << (int)buf[4] << " "  << (int)buf[5] << " " << hvoer <<endl;
  
  serial.writeBytes( buf , sizeof(buf) / sizeof(buf[0]) );
  bSendSerialMessage = true;
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

//--------------------------------------------------------------
void testApp::stepTo(unsigned int distenation, unsigned char speed) {
  ofResetElapsedTimeCounter();
  busy = true;
  
  unsigned char buf[7];
  
  unsigned char h_dist = (unsigned char)(distenation >> 8);
  unsigned char l_dist = (unsigned char)(distenation % 256);
  
  buf[0] = 'x';
  buf[1] = 'x';
  buf[2] = 'f';
  buf[3] = speed;
  buf[4] = h_dist;
  buf[5] = l_dist;
  buf[6] = '\n';
  
  int hvoer =((int)h_dist * 256 + (int)l_dist);
  cout << "packets..: "<<(int)buf[4] << " "  << (int)buf[5] << " " << hvoer <<endl;
  
  serial.writeBytes( buf , sizeof(buf) / sizeof(buf[0]) );
  bSendSerialMessage = true;
};

//--------------------------------------------------------------
void testApp::keyPressed(int key){
  
  unsigned char buf[7];
  unsigned int distenation;
  if(key == '1'){
    stepTo(120, 1);
  } else if(key == '2') {
    stepTo(220, 5);
  } else if(key == '3') {
    stepTo(1200, 2);
  } else if(key == '4') {
    stepTo(1310, 1);
  } else if(key == '0') {
    reset();
  }

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
  
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
  
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
  
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
  unsigned char buf[7];
  unsigned int distenation = 1200;
  unsigned char h_dist = (unsigned char)(distenation >> 8);
  unsigned char l_dist = (unsigned char)(distenation % 256);

  buf[0] = 'x';
  buf[1] = 'x';
  buf[2] = 'f';
  buf[3] = 143;
  buf[4] = h_dist;
  buf[5] = l_dist;
  buf[6] = '\n';
  
  int hvoer =((int)h_dist * 256 + (int)l_dist);
  cout << (int)buf[4] << " "  << (int)buf[5] << " " << hvoer <<endl;
  
  serial.writeBytes( buf , sizeof(buf) / sizeof(buf[0]) );
  bSendSerialMessage = true;
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
  
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
  
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){
  
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){
  
}
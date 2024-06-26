/*
   @title     StarBase
   @file      UserModInstances.h
   @date      20240411
   @repo      https://github.com/ewoudwijma/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewoudwijma/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#ifdef STARBASE_USERMOD_E131
  #include "UserModE131.h"
#endif

struct DMX {
  byte universe:3; //3 bits / 8
  uint16_t start:9; //9 bits / 512
  byte count:4; // 4 bits / 16
}; //total 16 bits

//additional data not in wled header
struct SysData {
  unsigned long upTime;
  byte type;
  byte syncMaster;
  DMX dmx;
  uint8_t macAddress[6]; // 48 bits
};

struct VarData {
  char id[3];
  byte value;
}; //4

//note: changing SysData and jsonData sizes: all instances should have the same version so change with care

struct InstanceInfo {
  IPAddress ip;
  char name[32];
  uint32_t version;
  unsigned long timeStamp; //when was the package received, used to check on aging
  SysData sysData;
  JsonDocument jsonData;
};

struct UDPWLEDMessage {
  byte token;       //0: 'binary token 255'
  byte id;          //1: id '1'
  byte ip0;         //2: uint32_t ip takes 6 bytes instead of 4 ?!?! so splitting it into bytes
  byte ip1;         //3
  byte ip2;         //4
  byte ip3;         //5
  char name[32];    //6..37: server name
  byte type;        //38: instance type id //esp32 tbd: CONFIG_IDF_TARGET_ESP32S3 etc
  byte insId;      //39: instance id  unit ID == last IP number
  uint32_t version; //40..43: version ID (here it takes 4 bytes (as it should)
}; //total 44 bytes!

//compatible with WLED instances as it only interprets first 44 bytes
struct UDPStarMessage {
  UDPWLEDMessage header; // 44 bytes fixed!
  SysData sysData;
  char jsonString[1460 - sizeof(UDPWLEDMessage) - sizeof(SysData)];
};

//WLED syncmessage
struct UDPWLEDSyncMessage { //see notify( in WLED
  byte protocol; //0
  byte callMode; //1
  byte bri; //2
  byte rCol0; //3
  byte gCol0; //4
  byte bCol0; //5
  byte nightlightActive; //6
  byte nightlightDelayMins; //7
  byte mainsegMode; //8
  byte mainsegSpeed; //9
  byte wCol0; //10
  byte version; //11
  byte col1[4]; //12
  byte mainsegIntensity; //16
  byte transitionDelay[2]; //17
  byte palette; //19
  byte col2[4]; //20
  byte followUp; //24
  byte timebase[4]; //25
  byte tokiSource; //29
  byte tokiTime[4]; //30
  byte tokiMs[2]; //34
  byte syncGroups; //36
  char body[1193 - 37]; //41 +(32*36)+0 = 1193
};

class UserModInstances:public SysModule {

public:

  std::vector<InstanceInfo> instances;

  UserModInstances() :SysModule("Instances") {
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initSysMod(parentVar, name, 3000);

    JsonObject tableVar = ui->initTable(parentVar, "insTbl", nullptr, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Instances");
        ui->setComment(var, "List of instances");
        return true;
      }
      default: return false;
    }});
    
    ui->initText(tableVar, "insName", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, JsonString(instances[rowNrL].name, JsonString::Copied), rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Name");
        return true;
      default: return false;
    }});

    ui->initURL(tableVar, "insLink", nullptr, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++) {
          char urlString[32] = "http://";
          strncat(urlString, instances[rowNrL].ip.toString().c_str(), sizeof(urlString)-1);
          mdl->setValue(var, JsonString(urlString, JsonString::Copied), rowNrL);
        }
        return true;
      case f_UIFun:
        ui->setLabel(var, "Show");
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "insIp", nullptr, 16, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, JsonString(instances[rowNrL].ip.toString().c_str(), JsonString::Copied), rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "IP");
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "insType", nullptr, 16, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, (instances[rowNrL].sysData.type==0)?"WLED":(instances[rowNrL].sysData.type==1)?"StarBase":(instances[rowNrL].sysData.type==2)?"StarLeds":"StarFork", rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Type");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "insVersion", UINT16_MAX, 0, (unsigned long)-1, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, instances[rowNrL].version, rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Version");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "insUp", UINT16_MAX, 0, (unsigned long)-1, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, instances[rowNrL].sysData.upTime, rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Uptime");
      default: return false;
    }});

    JsonObject currentVar;

    //default is 0 / None
    currentVar = ui->initIP(parentVar, "sma", 0, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Sync Master");
        ui->setComment(var, "Instance to sync from");
        JsonArray options = ui->setOptions(var);
        JsonArray instanceObject = options.add<JsonArray>();
        instanceObject.add(0);
        instanceObject.add("no sync");
        for (InstanceInfo &instance : instances) {
          char option[64] = { 0 };
          strncpy(option, instance.name, sizeof(option)-1);
          strncat(option, " ", sizeof(option)-1);
          strncat(option, instance.ip.toString().c_str(), sizeof(option)-1);
          instanceObject = options.add<JsonArray>();
          instanceObject.add(instance.ip[3]);
          instanceObject.add(option);
        }
        return true;
      }
      default: return false;
    }}); //syncMaster
    currentVar["dash"] = true;

    //find dash variables and add them to the table
    mdl->findVars("dash", true, [tableVar, this](JsonObject var) { //findFun

      ppf("dash %s %s found\n", mdl->varID(var), var["value"].as<String>().c_str());

      char columnVarID[32] = "ins";
      strcat(columnVarID, var["id"]);
      JsonObject insVar; // = ui->cloneVar(var, columnVarID, [this, var](JsonObject insVar){});

      //create a var of the same type. InitVar is not calling chFun which is good in this situation!
      insVar = ui->initVar(tableVar, columnVarID, var["type"], false, [this, var](JsonObject insVar, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case f_ValueFun:
          //should not trigger chFun
          for (forUnsigned8 rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++) {
            // ppf("initVar dash %s[%d]\n", mdl->varID(insVar), rowNrL);
            //do what setValue is doing except calling changeFun
            // insVar["value"][rowNrL] = instances[rowNrL].jsonData[mdl->varID(var)]; //only int values...

            web->addResponse(insVar["id"], "value", instances[rowNrL].jsonData[mdl->varID(var)], rowNrL);

            // mdl->setValue(insVar, instances[rowNrL].jsonData[mdl->varID(var)], rowNr);
          //send to ws?
          }
          return true;
        case f_UIFun:
          // call uiFun of the base variable for the new variable
          ui->varFunctions[var["fun"]](insVar, rowNr, f_UIFun);
          return true;
        case f_ChangeFun: {
          //do not set this initially!!!
          if (rowNr != UINT8_MAX) {
            //if this instance update directly, otherwise send over network
            if (instances[rowNr].ip == WiFi.localIP()) {
              mdl->setValue(var, mdl->getValue(insVar, rowNr).as<unsigned8>()); //this will call sendDataWS (tbd...), do not set for rowNr
            } else {
              // https://randomnerdtutorials.com/esp32-http-get-post-arduino/

              if (0 != instanceUDP.beginPacket(instances[rowNr].ip, instanceUDPPort)) {

                JsonDocument message;
                message["id"] = mdl->varID(var);
                message["value"] = mdl->getValue(insVar, rowNr).as<unsigned8>();

                char buffer[500];

                serializeJson(message, buffer);

                instanceUDP.write((byte *)buffer, sizeof(buffer));
                web->sendUDPCounter++;
                web->sendUDPBytes+=sizeof(buffer);
                instanceUDP.endPacket();
                ppf("udp json write %s\n", buffer);
              }
            }
          }
          // print->printJson(" ", var);
          return true;
        }
        default: return false;
      }});

      if (insVar) {
        if (!var["min"].isNull()) insVar["min"] = var["min"];
        if (!var["max"].isNull()) insVar["max"] = var["max"];
        if (!var["log"].isNull()) insVar["log"] = var["log"];
        // insVar["fun"] = var["fun"]; //copy the uiFun
      }

    });

    if (sizeof(UDPWLEDMessage) != 44) {
      ppf("Program error: Size of UDP message is not 44: %d\n", sizeof(UDPWLEDMessage));
      // ppf("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }
    if (sizeof(UDPStarMessage) != 1460) { //size of UDP Packet
      // one udp frame should be 1460, not 1472 (then split by network in 1460 and 12)
      ppf("Program error: Size of UDP message is not 44: %d\n", sizeof(UDPStarMessage));
      // ppf("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }

    ppf("UDPWLEDSyncMessage %d %d %d\n", sizeof(UDPWLEDMessage), sizeof(UDPStarMessage), sizeof(UDPWLEDSyncMessage));
  }

  void onOffChanged() {
    if (mdls->isConnected && isEnabled) {
      udpConnected = notifierUdp.begin(notifierUDPPort); //sync
      udp2Connected = instanceUDP.begin(instanceUDPPort); //instances
    } else {
      udpConnected = false;
      udp2Connected = false;
      instances.clear();

      //not needed here as there is no connection
      // ui->processUiFun("insTbl");

      //udp off ??
    }
  }

  void loop() {
    // SysModule::loop();

    handleNotifications();

    if (ui->dashVarChanged) {
      ui->dashVarChanged = false;
      sendSysInfoUDP(); 
    }
  }

  void loop10s() {
    sendSysInfoUDP(); 
  }

  void handleNotifications()
  {
    if(!mdls->isConnected) return;

    // instanceUDP.flush(); //tbd: test if needed

    int packetSize;

    //handle sync from WLED
    if (udpConnected) {
      int packetSize = notifierUdp.parsePacket();

      if (packetSize > 0) {
        IPAddress remoteIp = notifierUdp.remoteIP();

        // ppf("handleNotifications sync ...%d %d\n", remoteIp[3], packetSize);

        UDPWLEDSyncMessage wledSyncMessage;
        byte *udpIn = (byte *)&wledSyncMessage;
        notifierUdp.read(udpIn, packetSize);

        // for (int i=0; i<40; i++) {
        //   Serial.printf("%d: %d\n", i, udpIn[i]);
        // }

        ppf("   %d %d p:%d\n", wledSyncMessage.bri, wledSyncMessage.mainsegMode, packetSize);

        InstanceInfo *instance = findInstance(remoteIp); //if not exist, created

        instance->sysData.upTime = (wledSyncMessage.timebase[0] * 256*256*256 + 256*256*wledSyncMessage.timebase[1] + 256*wledSyncMessage.timebase[2] + wledSyncMessage.timebase[3]) / 1000;
        instance->sysData.syncMaster = wledSyncMessage.syncGroups; //tbd: change
        
        stackUnsigned8 syncMaster = mdl->getValue("sma");
        if (syncMaster == remoteIp[3]) {
          if (instance->jsonData["bri"] != wledSyncMessage.bri) mdl->setValue("bri", wledSyncMessage.bri);
          //only set brightness
        }

        instance->jsonData["bri"] = wledSyncMessage.bri;
        instance->jsonData["fx"] = wledSyncMessage.mainsegMode; //tbd: rowNr
        instance->jsonData["pal"] = wledSyncMessage.palette; //tbd: rowNr

        // for (size_t x = 0; x < packetSize; x++) {
        //   char xx = (char)udpIn[x];
        //   Serial.print(xx);
        // }
        // Serial.println();

        ppf("insTbl handleNotifications %d\n", remoteIp[3]);
        for (JsonObject childVar: mdl->varChildren("insTbl"))
          ui->callVarFun(childVar, UINT8_MAX, f_ValueFun); //rowNr //instance - instances.begin()

        web->recvUDPCounter++;
        web->recvUDPBytes+=packetSize;

        return;
      }
    }
 
    //handle instances update
    if (udp2Connected) {
      packetSize = instanceUDP.parsePacket();

      if (packetSize > 0) {
        IPAddress remoteIp = instanceUDP.remoteIP();
        // ppf("handleNotifications instances ...%d %d check %d or %d\n", remoteIp[3], packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarMessage));

        if (packetSize == sizeof(UDPWLEDMessage)) { //WLED instance
          UDPStarMessage starMessage;
          byte *udpIn = (byte *)&starMessage.header;
          instanceUDP.read(udpIn, packetSize);

          starMessage.sysData.type = 0; //WLED

          updateInstance(starMessage);
        }
        else if (packetSize == sizeof(UDPStarMessage)) { //StarBase instance
          UDPStarMessage starMessage;
          byte *udpIn = (byte *)&starMessage;
          instanceUDP.read(udpIn, packetSize);
          starMessage.sysData.type = (strcmp(_INIT(TOSTRING(APP)), "StarBase")==0)?1:(strcmp(_INIT(TOSTRING(APP)), "StarLeds")==0)?2:3; //1=StarBase,2=StarLeds, 3=StarFork

          updateInstance(starMessage);
        }
        else if (packetSize == 500) { //udp json message

            char buffer[packetSize];
            instanceUDP.read(buffer, packetSize);

            JsonDocument message;
            deserializeJson(message, buffer);

            mdl->setValue(message["id"].as<const char *>(), message["value"].as<uint8_t>());

            ppf("udp json read %s\n", buffer);

        }
        else {
          //read the rest of the data (flush)
          byte udpIn[1472+1];
          instanceUDP.read(udpIn, packetSize);

          ppf("packetSize %d not equal to %d or %d or 500\n", packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarMessage));
        }
        web->recvUDPCounter++;
        web->recvUDPBytes+=packetSize;

      } //packetSize
      else {
      }
    }

    //remove inactive instances
    for (std::vector<InstanceInfo>::iterator instance=instances.begin(); instance!=instances.end(); ) {
      if (millis() - instance->timeStamp > 32000) { //assuming a ping each 30 seconds
        instance = instances.erase(instance);
        // ppf("insTbl remove inactive instances %d\n", instance->ip[3]);

        for (JsonObject childVar: mdl->varChildren("insTbl"))
          ui->callVarFun(childVar, UINT8_MAX, f_ValueFun); //no rowNr so all rows updated

        ui->callVarFun("ddpInst", UINT8_MAX, f_UIFun);
        ui->callVarFun("artInst", UINT8_MAX, f_UIFun);
        ui->callVarFun("sma", UINT8_MAX, f_UIFun);
      }
      else
        ++instance;
    }
  }

  void sendSysInfoUDP()
  {
    if(!mdls->isConnected) return;
    if (!udp2Connected) return;

    IPAddress localIP = WiFi.localIP();
    if (!localIP || localIP == IPAddress(255,255,255,255)) localIP = IPAddress(4,3,2,1);

    UDPStarMessage starMessage;
    starMessage.header.token = 255; //WLED only accepts 255
    starMessage.header.id = 1; //WLED only accepts 1
    starMessage.header.ip0 = localIP[0];
    starMessage.header.ip1 = localIP[1];
    starMessage.header.ip2 = localIP[2];
    starMessage.header.ip3 = localIP[3];
    const char * instanceName = mdl->getValue("instanceName");
    strncpy(starMessage.header.name, instanceName?instanceName:_INIT(TOSTRING(APP)), sizeof(starMessage.header.name)-1);
    #if defined(CONFIG_IDF_TARGET_ESP32S2)
      starMessage.header.type = 33;
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
      starMessage.header.type = 34;
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
      starMessage.header.type = 35;
    #elif defined(ESP32)
      starMessage.header.type = 32;
    #else //???
      prf("dev unknown board\n");
      starMessage.header.type = 0;
    #endif
    starMessage.header.insId = localIP[3]; //WLED: used in map of instances as index!
    starMessage.header.version = VERSION;
    starMessage.sysData.type = (strcmp(_INIT(TOSTRING(APP)), "StarBase")==0)?1:(strcmp(_INIT(TOSTRING(APP)), "StarLeds")==0)?2:3; //1=StarBase,2=StarLeds, 3=StarFork
    starMessage.sysData.upTime = millis()/1000;
    starMessage.sysData.syncMaster = mdl->getValue("sma");
    starMessage.sysData.dmx.universe = 0;
    starMessage.sysData.dmx.start = 0;
    starMessage.sysData.dmx.count = 0;
    #ifdef STARBASE_USERMOD_E131
      if (e131mod->isEnabled) {
        starMessage.sysData.dmx.universe = mdl->getValue("dun");
        starMessage.sysData.dmx.start = mdl->getValue("dch");
        starMessage.sysData.dmx.count = 3;//e131->varsToWatch.size();
      }
    #endif

    updateInstance(starMessage); //temp? to show own instance in list as instance is not catching it's own udp message...

    //other way around: first set instance variables, then fill starMessage
    for (InstanceInfo &instance: instances) {
      if (instance.ip == WiFi.localIP()) {
        instance.jsonData.to<JsonObject>(); //clear

        //send dash values
        mdl->findVars("dash", true, [&instance](JsonObject var) { //varFun
          // print->printJson("setVar", var);
          JsonArray valArray = mdl->varValArray(var);
          if (valArray.isNull())
            instance.jsonData[mdl->varID(var)] = var["value"];
          else if (valArray.size())
            instance.jsonData[mdl->varID(var)] = valArray;
        });

        serializeJson(instance.jsonData, starMessage.jsonString);
        ppf("udp json String send ip:%d s:%s\n", instance.ip[3], starMessage.jsonString);
        // print->printJson(" d:", instance.jsonData);
        // print->printJDocInfo("   info", instance.jsonData);
      }
    }

    // broadcast to network
    if (0 != instanceUDP.beginPacket(IPAddress(255, 255, 255, 255), instanceUDPPort)) {  // WLEDMM beginPacket == 0 --> error
      // ppf("sendSysInfoUDP %s s:%d p:%d i:...%d\n", starMessage.header.name, sizeof(UDPStarMessage), instanceUDPPort, localIP[3]);
      // for (size_t x = 0; x < sizeof(UDPWLEDMessage) + sizeof(SysData); x++) {
      //   char * xx = (char *)&starMessage;
      //   Serial.printf("%d: %d - %c\n", x, xx[x], xx[x]);
      // }

      instanceUDP.write((byte*)&starMessage, sizeof(UDPStarMessage));
      web->sendUDPCounter++;
      web->sendUDPBytes+=sizeof(UDPStarMessage);
      instanceUDP.endPacket();
    }
    else {
      ppf("sendSysInfoUDP error\n");
    }
  }

  void updateInstance( UDPStarMessage udpStarMessage) {
    IPAddress messageIP = IPAddress(udpStarMessage.header.ip0, udpStarMessage.header.ip1, udpStarMessage.header.ip2, udpStarMessage.header.ip3);

    bool instanceFound = false;
    for (InstanceInfo &instance : instances) {
      if (instance.ip == messageIP)
        instanceFound = true;
    }

    // ppf("updateInstance Instance: ...%d n:%s found:%d\n", messageIP[3], udpStarMessage.header.name, instanceFound);

    if (!instanceFound) { //new instance
      InstanceInfo instance;
      instance.ip = messageIP;
      if (udpStarMessage.sysData.type == 0) {//WLED only
        instance.sysData.type = 0; //WLED
        //updated in udp sync message:
        instance.sysData.upTime = 0;
        instance.sysData.dmx.universe = 0;
        instance.sysData.dmx.start = 0;
        instance.sysData.dmx.count = 0;
        instance.sysData.syncMaster = 0;
        //dash values default 0
        instance.jsonData.to<JsonObject>();
      }

      instances.push_back(instance);
      std::sort(instances.begin(),instances.end(), [](InstanceInfo &a, InstanceInfo &b){ return a.ip < b.ip; });//Sorting the vector strcmp(a.name,b.name);
    }

    //update the instance in the instances array with the message data

    // stackUnsigned8 rowNr = 0;
    for (InstanceInfo &instance: instances) {
      if (instance.ip == messageIP) {
        instance.timeStamp = millis(); //update timestamp
        strncpy(instance.name, udpStarMessage.header.name, sizeof(instance.name)-1);
        instance.version = udpStarMessage.header.version;
        if (instance.ip == WiFi.localIP()) {
          esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_STA, instance.sysData.macAddress);
          // ppf("macaddress %02X:%02X:%02X:%02X:%02X:%02X\n", instance.macAddress[0], instance.macAddress[1], instance.macAddress[2], instance.macAddress[3], instance.macAddress[4], instance.macAddress[5]);
        }
        if (udpStarMessage.sysData.type >= 1) {//StarBase, StarLeds and forks only
          instance.sysData = udpStarMessage.sysData;

          if (instance.ip != WiFi.localIP()) { //send from localIP will be done after updateInstance
            //set instance.jsonData from new string
            JsonDocument newData;
            DeserializationError error = deserializeJson(newData, udpStarMessage.jsonString);
            if (error || !newData.is<JsonObject>()) {
              ppf("udp json String receive failed ip:%d e:%s\n", instance.ip[3], error.c_str(), udpStarMessage.jsonString);
            }
            else {
              stackUnsigned8 syncMaster = mdl->getValue("sma");
              if (syncMaster == messageIP[3]) { //if the message is from the master, set all values from the master
                for (JsonPair pair: newData.as<JsonObject>()) {
                  if (pair.key() != "sma" && pair.key() != "dch") { // do not sync sma and dch !!!
                    ppf("updateInstance sync from i:%s k:%s v:%s\n", instance.name, pair.key().c_str(), pair.value().as<String>().c_str());
                    // if (mdl->getValue(pair.key().c_str) != pair.value())
                    mdl->setValue(pair.key().c_str(), pair.value());
                  }
                }
              }

              instance.jsonData = newData; // deepcopy: https://github.com/bblanchon/ArduinoJson/issues/1023
              ppf("udp json String receive ip:%d", instance.ip[3]);
              print->printJson(" d:", instance.jsonData);
            }
          }
        }

        //only update cell in instbl!
        //create a json string
        //send the json
        //ui to parse the json

        if (instanceFound) {
          // JsonObject responseObject = web->getResponseObject();

          // responseObject["updRow"]["id"] = "insTbl";
          // responseObject["updRow"]["rowNr"] = rowNr;
          // responseObject["updRow"]["value"].to<JsonArray>();
          // addTblRow(responseObject["updRow"]["value"], instance);

          // web->sendResponseObject();

          // ppf("updateInstance updRow[%d] %s\n", instance - instances.begin(), instances[instance - instances.begin()].name);

          for (JsonObject childVar: mdl->varChildren("insTbl"))
            ui->callVarFun(childVar, UINT8_MAX, f_ValueFun); //rowNr instance - instances.begin()

          //tbd: now done for all rows, should be done only for updated rows!
        }

      } //ip
      // rowNr++;
    } // for instances

    if (!instanceFound) {
      // ppf("insTbl new instance %s\n", messageIP.toString().c_str());
      
      ui->callVarFun("ddpInst", UINT8_MAX, f_UIFun); //UiFun as select changes
      ui->callVarFun("artInst", UINT8_MAX, f_UIFun);
      ui->callVarFun("sma", UINT8_MAX, f_UIFun);

      // ui->processUiFun("insTbl");
      //run though it sorted to find the right rowNr
      // for (std::vector<InstanceInfo>::iterator instance=instances.begin(); instance!=instances.end(); ++instance) {
      //   if (instance->ip == messageIP) {
          for (JsonObject childVar: mdl->varChildren("insTbl")) {
            ui->callVarFun(childVar, UINT8_MAX, f_ValueFun); //no rowNr, update all
          }
      //   }
      // }
    }
  }

  InstanceInfo * findInstance(IPAddress ip) {

    bool instanceFound = false;
    for (InstanceInfo &instance : instances) {
      if (instance.ip == ip)
        instanceFound = true;
    }

    if (!instanceFound) { //instance always found
      InstanceInfo instance;
      instance.ip = ip;
      instances.push_back(instance);
      std::sort(instances.begin(),instances.end(), [](InstanceInfo &a, InstanceInfo &b){ return a.ip < b.ip; });//Sorting the vector strcmp(a.name,b.name);
    }

    // InstanceInfo foundInstance;
    //iterate vector pointers so we can update the instances
    for (InstanceInfo &instance : instances) {
      if (instance.ip == ip) {
        return &instance;
      }
    }

    return nullptr;
  }

  private:
    //sync (only WLED)
    WiFiUDP notifierUdp;
    unsigned16 notifierUDPPort = 21324;
    bool udpConnected = false;

    //instances (WLED and StarBase)
    WiFiUDP instanceUDP;
    unsigned16 instanceUDPPort = 65506;
    bool udp2Connected = false;

};

extern UserModInstances *instances;
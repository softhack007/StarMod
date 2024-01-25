/*
   @title     StarMod
   @file      SysModFiles.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModFiles.h"
#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModPrint.h"
#include "SysModModel.h"

// #include <FS.h>

bool SysModFiles::filesChanged = false;

SysModFiles::SysModFiles() :SysModule("Files") {
  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  if (!LittleFS.begin(true)) { //true: formatOnFail
    USER_PRINTF(" An Error has occurred while mounting File system");
    USER_PRINTF(" fail\n");
    success = false;
  }

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
};

//setup filesystem
void SysModFiles::setup() {
  SysModule::setup();
  parentVar = ui->initSysMod(parentVar, name);

  JsonObject tableVar = ui->initTable(parentVar, "fileTbl", nullptr, false, [this](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Files");
    web->addResponse(var["id"], "comment", "List of files");
    JsonArray rows = web->addResponseA(var["id"], "value");  //overwrite the value
    dirToJson(rows);
  }, [this](JsonObject var, uint8_t rowNr) { //chFun
    if (strcmp(var["value"], "delRow") == 0) {
      USER_PRINTF("fileTbl chFun %s %d %s\n", var["id"].as<const char *>(), rowNr, var["value"].as<String>().c_str());
      if (rowNr != UINT8_MAX) {
        // call uiFun of tbl to fill responseVariant with files
        ui->uFunctions[var["uiFun"]](var);
        //get the table values
        JsonVariant responseVariant = web->getResponseDoc()->as<JsonVariant>();
        JsonArray row = responseVariant["fileTbl"]["value"][rowNr];
        if (!row.isNull()) {
          const char * fileName = row[0]; //first column
          print->printJson("delete file", row);
          this->removeFiles(fileName, false);
        }
      }
      else {
        USER_PRINTF(" no rowNr!!\n");
      }
      print->printJson(" ", var);
    }
    else if (strcmp(var["value"], "insRow") == 0) {
      USER_PRINTF("fileTbl chFun %s %d %s\n", var["id"].as<const char *>(), rowNr, var["value"].as<String>().c_str());
      //add a row with all defaults
    }
  });

  ui->initText(tableVar, "flName", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Name");
  });
  ui->initNumber(tableVar, "flSize", UINT16_MAX, 0, UINT16_MAX, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Size (B)");
  });
  ui->initURL(tableVar, "flLink", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Show");
  });

  ui->initText(parentVar, "drsize", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponseV(var["id"], "value", "%d / %d B", files->usedBytes(), files->totalBytes());
    web->addResponse(var["id"], "label", "Total FS size");
  });

  // ui->initURL(parentVar, "urlTest", "file/3DCube202005.json", true);

  web->addUpload("/upload");
  web->addUpdate("/update");

  web->addFileServer("/file");

}

void SysModFiles::loop(){
  // SysModule::loop();

  // if (millis() - secondMillis >= 10000) {
  //   secondMillis = millis();
  //       // if something changed in fileTbl
  // }

  if (filesChanged) {
    mdl->setValueP("drsize", "%d / %d B", usedBytes(), totalBytes());
    filesChanged = false;

    ui->processUiFun("fileTbl");
  }
}

bool SysModFiles::remove(const char * path) {
  filesChange();
  USER_PRINTF("File remove %s\n", path);
  return LittleFS.remove(path);
}

size_t SysModFiles::usedBytes() {
  return LittleFS.usedBytes();
}

size_t SysModFiles::totalBytes() {
  return LittleFS.totalBytes();
}

File SysModFiles::open(const char * path, const char * mode, const bool create) {
  return LittleFS.open(path, mode, create);
}

// https://techtutorialsx.com/2019/02/24/esp32-arduino-listing-files-in-a-spiffs-file-system-specific-path/
void SysModFiles::dirToJson(JsonArray array, bool nameOnly, const char * filter) {
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {

    if (filter == nullptr || strstr(file.name(), filter) != nullptr) {
      if (nameOnly) {
        array.add(JsonString(file.name(), JsonString::Copied));
      }
      else {
        JsonArray row = array.createNestedArray();
        row.add(JsonString(file.name(), JsonString::Copied));
        row.add(file.size());
        char urlString[32] = "file/";
        strncat(urlString, file.name(), sizeof(urlString)-1);
        row.add(JsonString(urlString, JsonString::Copied));
      }
      // USER_PRINTF("FILE: %s %d\n", file.name(), file.size());
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}

void SysModFiles::filesChange() {
  filesChanged = true;
}

bool SysModFiles::seqNrToName(char * fileName, size_t seqNr) {

  File root = LittleFS.open("/");
  File file = root.openNextFile();

  size_t counter = 0;
  while (file) {
    if (counter == seqNr) {
      USER_PRINTF("seqNrToName: %s %d\n", file.name(), file.size());
      root.close();
      strncat(fileName, "/", 31); //add root prefix, fileName is 32 bytes but sizeof doesn't know so cheating
      strncat(fileName, file.name(), 31);
      file.close();
      return true;
    }

    file.close();
    file = root.openNextFile();
    counter++;
  }

  root.close();
  return false;
}

bool SysModFiles::readObjectFromFile(const char* path, JsonDocument* dest) {
  // if (doCloseFile) closeFile();
  File f = open(path, "r");
  if (!f) {
    USER_PRINTF("File %s open not successful\n", path);
    return false;
  }
  else { 
    USER_PRINTF(PSTR("File %s open to read, size %d bytes\n"), path, (int)f.size());
    DeserializationError error = deserializeJson(*dest, f);
    if (error) {
      print->printJDocInfo("readObjectFromFile", *dest);
      USER_PRINTF("readObjectFromFile deserializeJson failed with code %s\n", error.c_str());
      f.close();
      return false;
    } else {
      f.close();
      return true;
    }
  }
}

//candidate for deletion as taken over by JsonRDWS
// bool SysModFiles::writeObjectToFile(const char* path, JsonDocument* dest) {
//   File f = open(path, "w");
//   if (f) {
//     print->println("  success");
//     serializeJson(*dest, f);
//     f.close();
//     filesChange();
//     return true;
//   } else {
//     print->println("  fail");
//     return false;
//   }
// }

void SysModFiles::removeFiles(const char * filter, bool reverse) {
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {
    if (filter == nullptr || reverse?strstr(file.name(), filter) == nullptr: strstr(file.name(), filter) != nullptr) {
      char fileName[32] = "/";
      strncat(fileName, file.name(), sizeof(fileName)-1);
      file.close(); //close otherwise not removeable
      remove(fileName);
    }
    else
      file.close();

    file = root.openNextFile();
  }

  root.close();
}

bool SysModFiles::readFile(const char * path) {
  File f = open(path, "r");
  if (f) {

    while(f.available()) {
      Serial.print((char)f.read());
    }
    Serial.println();

    f.close();
    return true;
  }
  else 
    return false;
}

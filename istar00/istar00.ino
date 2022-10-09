/*
 * ISTAR Arduino IDE compatibility file.
 * 
 * Where has everything gone?
 * 
 * In April 2020, the project's structure underwent a major change. 
 * Global variables are now found in file "istar.h"
 * Global function declarations are found in "fcn_declare.h"
 * 
 * Usermod compatibility: Existing istar06_usermod.ino mods should continue to work. Delete usermod.cpp.
 * New usermods should use usermod.cpp instead.
 */
#include "istar.h"

void setup() {
  ISTAR::instance().setup();
}

void loop() {
  ISTAR::instance().loop();
}

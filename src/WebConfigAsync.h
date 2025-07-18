/*
__________           .___      .__  .__                 _____  .__       .__     ___ ________________    ___
\______   \ ____   __| _/____  |  | |__| ____   ____   /     \ |__| ____ |__|   /  / \__    ___/     \   \  \
 |     ___// __ \ / __ |\__  \ |  | |  |/    \ /  _ \ /  \ /  \|  |/    \|  |  /  /    |    | /  \ /  \   \  \
 |    |   \  ___// /_/ | / __ \|  |_|  |   |  (  <_> )    Y    \  |   |  \  | (  (     |    |/    Y    \   )  )
 |____|    \___  >____ |(____  /____/__|___|  /\____/\____|__  /__|___|  /__|  \  \    |____|\____|__  /  /  /
               \/     \/     \/             \/               \/        \/       \__\                 \/  /__/
                                                                                   (c) 2018-2024 alf45star
                                                                       https://github.com/alf45tar/PedalinoMini
 */

String theme         = "bootstrap";
String httpUsername  = "";
String httpPassword  = "";
bool   authenticated = false;

#ifdef NOWIFI
inline void http_run() {};
#else

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <StreamString.h>
#include <FS.h>
#include <SPIFFS.h>
#include <nvs.h>

#include "Version.h"

AsyncWebServer          httpServer(80);

#ifdef WEBCONFIG

#ifdef WEBSOCKET
AsyncWebSocket               webSocket("/ws");
AsyncEventSource             events("/events");    // EventSource is single direction, text-only protocol.
AsyncWebSocketMessageBuffer *buffer = NULL;
AsyncWebSocketClient        *wsClient = NULL;
#endif

#define WEBPAGE_MEMORY_ALLOCATION 8192    // To avoid memory fragmentation keep web page chunk smaller than allocated space

String page          = "";
String alert         = "";
String alertError    = "";
String uiprofile     = "1";
String uibank        = "1";
String uicontrol     = "All";
String uicontrolpage = "1";
String uisequence    = "1";

#define CONTROLS_PER_PAGE   10

bool fullPageCompleted = false;

int networks = 0;


bool trim_page(unsigned int start, unsigned int len, bool lastcall = false) {

  static unsigned int skipped = 0;
  unsigned int fullPageLength;

  const unsigned int saved = page.length();

  if (saved > WEBPAGE_MEMORY_ALLOCATION) {
    DPRINT("Memory fragmentation warning: webpage memory allocation %d bytes greater then %d bytes reserved (requested %d bytes).\n%s\n", saved, WEBPAGE_MEMORY_ALLOCATION, len, page.c_str());
    // Feed the watchdog during heavy page generation to prevent timeout
    vTaskDelay(1);
  }

  if (fullPageCompleted) {
    skipped = 0;
    fullPageCompleted = false;
  }

  fullPageLength = skipped + saved;

  // Start is after last addition
  if (start > (fullPageLength - 1)) {
    page = "";
    skipped += saved;
    return false;
  }

  // Start is in the last addition
  if (start > (fullPageLength - saved)) {
    page.remove(0, start - (fullPageLength - saved));
    skipped += start - (fullPageLength - saved);
  }

  // Finish reached
  if (fullPageLength >= (start + len) || lastcall) {
    page.remove(len - 1);
    skipped = 0;
    return true;
  }

  return false;
}


bool get_top_page(int p, unsigned int start, unsigned int len) {

  if (reloadProfile) controller_run();

  page = "";

  page += F("<!doctype html>");
  page += F("<html lang='en'>");
  page += F("<head>");
  page += F("<title>PedalinoMini&trade;</title>");
  page += F("<meta charset='utf-8'>");
  page += F(" <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>");
  if ( theme == "bootstrap" ) {
  #ifdef BOOTSTRAP_LOCAL
    page += F("<link rel='stylesheet' href='/css/bootstrap.min.css' integrity='sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH' crossorigin='anonymous'>");
  #else
    page += F("<link href='https://cdn.jsdelivr.net/npm/bootstrap@latest/dist/css/bootstrap.min.css' rel='stylesheet' crossorigin='anonymous'>");
  #endif
  } else {
    page += F("<link href='https://cdn.jsdelivr.net/npm/bootswatch@latest/dist/");
    page += theme;
    page += F("/bootstrap.min.css' rel='stylesheet' crossorigin='anonymous'>");
  }
  if (p == 2) page += F("<script src='/js/Sortable.min.js' integrity='' crossorigin='anonymous'></script>");
  page += F("</head>");

  page += F("<body>");

  if (trim_page(start, len)) return true;

  if (p >= 0) {
  page += F("<div class='container-fluid mt-3 mb-3'>");

  page += F("<nav class='navbar navbar-expand-md navbar-light bg-light mb-3'>");
  page += F("<div class='container-fluid'>");
  page += F("<a class='navbar-brand' href='/'>");
  page += F("<img src='/logo.png' width='30' height='30' class='d-inline-block align-top' alt=''></a>");
  page += F("<button class='navbar-toggler' type='button' data-bs-toggle='collapse' data-bs-target='#navbarNavDropdown' aria-controls='navbarNavDropdown' aria-expanded='false' aria-label='Toggle navigation'>");
  page += F("<span class='navbar-toggler-icon'></span>");
  page += F("</button>");
  page += F("<div class='collapse navbar-collapse' id='navbarNavDropdown'>");
  page += F("<ul class='navbar-nav mr-auto'>");
  //page += F("<li class='nav-item");
  //page += (p == 1 ? F(" active'>") : F("'>"));
  //page += F("<a class='nav-link' href='/live'>Live</a>");
  //page += F("</li>");

  if (trim_page(start, len)) return true;

  page += F("<li class='nav-item");
  page += (p == 2 ? F(" active'>") : F("'>"));
  page += F("<a class='nav-link' href='/actions'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-clipboard-check' viewBox='0 0 16 16'>");
  page += F("<path fill-rule='evenodd' d='M10.854 7.146a.5.5 0 0 1 0 .708l-3 3a.5.5 0 0 1-.708 0l-1.5-1.5a.5.5 0 1 1 .708-.708L7.5 9.793l2.646-2.647a.5.5 0 0 1 .708 0z'/>");
  page += F("<path d='M4 1.5H3a2 2 0 0 0-2 2V14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V3.5a2 2 0 0 0-2-2h-1v1h1a1 1 0 0 1 1 1V14a1 1 0 0 1-1 1H3a1 1 0 0 1-1-1V3.5a1 1 0 0 1 1-1h1v-1z'/>");
  page += F("<path d='M9.5 1a.5.5 0 0 1 .5.5v1a.5.5 0 0 1-.5.5h-3a.5.5 0 0 1-.5-.5v-1a.5.5 0 0 1 .5-.5h3zm-3-1A1.5 1.5 0 0 0 5 1.5v1A1.5 1.5 0 0 0 6.5 4h3A1.5 1.5 0 0 0 11 2.5v-1A1.5 1.5 0 0 0 9.5 0h-3z'/>");
  page += F("</svg>");
  page += F(" Actions</a>");
  page += F("</li>");

  if (trim_page(start, len)) return true;

  page += F("<li class='nav-item");
  page += (p == 4 ? F(" active'>") : F("'>"));
  page += F("<a class='nav-link' href='/pedals'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-controller' viewBox='0 0 16 16'>");
  page += F("<path d='M11.5 6.027a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0zm-1.5 1.5a.5.5 0 1 0 0-1 .5.5 0 0 0 0 1zm2.5-.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0zm-1.5 1.5a.5.5 0 1 0 0-1 .5.5 0 0 0 0 1zm-6.5-3h1v1h1v1h-1v1h-1v-1h-1v-1h1v-1z'/>");
  page += F("<path d='M3.051 3.26a.5.5 0 0 1 .354-.613l1.932-.518a.5.5 0 0 1 .62.39c.655-.079 1.35-.117 2.043-.117.72 0 1.443.041 2.12.126a.5.5 0 0 1 .622-.399l1.932.518a.5.5 0 0 1 .306.729c.14.09.266.19.373.297.408.408.78 1.05 1.095 1.772.32.733.599 1.591.805 2.466.206.875.34 1.78.364 2.606.024.816-.059 1.602-.328 2.21a1.42 1.42 0 0 1-1.445.83c-.636-.067-1.115-.394-1.513-.773-.245-.232-.496-.526-.739-.808-.126-.148-.25-.292-.368-.423-.728-.804-1.597-1.527-3.224-1.527-1.627 0-2.496.723-3.224 1.527-.119.131-.242.275-.368.423-.243.282-.494.575-.739.808-.398.38-.877.706-1.513.773a1.42 1.42 0 0 1-1.445-.83c-.27-.608-.352-1.395-.329-2.21.024-.826.16-1.73.365-2.606.206-.875.486-1.733.805-2.466.315-.722.687-1.364 1.094-1.772a2.34 2.34 0 0 1 .433-.335.504.504 0 0 1-.028-.079zm2.036.412c-.877.185-1.469.443-1.733.708-.276.276-.587.783-.885 1.465a13.748 13.748 0 0 0-.748 2.295 12.351 12.351 0 0 0-.339 2.406c-.022.755.062 1.368.243 1.776a.42.42 0 0 0 .426.24c.327-.034.61-.199.929-.502.212-.202.4-.423.615-.674.133-.156.276-.323.44-.504C4.861 9.969 5.978 9.027 8 9.027s3.139.942 3.965 1.855c.164.181.307.348.44.504.214.251.403.472.615.674.318.303.601.468.929.503a.42.42 0 0 0 .426-.241c.18-.408.265-1.02.243-1.776a12.354 12.354 0 0 0-.339-2.406 13.753 13.753 0 0 0-.748-2.295c-.298-.682-.61-1.19-.885-1.465-.264-.265-.856-.523-1.733-.708-.85-.179-1.877-.27-2.913-.27-1.036 0-2.063.091-2.913.27z'/>");
  page += F("</svg>");
  page += F(" Pedals</a>");
  page += F("</li>");

  if (trim_page(start, len)) return true;

  page += F("<li class='nav-item");
  page += (p == 4 ? F(" active'>") : F("'>"));
  page += F("<a class='nav-link' href='/virtualpedals'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-toggles' viewBox='0 0 16 16'>");
  page += F("<path d='M4.5 9a3.5 3.5 0 1 0 0 7h7a3.5 3.5 0 1 0 0-7h-7zm7 6a2.5 2.5 0 1 1 0-5 2.5 2.5 0 0 1 0 5zm-7-14a2.5 2.5 0 1 0 0 5 2.5 2.5 0 0 0 0-5zm2.45 0A3.49 3.49 0 0 1 8 3.5 3.49 3.49 0 0 1 6.95 6h4.55a2.5 2.5 0 0 0 0-5H6.95zM4.5 0h7a3.5 3.5 0 1 1 0 7h-7a3.5 3.5 0 1 1 0-7z'/>");
  page += F("</svg>");
  page += F(" Controls</a>");
  page += F("</li>");

  if (trim_page(start, len)) return true;

  page += F("<li class='nav-item");
  page += (p == 5 ? F(" active'>") : F("'>"));
  page += F("<a class='nav-link' href='/interfaces'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-hdd-network' viewBox='0 0 16 16'>");
  page += F("<path d='M4.5 5a.5.5 0 1 0 0-1 .5.5 0 0 0 0 1zM3 4.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0z'/>");
  page += F("<path d='M0 4a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v1a2 2 0 0 1-2 2H8.5v3a1.5 1.5 0 0 1 1.5 1.5h5.5a.5.5 0 0 1 0 1H10A1.5 1.5 0 0 1 8.5 14h-1A1.5 1.5 0 0 1 6 12.5H.5a.5.5 0 0 1 0-1H6A1.5 1.5 0 0 1 7.5 10V7H2a2 2 0 0 1-2-2V4zm1 0v1a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V4a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1zm6 7.5v1a.5.5 0 0 0 .5.5h1a.5.5 0 0 0 .5-.5v-1a.5.5 0 0 0-.5-.5h-1a.5.5 0 0 0-.5.5z'/>");
  page += F("</svg>");
  page += F(" Interfaces</a>");
  page += F("</li>");

  if (trim_page(start, len)) return true;

  page += F("<li class='nav-item");
  page += (p == 6 ? F(" active'>") : F("'>"));
  page += F("<a class='nav-link' href='/sequences'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-camera-reels' viewBox='0 0 16 16'>");
  page += F("<path d='M6 3a3 3 0 1 1-6 0 3 3 0 0 1 6 0zM1 3a2 2 0 1 0 4 0 2 2 0 0 0-4 0z'/>");
  page += F("<path d='M9 6h.5a2 2 0 0 1 1.983 1.738l3.11-1.382A1 1 0 0 1 16 7.269v7.462a1 1 0 0 1-1.406.913l-3.111-1.382A2 2 0 0 1 9.5 16H2a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h7zm6 8.73V7.27l-3.5 1.555v4.35l3.5 1.556zM1 8v6a1 1 0 0 0 1 1h7.5a1 1 0 0 0 1-1V8a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1z'/>");
  page += F("<path d='M9 6a3 3 0 1 0 0-6 3 3 0 0 0 0 6zM7 3a2 2 0 1 1 4 0 2 2 0 0 1-4 0z'/>");
  page += F("</svg>");
  page += F(" Sequences</a>");
  page += F("</li>");

  if (trim_page(start, len)) return true;

  page += F("<li class='nav-item");
  page += (p == 7 ? F(" active'>") : F("'>"));
  page += F("<a class='nav-link' href='/options'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-gear' viewBox='0 0 16 16'>");
  page += F("<path d='M8 4.754a3.246 3.246 0 1 0 0 6.492 3.246 3.246 0 0 0 0-6.492zM5.754 8a2.246 2.246 0 1 1 4.492 0 2.246 2.246 0 0 1-4.492 0z'/>");
  page += F("<path d='M9.796 1.343c-.527-1.79-3.065-1.79-3.592 0l-.094.319a.873.873 0 0 1-1.255.52l-.292-.16c-1.64-.892-3.433.902-2.54 2.541l.159.292a.873.873 0 0 1-.52 1.255l-.319.094c-1.79.527-1.79 3.065 0 3.592l.319.094a.873.873 0 0 1 .52 1.255l-.16.292c-.892 1.64.901 3.434 2.541 2.54l.292-.159a.873.873 0 0 1 1.255.52l.094.319c.527 1.79 3.065 1.79 3.592 0l.094-.319a.873.873 0 0 1 1.255-.52l.292.16c1.64.893 3.434-.902 2.54-2.541l-.159-.292a.873.873 0 0 1 .52-1.255l.319-.094c1.79-.527 1.79-3.065 0-3.592l-.319-.094a.873.873 0 0 1-.52-1.255l.16-.292c.893-1.64-.902-3.433-2.541-2.54l-.292.159a.873.873 0 0 1-1.255-.52l-.094-.319zm-2.633.283c.246-.835 1.428-.835 1.674 0l.094.319a1.873 1.873 0 0 0 2.693 1.115l.291-.16c.764-.415 1.6.42 1.184 1.185l-.159.292a1.873 1.873 0 0 0 1.116 2.692l.318.094c.835.246.835 1.428 0 1.674l-.319.094a1.873 1.873 0 0 0-1.115 2.693l.16.291c.415.764-.42 1.6-1.185 1.184l-.291-.159a1.873 1.873 0 0 0-2.693 1.116l-.094.318c-.246.835-1.428.835-1.674 0l-.094-.319a1.873 1.873 0 0 0-2.692-1.115l-.292.16c-.764.415-1.6-.42-1.184-1.185l.159-.291A1.873 1.873 0 0 0 1.945 8.93l-.319-.094c-.835-.246-.835-1.428 0-1.674l.319-.094A1.873 1.873 0 0 0 3.06 4.377l-.16-.292c-.415-.764.42-1.6 1.185-1.184l.292.159a1.873 1.873 0 0 0 2.692-1.115l.094-.319z'/>");
  page += F("</svg>");
  page += F(" Options</a>");
  page += F("</li>");

  if (trim_page(start, len)) return true;

  page += F("<li class='nav-item");
  page += (p == 8 ? F(" active'>") : F("'>"));
  page += F("<a class='nav-link' href='/configurations'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-file-code' viewBox='0 0 16 16'>");
  page += F("<path d='M6.646 5.646a.5.5 0 1 1 .708.708L5.707 8l1.647 1.646a.5.5 0 0 1-.708.708l-2-2a.5.5 0 0 1 0-.708l2-2zm2.708 0a.5.5 0 1 0-.708.708L10.293 8 8.646 9.646a.5.5 0 0 0 .708.708l2-2a.5.5 0 0 0 0-.708l-2-2z'/>");
  page += F("<path d='M2 2a2 2 0 0 1 2-2h8a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V2zm10-1H4a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1z'/>");
  page += F("</svg>");
  page += F(" Configurations</a>");
  page += F("</li>");
  page += F("</ul>");
  }
  page += F("</div>");

  if (trim_page(start, len)) return true;

  if (p != -1 && p != 0 && p != 7)
  {
    page += F("<form class='d-flex'>");
    page += F("<div class='btn-group'>");
    page += currentProfile == 0 ? F("<a class='btn btn-primary' href='?profile=1' role='button'>A</a>") : F("<a class='btn btn-outline-primary' href='?profile=1' role='button'>A</a>");
    page += currentProfile == 1 ? F("<a class='btn btn-primary' href='?profile=2' role='button'>B</a>") : F("<a class='btn btn-outline-primary' href='?profile=2' role='button'>B</a>");
    page += currentProfile == 2 ? F("<a class='btn btn-primary' href='?profile=3' role='button'>C</a>") : F("<a class='btn btn-outline-primary' href='?profile=3' role='button'>C</a>");
    page += F("</div>");
    page += F("</form>");
  }
  page += F("</div>");
  page += F("</nav>");


/*
  page += F("<div class='d-flex flex-column bg-light' style='width: 4.5rem;'>");
  page += F("<a href='/' id='bootstrap' class='d-block p-3 link-dark text-decoration-none' title='PedalinoMini' data-bs-toggle='tooltip' data-bs-placement='right'>");
  page += F("<img src='/logo.png' width='32' height='32'>");
  page += F("<span class='visually-hidden'>Icon-only</span>");
  page += F("</a>");
  page += F("<ul class='nav nav-pills nav-flush flex-column mb-auto text-center'>");
  page += F("<li class='nav-item'>");
  page += F("<a href='/actions' class='nav-link active py-3 border-bottom' title='Actions' data-bs-toggle='tooltip' data-bs-placement='right'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' fill='currentColor' class='bi bi-clipboard-check' viewBox='0 0 16 16'>");
  page += F("<path fill-rule='evenodd' d='M10.854 7.146a.5.5 0 0 1 0 .708l-3 3a.5.5 0 0 1-.708 0l-1.5-1.5a.5.5 0 1 1 .708-.708L7.5 9.793l2.646-2.647a.5.5 0 0 1 .708 0z'/>");
  page += F("<path d='M4 1.5H3a2 2 0 0 0-2 2V14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V3.5a2 2 0 0 0-2-2h-1v1h1a1 1 0 0 1 1 1V14a1 1 0 0 1-1 1H3a1 1 0 0 1-1-1V3.5a1 1 0 0 1 1-1h1v-1z'/>");
  page += F("<path d='M9.5 1a.5.5 0 0 1 .5.5v1a.5.5 0 0 1-.5.5h-3a.5.5 0 0 1-.5-.5v-1a.5.5 0 0 1 .5-.5h3zm-3-1A1.5 1.5 0 0 0 5 1.5v1A1.5 1.5 0 0 0 6.5 4h3A1.5 1.5 0 0 0 11 2.5v-1A1.5 1.5 0 0 0 9.5 0h-3z'/>");
  page += F("<use xlink:href='#home'/></svg>");
  page += F("</a>");
  page += F("</li>");
  page += F("<li class='nav-item'>");
  page += F("<a href='/pedals' class='nav-link py-3 border-bottom' title='Pedals' data-bs-toggle='tooltip' data-bs-placement='right'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' fill='currentColor' class='bi bi-controller' viewBox='0 0 16 16'>");
  page += F("<path d='M11.5 6.027a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0zm-1.5 1.5a.5.5 0 1 0 0-1 .5.5 0 0 0 0 1zm2.5-.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0zm-1.5 1.5a.5.5 0 1 0 0-1 .5.5 0 0 0 0 1zm-6.5-3h1v1h1v1h-1v1h-1v-1h-1v-1h1v-1z'/>");
  page += F("<path d='M3.051 3.26a.5.5 0 0 1 .354-.613l1.932-.518a.5.5 0 0 1 .62.39c.655-.079 1.35-.117 2.043-.117.72 0 1.443.041 2.12.126a.5.5 0 0 1 .622-.399l1.932.518a.5.5 0 0 1 .306.729c.14.09.266.19.373.297.408.408.78 1.05 1.095 1.772.32.733.599 1.591.805 2.466.206.875.34 1.78.364 2.606.024.816-.059 1.602-.328 2.21a1.42 1.42 0 0 1-1.445.83c-.636-.067-1.115-.394-1.513-.773-.245-.232-.496-.526-.739-.808-.126-.148-.25-.292-.368-.423-.728-.804-1.597-1.527-3.224-1.527-1.627 0-2.496.723-3.224 1.527-.119.131-.242.275-.368.423-.243.282-.494.575-.739.808-.398.38-.877.706-1.513.773a1.42 1.42 0 0 1-1.445-.83c-.27-.608-.352-1.395-.329-2.21.024-.826.16-1.73.365-2.606.206-.875.486-1.733.805-2.466.315-.722.687-1.364 1.094-1.772a2.34 2.34 0 0 1 .433-.335.504.504 0 0 1-.028-.079zm2.036.412c-.877.185-1.469.443-1.733.708-.276.276-.587.783-.885 1.465a13.748 13.748 0 0 0-.748 2.295 12.351 12.351 0 0 0-.339 2.406c-.022.755.062 1.368.243 1.776a.42.42 0 0 0 .426.24c.327-.034.61-.199.929-.502.212-.202.4-.423.615-.674.133-.156.276-.323.44-.504C4.861 9.969 5.978 9.027 8 9.027s3.139.942 3.965 1.855c.164.181.307.348.44.504.214.251.403.472.615.674.318.303.601.468.929.503a.42.42 0 0 0 .426-.241c.18-.408.265-1.02.243-1.776a12.354 12.354 0 0 0-.339-2.406 13.753 13.753 0 0 0-.748-2.295c-.298-.682-.61-1.19-.885-1.465-.264-.265-.856-.523-1.733-.708-.85-.179-1.877-.27-2.913-.27-1.036 0-2.063.091-2.913.27z'/>");
  page += F("</svg>");
  page += F("</a>");
  page += F("</li>");
  page += F("<li class='nav-item'>");
  page += F("<a href='/interfaces' class='nav-link py-3 border-bottom' title='Interfaces' data-bs-toggle='tooltip' data-bs-placement='right'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' fill='currentColor' class='bi bi-hdd-network' viewBox='0 0 16 16'>");
  page += F("<path d='M4.5 5a.5.5 0 1 0 0-1 .5.5 0 0 0 0 1zM3 4.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0z'/>");
  page += F("<path d='M0 4a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v1a2 2 0 0 1-2 2H8.5v3a1.5 1.5 0 0 1 1.5 1.5h5.5a.5.5 0 0 1 0 1H10A1.5 1.5 0 0 1 8.5 14h-1A1.5 1.5 0 0 1 6 12.5H.5a.5.5 0 0 1 0-1H6A1.5 1.5 0 0 1 7.5 10V7H2a2 2 0 0 1-2-2V4zm1 0v1a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V4a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1zm6 7.5v1a.5.5 0 0 0 .5.5h1a.5.5 0 0 0 .5-.5v-1a.5.5 0 0 0-.5-.5h-1a.5.5 0 0 0-.5.5z'/>");
  page += F("</svg>");
  page += F("</a>");
  page += F("</li>");
  page += F("<li class='nav-item'>");
  page += F("<a href='/sequences' class='nav-link py-3 border-bottom' title='Sequences' data-bs-toggle='tooltip' data-bs-placement='right'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' fill='currentColor' class='bi bi-camera-reels' viewBox='0 0 16 16'>");
  page += F("<path d='M6 3a3 3 0 1 1-6 0 3 3 0 0 1 6 0zM1 3a2 2 0 1 0 4 0 2 2 0 0 0-4 0z'/>");
  page += F("<path d='M9 6h.5a2 2 0 0 1 1.983 1.738l3.11-1.382A1 1 0 0 1 16 7.269v7.462a1 1 0 0 1-1.406.913l-3.111-1.382A2 2 0 0 1 9.5 16H2a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h7zm6 8.73V7.27l-3.5 1.555v4.35l3.5 1.556zM1 8v6a1 1 0 0 0 1 1h7.5a1 1 0 0 0 1-1V8a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1z'/>");
  page += F("<path d='M9 6a3 3 0 1 0 0-6 3 3 0 0 0 0 6zM7 3a2 2 0 1 1 4 0 2 2 0 0 1-4 0z'/>");
  page += F("</svg>");
  page += F("</a>");
  page += F("</li>");
  page += F("<li class='nav-item'>");
  page += F("<a href='/options' class='nav-link py-3 border-bottom' title='Options' data-bs-toggle='tooltip' data-bs-placement='right'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' fill='currentColor' class='bi bi-gear' viewBox='0 0 16 16'>");
  page += F("<path d='M8 4.754a3.246 3.246 0 1 0 0 6.492 3.246 3.246 0 0 0 0-6.492zM5.754 8a2.246 2.246 0 1 1 4.492 0 2.246 2.246 0 0 1-4.492 0z'/>");
  page += F("<path d='M9.796 1.343c-.527-1.79-3.065-1.79-3.592 0l-.094.319a.873.873 0 0 1-1.255.52l-.292-.16c-1.64-.892-3.433.902-2.54 2.541l.159.292a.873.873 0 0 1-.52 1.255l-.319.094c-1.79.527-1.79 3.065 0 3.592l.319.094a.873.873 0 0 1 .52 1.255l-.16.292c-.892 1.64.901 3.434 2.541 2.54l.292-.159a.873.873 0 0 1 1.255.52l.094.319c.527 1.79 3.065 1.79 3.592 0l.094-.319a.873.873 0 0 1 1.255-.52l.292.16c1.64.893 3.434-.902 2.54-2.541l-.159-.292a.873.873 0 0 1 .52-1.255l.319-.094c1.79-.527 1.79-3.065 0-3.592l-.319-.094a.873.873 0 0 1-.52-1.255l.16-.292c.893-1.64-.902-3.433-2.541-2.54l-.292.159a.873.873 0 0 1-1.255-.52l-.094-.319zm-2.633.283c.246-.835 1.428-.835 1.674 0l.094.319a1.873 1.873 0 0 0 2.693 1.115l.291-.16c.764-.415 1.6.42 1.184 1.185l-.159.292a1.873 1.873 0 0 0 1.116 2.692l.318.094c.835.246.835 1.428 0 1.674l-.319.094a1.873 1.873 0 0 0-1.115 2.693l.16.291c.415.764-.42 1.6-1.185 1.184l-.291-.159a1.873 1.873 0 0 0-2.693 1.116l-.094.318c-.246.835-1.428.835-1.674 0l-.094-.319a1.873 1.873 0 0 0-2.692-1.115l-.292.16c-.764.415-1.6-.42-1.184-1.185l.159-.291A1.873 1.873 0 0 0 1.945 8.93l-.319-.094c-.835-.246-.835-1.428 0-1.674l.319-.094A1.873 1.873 0 0 0 3.06 4.377l-.16-.292c-.415-.764.42-1.6 1.185-1.184l.292.159a1.873 1.873 0 0 0 2.692-1.115l.094-.319z'/>");
  page += F("</svg>");
  page += F("</a>");
  page += F("</li>");
  page += F("<li class='nav-item'>");
  page += F("<a href='/configurations' class='nav-link py-3 border-bottom' title='Configurations' data-bs-toggle='tooltip' data-bs-placement='right'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' fill='currentColor' class='bi bi-file-code' viewBox='0 0 16 16'>");
  page += F("<path d='M6.646 5.646a.5.5 0 1 1 .708.708L5.707 8l1.647 1.646a.5.5 0 0 1-.708.708l-2-2a.5.5 0 0 1 0-.708l2-2zm2.708 0a.5.5 0 1 0-.708.708L10.293 8 8.646 9.646a.5.5 0 0 0 .708.708l2-2a.5.5 0 0 0 0-.708l-2-2z'/>");
  page += F("<path d='M2 2a2 2 0 0 1 2-2h8a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V2zm10-1H4a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1z'/>");
  page += F("</svg>");
  page += F("</a>");
  page += F("</li>");

  page += F("</ul>");
  page += F("<div class='dropdown border-top'>");
  page += F("<a href='#' class='d-flex align-items-center justify-content-center p-3 link-dark text-decoration-none dropdown-toggle' id='dropdownUser3' data-bs-toggle='dropdown' aria-expanded='false'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' fill='currentColor' class='bi bi-stack' viewBox='0 0 16 16'>");
  page += F("<path d='m14.12 10.163 1.715.858c.22.11.22.424 0 .534L8.267 15.34a.598.598 0 0 1-.534 0L.165 11.555a.299.299 0 0 1 0-.534l1.716-.858 5.317 2.659c.505.252 1.1.252 1.604 0l5.317-2.66zM7.733.063a.598.598 0 0 1 .534 0l7.568 3.784a.3.3 0 0 1 0 .535L8.267 8.165a.598.598 0 0 1-.534 0L.165 4.382a.299.299 0 0 1 0-.535L7.733.063z'/>");
  page += F("<path d='m14.12 6.576 1.715.858c.22.11.22.424 0 .534l-7.568 3.784a.598.598 0 0 1-.534 0L.165 7.968a.299.299 0 0 1 0-.534l1.716-.858 5.317 2.659c.505.252 1.1.252 1.604 0l5.317-2.659z'/>");
  page += F("</svg>");
  page += F("</a>");
  page += F("<ul class='dropdown-menu text-small shadow' aria-labelledby='dropdownUser3'>");
  page += F("<li><a class='dropdown-item' href='#'>Profile A</a></li>");
  page += F("<li><a class='dropdown-item' href='#'>Profile B</a></li>");
  page += F("<li><a class='dropdown-item' href='#'>Profile C</a></li>");
  page += F("<li><hr class='dropdown-divider'></li>");
  page += F("<li><a class='dropdown-item' href='#'>Sign out</a></li>");
  page += F("</ul>");
  page += F("</div>");
  page += F("</div>");
  page += F("<script>");
  page += F("(function () {\n"
              "'use strict'\n"
              "var tooltipTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle=\"tooltip\"]'))\n"
              "tooltipTriggerList.forEach(function (tooltipTriggerEl) {\n"
                "new bootstrap.Tooltip(tooltipTriggerEl)\n"
              "})\n"
            "})()\n");
  page += F("</script>");

  page += F("<div class='container mt-3 mb-3'>");
*/

  if (alert != "") {
    page += F("<div class='alert alert-success alert-dismissible fade show' role='alert'>");
    page += alert;
    page += F("<button type='button' class='btn-close' data-bs-dismiss='alert' aria-label='Close'></button>");
    page += F("</div>");
  }
  if (alertError != "") {
    page += F("<div class='alert alert-danger alert-dismissible fade show' role='alert'>");
    page += alertError;
    page += F("<button type='button' class='btn-close' data-bs-dismiss='alert' aria-label='Close'></button>");
    page += F("</div>");
  }

  if (trim_page(start, len)) return true;

  return false;
}

void get_footer_page() {

  page += F("</div>");
#ifdef BOOTSTRAP_LOCAL
  page += F("<script defer src='/js/bootstrap.bundle.min.js' integrity='sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz' crossorigin='anonymous'></script>");
#else
  page += F("<script src='https://cdn.jsdelivr.net/npm/bootstrap@latest/dist/js/bootstrap.bundle.min.js' crossorigin='anonymous'></script>");
#endif
  page += F("</body>");
  page += F("</html>");
}

void get_login_page() {

  get_top_page(-1, 0, 65535);

  page += F("<div class='col-4'>");
  page += F("</div>");

  page += F("<div class='col-4'>");
  page += F("<form class='form-signin'>");
  page += F("<div class='text-center mb-4'>");
  page += F("<img class='mb-4' src='/logo.png' alt='' width='64' height='64'>");
  page += F("<h1 class='h3 mb-3 font-weight-normal'>PedalinoMini&trade;</h1>");
  page += F("<p>Wireless MIDI foot controller <a href='https://github.com/alf45tar/PedalinoMini'>More info</a></p>");
  page += F("</div>");

  page += F("<div class='form-label-group'>");
  page += F("<input type='text' id='username' class='form-control' placeholder='Username' required='' autofocus=''>");
  page += F("<label for='username'>Username</label>");
  page += F("</div>");

  page += F("<div class='form-label-group'>");
  page += F("<input type='password' id='password' class='form-control' placeholder='Password' required=''>");
  page += F("<label for='password'>Password</label>");
  page += F("</div>");

  page += F("<div class='checkbox mb-3'>");
  page += F("<label>");
  page += F("<input type='checkbox' value='remember-me'> Remember me");
  page += F("</label>");
  page += F("</div>");
  page += F("<button class='btn btn-lg btn-primary btn-block' type='submit'>Sign in</button>");
  page += F("<p class='mt-5 mb-3 text-muted text-center'>© 2018-2019</p>");
  page += F("</form>");
  page += F("</div>");

  page += F("<div class='col-4'>");
  page += F("</div>");

  get_footer_page();
}

void get_root_page(unsigned int start, unsigned int len) {

  if (get_top_page(0, start, len)) return;

  page += F("<h4 class='display-4'>Wireless MIDI foot controller</h4>");

  page += F("<div class='row row-cols-1 row-cols-sm-2 row-cols-lg-4'>");

  page += F("<div class='col'>");
  page += F("<h3>Product</h3>");
  page += F("<dt>Model</dt><dd>");
  page += MODEL;
  page += F("</dd>");
  page += F("<dt>Profiles</dt><dd>");
  page += PROFILES;
  page += F("</dd>");
  page += F("<dt>Banks</dt><dd>");
  page += BANKS;
  page += F("</dd>");
  page += F("<dt>Pedals</dt><dd>");
  page += PEDALS;
  page += F("</dd>");
  page += F("<dt>Controls</dt><dd>");
  page += CONTROLS;
  page += F("</dd>");
  page += F("<dt>Sequences</dt><dd>");
  page += SEQUENCES;
  page += F("</dd>");
  page += F("<dt>Leds</dt><dd>");
  page += LEDS;
  page += F("</dd>");
  page += F("<dt>Boot Mode</dt><dd>");
  switch (bootMode) {
    case PED_BOOT_NORMAL:
      page += F("Normal");
      break;
    case PED_BOOT_BLE:
      page += F("BLE only");
      break;
    case PED_BOOT_WIFI:
      page += F("WiFi only");
      break;
    case PED_BOOT_AP:
      page += F("Access Point and BLE");
      break;
    case PED_BOOT_AP_NO_BLE:
      page += F("Access Point without BLE");
      break;
  }
  page += F("</dd>");
  page += F("<dt>PlatformIO ESP32 Platform</dt><dd>");
  page += xstr(ESP32_PLATFORM_VERSION);
  page += F("</dd>");
  page += F("<dt>IDF Version</dt><dd>");
  page += ESP_IDF_VERSION_MAJOR;
  page += F(".");
  page += ESP_IDF_VERSION_MINOR;
  page += F(".");
  page += ESP_IDF_VERSION_PATCH;
  page += F("</dd>");
  page += F("<dt>SDK Version</dt><dd>");
  page += ESP.getSdkVersion();
  page += F("</dd>");
/*
  page += F("<dt>Arduino Version</dt><dd>");
  page += ESP_ARDUINO_VERSION_MAJOR;
  page += F(".");
  page += ESP_ARDUINO_VERSION_MINOR;
  page += F(".");
  page += ESP_ARDUINO_VERSION_PATCH;
  page += F("</dd>");
*/
  page += F("<dt>PlatformIO Build Env</dt><dd>");
  page += xstr(PLATFORMIO_ENV);
  page += F("</dd>");

  page += F("<dt>Bootstrap Version</dt><dd><span id='bootstrap-version'></span></dd>");
  page += F("<script>"
            "   document.addEventListener('DOMContentLoaded', function() {"
            "     if (typeof bootstrap !== 'undefined') {"
            "       document.getElementById('bootstrap-version').textContent = bootstrap.Tooltip.VERSION;"
            "     } else {"
            "       document.getElementById('bootstrap-version').textContent = 'Not available';"
            "     }"
            "   });"
            "</script>");

  page += F("<dt>Firmware</dt><dd>");
  page += PEDALINO_VERSION_MAJOR;
  page += F(".");
  page += PEDALINO_VERSION_MINOR;
  page += F(".");
  page += PEDALINO_VERSION_PATCH;
  page += F("</dd>");
  page += F("<dt>Firmware Size</dt><dd>");
  page += sketchSize;
  page += F(" bytes</dd>");
  page += F("<dt>Firmware Hash</dt><dd>");
  page += sketchMD5;
  page += F("</dd>");
  page += F("</div>");
  
  if (trim_page(start, len)) return;

  page += F("<div class='col'>");
  page += F("<h3>Hardware</h3>");
  page += F("<dt>Board</dt><dd>");
  page += ARDUINO_BOARD;
  page += F("</dd>");
  page += F("<dt>Chip</dt><dd>");
  page += ESP.getChipModel();
  page += F("</dd>");
  page += F("<dt>Chip Revision</dt><dd>");
  page += ESP.getChipRevision();
  page += F("</dd>");
  page += F("</dd>");
  page += F("<dt>Chip ID</dt><dd>");
  page += getChipId();
  page += F("</dd>");
  page += F("<dt>CPU Frequency</dt><dd>");
  page += ESP.getCpuFreqMHz();
  page += F(" MHz</dd>");
  page += F("<dt>Flash Chip Frequency</dt><dd>");
  page += ESP.getFlashChipSpeed() / 1000000;
  page += F(" MHz</dd>");
  page += F("<dt>Flash Size</dt><dd>");
  page += ESP.getFlashChipSize() / (1024 * 1024);
  page += F(" MB</dd>");
  page += F("<dt>PSRAM Used/Total</dt><dd>");
  page += (ESP.getPsramSize() - ESP.getFreePsram()) / 1024;
  page += F("/");
  page += ESP.getPsramSize() / 1024;
  page += F(" kB</dd>");
  nvs_stats_t nvs_stats;
  if (nvs_get_stats("nvs", &nvs_stats) == ESP_OK) {
    page += F("<dt>NVS Used/Total</dt><dd>");
    page += nvs_stats.used_entries;
    page += F("/");
    page += nvs_stats.total_entries;
    page += F(" entries</dd>");
  }
  page += F("<dt>SPIFFS Used/Total</dt><dd>");
  page += SPIFFS.usedBytes() / 1024;
  page += F("/");
  page += SPIFFS.totalBytes() / 1024;
  page += F(" kB</dd>");
  page += F("<dt>Free Heap Size (Max Allocation)</dt><dd>");
  page += freeMemory / 1024;
  page += F(" kB (");
  page += maxAllocation / 1024;
  page += F(" kB)</dd>");
#ifdef BATTERY
#ifdef ARDUINO_BPI_LEAF_S3
  page += F("<dt>Battery Voltage</dt><dd>");
  page += String(batteryVoltage / 1000.0, 1);
  page += F(" V ");
  page += F("</dd>");
#else
  page += F("<dt>Input Voltage</dt><dd>");
  page += String(batteryVoltage / 1000.0, 1);
  page += F(" V ");
  page += batteryVoltage > 4300 ? F("plugged") : F("on battery");
  page += F("</dd>");
#endif // ARDUINO_BPI_LEAF_S3
#endif // BATTERY
  page += F("<dt>Uptime</dt><dd>");
  unsigned long sec = (millis() / 1000) % 60;
  unsigned long min = (millis() / 1000 / 60) % 60;
  unsigned long h   = (millis() / 1000 / 60 / 60) % 24;
  unsigned long day = (millis() / 1000 / 60 / 60 / 24);
  if (day > 0) { page += day; page += F("d "); }
  if (h   > 0) { page += h;   page += F("h "); }
  if (min > 0) { page += min; page += F("m "); }
  page += sec;
  page += F("s");
  page += F("</dd>");
  page += F("<dt>Webserver Running On Core</dt><dd>");
  page += xPortGetCoreID();
  page += F("</dd>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col'>");
  if (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA) {
  page += F("<h3>Wireless STA</h3>");
  page += F("<dt>SSID</dt><dd>");
  page += wifiSSID;
  page += F("</dd>");
  page += F("<dt>BSSID</dt><dd>");
  page += WiFi.BSSIDstr();
  page += F("</dd>");
  page += F("<dt>RSSI</dt><dd>");
  page += WiFi.RSSI();
  page += F(" dBm</dd>");
  page += F("<dt>Channel</dt><dd>");
  page += WiFi.channel();
  page += F("</dd>");
  page += F("<dt>Station MAC</dt><dd>");
  page += WiFi.macAddress();
  page += F("</dd>");
  }

  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
  page += F("<h3>Wireless AP</h3>");
  page += F("<dt>AP SSID</dt><dd>");
  page += ssidSoftAP;
  page += F("</dd>");
  page += F("<dt>AP MAC Address</dt><dd>");
  page += WiFi.softAPmacAddress();
  page += F("</dd>");
  page += F("<dt>AP IP Address</dt><dd>");
  page += WiFi.softAPIP().toString();
  page += F("</dd>");
  page += F("<dt>Channel</dt><dd>");
  page += WiFi.channel();
  page += F("</dd>");
  page += F("<dt>Hostname</dt><dd>");
  page += WiFi.softAPgetHostname();
  page += F("</dd>");
  page += F("<dt>Connected Stations</dt><dd>");
  page += WiFi.softAPgetStationNum();
  page += F("</dd>");
  }
  page += F("</div>");

  page += F("<div class='col'>");
  page += F("<h3>Network</h3>");
  page += F("<dt>Hostname</dt><dd>");
  page += WiFi.getHostname();
  page += F(".local");
  page += F("</dd>");
  page += F("<dt>IP address</dt><dd>");
  page += WiFi.localIP().toString();
  page += F("</dd>");
  page += F("<dt>Subnet mask</dt><dd>");
  page += WiFi.subnetMask().toString();
  page += F("</dd>");
  page += F("<dt>Gataway IP</dt><dd>");
  page += WiFi.gatewayIP().toString();
  page += F("</dd>");
  page += F("<dt>DNS 1</dt><dd>");
  page += WiFi.dnsIP(0).toString();
  page += F("</dd>");
  page += F("<dt>DNS 2</dt><dd>");
  page += WiFi.dnsIP(1).toString();
  page += F("</dd>");
  page += F("<dt>MIDI Network</dt><dd>");
  if (appleMidiConnected) {
    page += F("Connected to<br>");
    page += appleMidiSessionName;
  }
  else page += F("Disconnected");
  page += F("</dd>");
#ifdef BLE
  page += F("<dt>Bluetooth LE MIDI</dt><dd>");
  if (bleMidiConnected) page += F("Connected");
  else page += F("Disconnected");
  page += F("</dd>");
#endif
  page += F("</div>");

  page += F("</div>");

  get_footer_page();

  if (trim_page(start, len, true)) return;
}

void get_live_page(unsigned int start, unsigned int len) {

  if (get_top_page(1, start, len)) return;

  page += F("<div aria-live='polite' aria-atomic='true' style='position: relative;'>"
            "<div id='remotedisplay' class='toast' style='position: absolute; top: 0; right: 0; max-width: 600px;' data-autohide='false'>"
            "<div class='toast-header'>"
            "<strong class='me-auto'>Remote Display</strong>"
            "<small class='text-muted'>128x64</small>"
            "<button type='button' class='btn-close' data-bs-dismiss='toast' aria-label='Close'></button>"
            "</div>"
            "<div class='toast-body'>"
            "<canvas id='screen' height='64' width='128'>"
            "Sorry, your browser does not support canvas."
            "</canvas><br><small>"
            "<a id='zoom1' href='#' role='button'>1x</a> "
            "<a id='zoom2' href='#' role='button'>2x</a> "
            "<a id='zoom4' href='#' role='button'>4x</a> "
            "<a id='invert' href='#' role='button'>Invert</a></small>"
            "</div>"
            "</div>"
            "</div>"

            "<div id='live'>"
            "<a id='showremotedisplay' href='#' role='button'>Remote Display</a>"
            "<p></p>"
            "<small>Bank</small><br>"
            "<div class='btn-group btn-group-toggle' data-toggle='buttons'>");

  if (trim_page(start, len)) return;

  for (unsigned int i = 1; i < BANKS; i++) {
    page += F("<div class='col text-center g-0'>");
    page += F("<div class='grid-square'>");
    page += F("<button type='button' class='btn btn-outline-primary btn-sm' id='bank");
    page += i;
    page += F("'>");
    if (String(banknames[i]).isEmpty())
      page += i;
    else
      page += banknames[i];
    page += F("</button>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>"
            "<p></p>"

            "<div class='btn-group'>"
            "<button type='button' class='btn btn-primary dropdown-toggle' data-bs-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>"
            "MIDI Clock</button>"
            "<ul class='dropdown-menu'>"
            "<li><a id='clock-master' class='dropdown-item' href='#'>Master</a></li>"
            "<li><a id='clock-slave'  class='dropdown-item' href='#'>Slave</a></li>"
            "</ul>"
            "</div>"

            "<div class='btn-group'>"
            "<button type='button' class='btn btn-outline-primary dropdown-toggle' data-bs-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>"
            "Time Signature</button>"
            "<ul class='dropdown-menu'>"
            "<li><a id='4_4' class='dropdown-item' href='#'>4/4 Common Time</a></li>"
            "<li><a id='3_4' class='dropdown-item' href='#'>3/4 Waltz Time</a></li>"
            "<li><a id='2_4' class='dropdown-item' href='#'>2/4 March Time</a></li>"
            "<li><a id='3_8' class='dropdown-item' href='#'>3/8</a></li>"
            "<li><a id='6_8' class='dropdown-item' href='#'>6/8</a></li>"
            "<li><a id='9_8' class='dropdown-item' href='#'>9/8</a></li>"
            "<li><a id='12_8' class='dropdown-item' href='#'>12/8</a></li>"
            "</ul>"
            "</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='btn-group'>"
            "<button type='button' class='btn btn-primary dropdown-toggle' data-bs-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>"
            "MTC</button>"
            "<ul class='dropdown-menu'>"
            "<li><a id='mtc-master' class='dropdown-item' href='#'>Master</a></li>"
            "<li><a id='mtc-slave' class='dropdown-item' href='#'>Slave</a></li>"
            "</ul>"
            "</div>"
            "<p></p>"

            "<div>"
            "<h1 id='bpm'></h1> bpm"
            "<h1 id='timesignature'></h1>"
            "<h1 id='beat'></h1>"
            "<h1 id='mtc'></h1>"
            "</div>"
            "<p></p>"

            "<button id='start' type='button' class='btn btn-outline-primary'>Start</button>"
            "<button id='stop' type='button' class='btn btn-outline-primary'>Stop</button>"
            "<button id='continue' type='button' class='btn btn-outline-primary'>Continue</button>"
            "<button id='tap' type='button' class='btn btn-outline-primary'>Tap</button>"
            "</div>");

  if (trim_page(start, len)) return;

  page += F("<script>"
            "var isplaying = 0;"
            "var invert = 0;"
            "var zoom = 1;"
            "var con;"
            "var source;"

            "function webSocketConnect() {"
            "con = new WebSocket('ws://' + location.hostname + ':80/ws');"
            "con.binaryType = 'arraybuffer';"
            "con.onopen = function () {"
            "console.log('WebSocket to Pedalino open');"
            //"$('#live').find('input, button, submit, textarea, select').removeAttr('disabled');"
            //"$('#live').find('a').removeClass('disablehyper').unbind('click');"
            "var preview = document.querySelectorAll('input, button, submit, textarea, select');"
            "for (var i = 0; i < preview.length;  i++) {preview[i].removeAttribute('disabled');};"
            //"document.querySelectorAll('a').forEach(function(number, index, array){array[index].classList.remove('disablehyper').removeEventListener('click', click);});"
            "};"
            "con.onerror = function (error) {"
            "console.log('WebSocket to Pedalino error ', error);"
            "};"
            "con.onmessage = function (e) {"
            "var data = e.data;"
            "var dv = new DataView(data);"
  //          "if (dv.buffer.byteLength != 1024) return;"
            "var canvas=document.getElementById('screen');"
            "var context=canvas.getContext('2d');"
            "var x=0; y=0;"
            "for (y=0; y<64; y++)"
            "  for (x=0; x<128; x++)"
            "    if ((dv.getUint8(x+Math.floor(y/8)*128) & (1<<(y&7))) == 0){"
            "      (invert == 0) ? context.clearRect(x*zoom,y*zoom,zoom,zoom) : context.fillRect(x*zoom,y*zoom,zoom,zoom);"
            "    } else {(invert == 0) ? context.fillRect(x*zoom,y*zoom,zoom,zoom) : context.clearRect(x*zoom,y*zoom,zoom,zoom);}"
            "};"
            "con.onclose = function () {"
            "console.log('WebSocket to Pedalino closed');"
            //"$('#live').find('input, button, submit, textarea, select').attr('disabled', 'disabled');"
            //"$('#live').find('a').addClass('disablehyper').click(function (e) { e.preventDefault(); });"
            "var preview = document.querySelectorAll('input, button, submit, textarea, select');"
            "for (var i = 0; i < preview.length;  i++) {preview[i].setAttribute('disabled', 'disabled');};"
            "document.querySelectorAll('a').forEach(function(number, index, array){array[index].classList.add('disablehyper').addEventListener(click, click);});"
            "};"
            "setInterval(keepAliveConnection, 1000);"
            "};");

  if (trim_page(start, len)) return;

  page += F("function click(e) { e.preventDefault(); };"

            "function keepAliveConnection() {"
            "if (con.readyState == WebSocket.CLOSED) webSocketConnect();"
            "if (source.readyState == EventSource.CLOSED) eventSourceConnect();"
            "};"

            "webSocketConnect();"

            "function eventSourceConnect() {"
            "if (!!window.EventSource) {"
            "source = new EventSource('/events');"
            "source.addEventListener('open', function(e) {"
            "console.log('Events Connected');"
            "}, false);"
            "source.addEventListener('error', function(e) {"
            "if (e.target.readyState != EventSource.OPEN) {"
            "console.log('Events Disconnected');"
            "}"
            "}, false);"
            "source.addEventListener('message', function(e) {"
            "console.log('Event: ', e.data);"
            "}, false);"
            "source.addEventListener('play', function(e) { isplaying = e.data; }, false);"
            "source.addEventListener('timesignature', function(e) {"
            "document.getElementById('timesignature').innerHTML = e.data;"
            "}, false);"
            "source.addEventListener('bpm', function(e) {"
            "document.getElementById('bpm').innerHTML = e.data;"
            "}, false);"
            "source.addEventListener('beat', function(e) {"
            "document.getElementById('beat').innerHTML = e.data;"
            "}, false);"
            "source.addEventListener('mtc', function(e) {"
            "document.getElementById('mtc').innerHTML = e.data;"
	          "}, false);"
            "source.addEventListener('screen', function(e) {"
            "}, false);"
            "}"
            "}"

            "eventSourceConnect();");

  if (trim_page(start, len)) return;

  page += F("function sendBinary(str) {"
            "if (con.readyState != WebSocket.OPEN || con.bufferedAmount > 0) return;"
            "var buffer = new ArrayBuffer(str.length+1);"
            "var view = new DataView(buffer);"
            "for (i=0; i<str.length; i++)"
            "  view.setUint8(i, str.charCodeAt(i));"
            "view.setUint8(str.length, 0);"
            "con.send(view);"
            "}"

            "document.getElementById('showremotedisplay').onclick = function() {"
            "const toast = new bootstrap.Toast(document.getElementById('remotedisplay'));"
            "toast.show();"
            "setInterval(requestRemoteDisplay, 1000);"
            "return false; };"

            "function requestRemoteDisplay() {sendBinary('.');}"

            "function resizeScreen(z) {"
            "zoom = z;"
            "var canvas=document.getElementById('screen');"
            "var context=canvas.getContext('2d');"
            "context.canvas.width = 128*zoom;"
            "context.canvas.height = 64*zoom;"
            "};");

  if (trim_page(start, len)) return;

  for (unsigned int i = 1; i < BANKS; i++) {
    page += F("document.getElementById('bank");
    page += i;
    page += F("').onclick = function() {"
              "sendBinary('bank");
    page += i;
    page += F("');"
              "return false; };");

    if (trim_page(start, len)) return;
  }

  page += F("document.getElementById('invert').onclick = function() {"
            "if (invert == 0 ) invert = 1; else invert = 0; return false; };"
            "document.getElementById('zoom1').onclick = function() { resizeScreen(1); return false; };"
            "document.getElementById('zoom2').onclick = function() { resizeScreen(2); return false; };"
            "document.getElementById('zoom4').onclick = function() { resizeScreen(4); return false; };"

            "document.getElementById('clock-master').onclick = function() {"
            "sendBinary('clock-master');"
            "return false; };"
            "document.getElementById('clock-slave').onclick = function() {"
            "sendBinary('clock-slave');"
            "return false; };"
            "document.getElementById('mtc-master').onclick = function() {"
            "sendBinary('mtc-master');"
            "return false; };"
            "document.getElementById('mtc-slave').onclick = function() {"
            "sendBinary('mtc-slave');"
            "return false; };"

            "document.getElementById('4_4').onclick = function() {"
            "sendBinary('4/4');"
            "return false; };"
            "document.getElementById('3_4').onclick = function() {"
            "sendBinary('3/4');"
            "return false; };"
            "document.getElementById('2_4').onclick = function() {"
            "sendBinary('2/4');"
            "return false; };"
            "document.getElementById('3_8').onclick = function() {"
            "sendBinary('3/8');"
            "return false; };"
            "document.getElementById('6_8').onclick = function() {"
            "sendBinary('6/8');"
            "return false; };"
            "document.getElementById('9_8').onclick = function() {"
            "sendBinary('9/8');"
            "return false; };"
            "document.getElementById('12_8').onclick = function() {"
            "sendBinary('12/8');"
            "return false; };"

            "document.getElementById('start').onclick = function() {"
            "sendBinary('start');"
            "return false; };"
            "document.getElementById('stop').onclick = function() {"
            "sendBinary('stop');"
            "return false; };"
            "document.getElementById('continue').onclick = function() {"
            "sendBinary('continue');"
            "return false; };"
            "document.getElementById('tap').onclick = function() {"
            "sendBinary('tap');"
            "return false; };"

            "</script>");

  get_footer_page();

  if (trim_page(start, len, true)) return;
}

void get_actions_page(unsigned int start, unsigned int len) {

  const byte   b = constrain(uibank.toInt(), 0, BANKS - 1);
  action      *act;
  unsigned int i;
  bool         same_control;

  if (get_top_page(2, start, len)) return;

  page += F("<div class='card h-100 mb-3'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-book' viewBox='0 0 20 20'>");
  page += F("<path d='M1 2.828c.885-.37 2.154-.769 3.388-.893 1.33-.134 2.458.063 3.112.752v9.746c-.935-.53-2.12-.603-3.213-.493-1.18.12-2.37.461-3.287.811V2.828zm7.5-.141c.654-.689 1.782-.886 3.112-.752 1.234.124 2.503.523 3.388.893v9.923c-.918-.35-2.107-.692-3.287-.81-1.094-.111-2.278-.039-3.213.492V2.687zM8 1.783C7.015.936 5.587.81 4.287.94c-1.514.153-3.042.672-3.994 1.105A.5.5 0 0 0 0 2.5v11a.5.5 0 0 0 .707.455c.882-.4 2.303-.881 3.68-1.02 1.409-.142 2.59.087 3.223.877a.5.5 0 0 0 .78 0c.633-.79 1.814-1.019 3.222-.877 1.378.139 2.8.62 3.681 1.02A.5.5 0 0 0 16 13.5v-11a.5.5 0 0 0-.293-.455c-.952-.433-2.48-.952-3.994-1.105C10.413.809 8.985.936 8 1.783z'/>");
  page += F("</svg>");
  page += F(" Bank ");
  page += uibank;
  page += F("</h5>");
  page += F("<div class='card-body'>");
  page += F("<form method='get' id='banks-form'>");
  page += F("<input type='hidden' id='banksorder' name='banksorder' value=''/>");
  page += F("<div class='container g-0'>");
  page += F("<div id='banklist' class='row g-0'>");

  if (trim_page(start, len)) return;

  for (i = 0; i < BANKS; i++) {
    page += F("<div class='col text-center g-0'>");
    page += F("<button type='button submit' class='btn btn-sm btn-block");
    page += (uibank == String(i) ? F(" btn-primary") : F(""));
    page += F("' id='");
    page += i;
    page += F("' name='bank' value='");
    page += i;
    page += F("'>");
    if (String(banknames[i]).isEmpty())
      page += i;
    else
      page += banknames[i];
    page += F("</button>");
    page += F("</div>");
    //if ((i + 1) % (BANKS / 3) == 0) page += F("<div class='w-100'></div>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");
  page += F("</form>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<script>");
  page += F("var el = document.getElementById('banklist');"
            "var sortable = Sortable.create(el);"

            "let banksForm = document.getElementById('banks-form');"
            "banksForm.addEventListener('submit', function (e) {"
              // get ids for all currently bank list items
              "let bank_ids = document.querySelectorAll('#banklist  button');"
              "let bank_ids_list = [];"
              "for (let i = 0; i < bank_ids.length; i++) {"
                "bank_ids_list.push(bank_ids[i].id);"
              "}"
              // fill in the hidden input
              "let banks_input = document.getElementById('banksorder');"
              "banks_input.value = bank_ids_list.join(',');"
            "});");
  page += F("</script>");

  if (trim_page(start, len)) return;

  page += F("<div class='card h-100 mb-3'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-toggles' viewBox='0 0 20 20'>");
  page += F("<path d='M4.5 9a3.5 3.5 0 1 0 0 7h7a3.5 3.5 0 1 0 0-7h-7zm7 6a2.5 2.5 0 1 1 0-5 2.5 2.5 0 0 1 0 5zm-7-14a2.5 2.5 0 1 0 0 5 2.5 2.5 0 0 0 0-5zm2.45 0A3.49 3.49 0 0 1 8 3.5 3.49 3.49 0 0 1 6.95 6h4.55a2.5 2.5 0 0 0 0-5H6.95zM4.5 0h7a3.5 3.5 0 1 1 0 7h-7a3.5 3.5 0 1 1 0-7z'/>");
  page += F("</svg>");
  page += F(" Control ");
  page += uicontrol;
  page += F("</h5>");
  page += F("<div class='card-body'>");
  page += F("<form method='get'>");
  page += F("<div class='container g-0'>");
  page += F("<div class='row'>");
  page += F("<div class='col text-center'>");
  page += F("<button type='button submit' class='btn btn-sm");
  page += (uicontrol.equals("All") ? F(" btn-primary") : F(""));
  page += F("' name='control' value='All'>All</button>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  for (unsigned int c = 1; c <= CONTROLS; c++) {
    if (control_not_defined(c-1)) continue;
    page += F("<div class='col text-center'>");
    page += F("<button type='button submit' class='btn btn-sm");
    page += (uicontrol == String(c) ? F(" btn-primary") : F(""));
    page += F("' name='control' value='");
    page += c;
    page += F("'>");
    page += c;
    page += F("</button>");
    page += F("</div>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");
  page += F("</form>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<form method='post'>");

  page += F("<div class='card mb-3'>");
  page += F("<div class='card-header'>");
  page += F("<div class='row'>");
  page += F("<div class='col-auto me-auto'>");
  page += F("<h5 class='mb-0'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-clipboard-check' viewBox='0 0 20 20'>");
  page += F("<path fill-rule='evenodd' d='M10.854 7.146a.5.5 0 0 1 0 .708l-3 3a.5.5 0 0 1-.708 0l-1.5-1.5a.5.5 0 1 1 .708-.708L7.5 9.793l2.646-2.647a.5.5 0 0 1 .708 0z'/>");
  page += F("<path d='M4 1.5H3a2 2 0 0 0-2 2V14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V3.5a2 2 0 0 0-2-2h-1v1h1a1 1 0 0 1 1 1V14a1 1 0 0 1-1 1H3a1 1 0 0 1-1-1V3.5a1 1 0 0 1 1-1h1v-1z'/>");
  page += F("<path d='M9.5 1a.5.5 0 0 1 .5.5v1a.5.5 0 0 1-.5.5h-3a.5.5 0 0 1-.5-.5v-1a.5.5 0 0 1 .5-.5h3zm-3-1A1.5 1.5 0 0 0 5 1.5v1A1.5 1.5 0 0 0 6.5 4h3A1.5 1.5 0 0 0 11 2.5v-1A1.5 1.5 0 0 0 9.5 0h-3z'/>");
  page += F("</svg>");
  page += F(" Actions");
  page += F("</h5>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-auto'>");
  page += F("<div class='btn-group' role='group'>");
  page += F("<button id='btnGroupNewAction' type='button' class='btn btn-primary btn-sm dropdown-toggle' data-bs-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-plus-circle-dotted' viewBox='0 0 16 16'>");
  page += F("<path d='M8 0c-.176 0-.35.006-.523.017l.064.998a7.117 7.117 0 0 1 .918 0l.064-.998A8.113 8.113 0 0 0 8 0zM6.44.152c-.346.069-.684.16-1.012.27l.321.948c.287-.098.582-.177.884-.237L6.44.153zm4.132.271a7.946 7.946 0 0 0-1.011-.27l-.194.98c.302.06.597.14.884.237l.321-.947zm1.873.925a8 8 0 0 0-.906-.524l-.443.896c.275.136.54.29.793.459l.556-.831zM4.46.824c-.314.155-.616.33-.905.524l.556.83a7.07 7.07 0 0 1 .793-.458L4.46.824zM2.725 1.985c-.262.23-.51.478-.74.74l.752.66c.202-.23.418-.446.648-.648l-.66-.752zm11.29.74a8.058 8.058 0 0 0-.74-.74l-.66.752c.23.202.447.418.648.648l.752-.66zm1.161 1.735a7.98 7.98 0 0 0-.524-.905l-.83.556c.169.253.322.518.458.793l.896-.443zM1.348 3.555c-.194.289-.37.591-.524.906l.896.443c.136-.275.29-.54.459-.793l-.831-.556zM.423 5.428a7.945 7.945 0 0 0-.27 1.011l.98.194c.06-.302.14-.597.237-.884l-.947-.321zM15.848 6.44a7.943 7.943 0 0 0-.27-1.012l-.948.321c.098.287.177.582.237.884l.98-.194zM.017 7.477a8.113 8.113 0 0 0 0 1.046l.998-.064a7.117 7.117 0 0 1 0-.918l-.998-.064zM16 8a8.1 8.1 0 0 0-.017-.523l-.998.064a7.11 7.11 0 0 1 0 .918l.998.064A8.1 8.1 0 0 0 16 8zM.152 9.56c.069.346.16.684.27 1.012l.948-.321a6.944 6.944 0 0 1-.237-.884l-.98.194zm15.425 1.012c.112-.328.202-.666.27-1.011l-.98-.194c-.06.302-.14.597-.237.884l.947.321zM.824 11.54a8 8 0 0 0 .524.905l.83-.556a6.999 6.999 0 0 1-.458-.793l-.896.443zm13.828.905c.194-.289.37-.591.524-.906l-.896-.443c-.136.275-.29.54-.459.793l.831.556zm-12.667.83c.23.262.478.51.74.74l.66-.752a7.047 7.047 0 0 1-.648-.648l-.752.66zm11.29.74c.262-.23.51-.478.74-.74l-.752-.66c-.201.23-.418.447-.648.648l.66.752zm-1.735 1.161c.314-.155.616-.33.905-.524l-.556-.83a7.07 7.07 0 0 1-.793.458l.443.896zm-7.985-.524c.289.194.591.37.906.524l.443-.896a6.998 6.998 0 0 1-.793-.459l-.556.831zm1.873.925c.328.112.666.202 1.011.27l.194-.98a6.953 6.953 0 0 1-.884-.237l-.321.947zm4.132.271a7.944 7.944 0 0 0 1.012-.27l-.321-.948a6.954 6.954 0 0 1-.884.237l.194.98zm-2.083.135a8.1 8.1 0 0 0 1.046 0l-.064-.998a7.11 7.11 0 0 1-.918 0l-.064.998zM8.5 4.5a.5.5 0 0 0-1 0v3h-3a.5.5 0 0 0 0 1h3v3a.5.5 0 0 0 1 0v-3h3a.5.5 0 0 0 0-1h-3v-3z'/>");
  page += F("</svg>");
  page += F(" New Action</button>");
  page += F("<div class='dropdown-menu' aria-labelledby='btnGroupNewAction'>");
  for (unsigned int c = 1; c <= CONTROLS; c++) {
    if (control_not_defined(c-1)) continue;
    page += F("<button type='submit' class='dropdown-item' name='action' value='new");
    page += c;
    page += F("'>Control ");
    page += c;
    page += F("</button>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");
  page += F(" ");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F(" Apply</button>");
  page += F(" ");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='card-body'>");
  page += F("<div class='row'>");
  page += F("<div class='col-auto me-auto'>");
  page += F("<div class='form-floating'>");
  page += F("<input type='text' class='form-control' id='bankNameInput' name='bankname' maxlength='");
  page += MAXBANKNAME;
  page += F("' value='");
  page += banknames[b];
  page += F("'>");
  page += F("<label for='bankNameInput'>Bank Name</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<div class='col-auto'>");
  page += F("<div class='btn-group' role='group'>");
  page += F("<button id='btnGroupCopyBank' type='button' class='btn btn-primary btn-sm dropdown-toggle' data-bs-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-layers' viewBox='0 0 16 16'>");
  page += F("<path d='M8.235 1.559a.5.5 0 0 0-.47 0l-7.5 4a.5.5 0 0 0 0 .882L3.188 8 .264 9.559a.5.5 0 0 0 0 .882l7.5 4a.5.5 0 0 0 .47 0l7.5-4a.5.5 0 0 0 0-.882L12.813 8l2.922-1.559a.5.5 0 0 0 0-.882l-7.5-4zm3.515 7.008L14.438 10 8 13.433 1.562 10 4.25 8.567l3.515 1.874a.5.5 0 0 0 .47 0l3.515-1.874zM8 9.433 1.562 6 8 2.567 14.438 6 8 9.433z'/>");
  page += F("</svg>");
  page += F(" Duplicate Bank To</button>");
  page += F("<div class='dropdown-menu' aria-labelledby='btnGroupCopyBank'>");

  if (trim_page(start, len)) return;

  for (i = 0; i < BANKS; i++) {
    page += F("<button type='submit' class='dropdown-item' name='action' value='copy");
    page += i;
    page += F("'>");
    (banknames[i][0] == 0) ? page += i : page += banknames[i];
    page += F("</button>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  /*
  page += F("<div class='col-auto'>");
  page += F("<div class='btn-group' role='group'>");
  page += F("<button id='btnGroupNewAction' type='button' class='btn btn-primary btn-sm dropdown-toggle' data-bs-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-plus-circle-dotted' viewBox='0 0 16 16'>");
  page += F("<path d='M8 0c-.176 0-.35.006-.523.017l.064.998a7.117 7.117 0 0 1 .918 0l.064-.998A8.113 8.113 0 0 0 8 0zM6.44.152c-.346.069-.684.16-1.012.27l.321.948c.287-.098.582-.177.884-.237L6.44.153zm4.132.271a7.946 7.946 0 0 0-1.011-.27l-.194.98c.302.06.597.14.884.237l.321-.947zm1.873.925a8 8 0 0 0-.906-.524l-.443.896c.275.136.54.29.793.459l.556-.831zM4.46.824c-.314.155-.616.33-.905.524l.556.83a7.07 7.07 0 0 1 .793-.458L4.46.824zM2.725 1.985c-.262.23-.51.478-.74.74l.752.66c.202-.23.418-.446.648-.648l-.66-.752zm11.29.74a8.058 8.058 0 0 0-.74-.74l-.66.752c.23.202.447.418.648.648l.752-.66zm1.161 1.735a7.98 7.98 0 0 0-.524-.905l-.83.556c.169.253.322.518.458.793l.896-.443zM1.348 3.555c-.194.289-.37.591-.524.906l.896.443c.136-.275.29-.54.459-.793l-.831-.556zM.423 5.428a7.945 7.945 0 0 0-.27 1.011l.98.194c.06-.302.14-.597.237-.884l-.947-.321zM15.848 6.44a7.943 7.943 0 0 0-.27-1.012l-.948.321c.098.287.177.582.237.884l.98-.194zM.017 7.477a8.113 8.113 0 0 0 0 1.046l.998-.064a7.117 7.117 0 0 1 0-.918l-.998-.064zM16 8a8.1 8.1 0 0 0-.017-.523l-.998.064a7.11 7.11 0 0 1 0 .918l.998.064A8.1 8.1 0 0 0 16 8zM.152 9.56c.069.346.16.684.27 1.012l.948-.321a6.944 6.944 0 0 1-.237-.884l-.98.194zm15.425 1.012c.112-.328.202-.666.27-1.011l-.98-.194c-.06.302-.14.597-.237.884l.947.321zM.824 11.54a8 8 0 0 0 .524.905l.83-.556a6.999 6.999 0 0 1-.458-.793l-.896.443zm13.828.905c.194-.289.37-.591.524-.906l-.896-.443c-.136.275-.29.54-.459.793l.831.556zm-12.667.83c.23.262.478.51.74.74l.66-.752a7.047 7.047 0 0 1-.648-.648l-.752.66zm11.29.74c.262-.23.51-.478.74-.74l-.752-.66c-.201.23-.418.447-.648.648l.66.752zm-1.735 1.161c.314-.155.616-.33.905-.524l-.556-.83a7.07 7.07 0 0 1-.793.458l.443.896zm-7.985-.524c.289.194.591.37.906.524l.443-.896a6.998 6.998 0 0 1-.793-.459l-.556.831zm1.873.925c.328.112.666.202 1.011.27l.194-.98a6.953 6.953 0 0 1-.884-.237l-.321.947zm4.132.271a7.944 7.944 0 0 0 1.012-.27l-.321-.948a6.954 6.954 0 0 1-.884.237l.194.98zm-2.083.135a8.1 8.1 0 0 0 1.046 0l-.064-.998a7.11 7.11 0 0 1-.918 0l-.064.998zM8.5 4.5a.5.5 0 0 0-1 0v3h-3a.5.5 0 0 0 0 1h3v3a.5.5 0 0 0 1 0v-3h3a.5.5 0 0 0 0-1h-3v-3z'/>");
  page += F("</svg>");
  page += F(" New Action</button>");
  page += F("<div class='dropdown-menu' aria-labelledby='btnGroupNewAction'>");
  for (i = 1; i <= PEDALS; i++) {
    page += F("<button type='submit' class='dropdown-item' name='action' value='new");
    page += i;
    page += F("'>Pedal ");
    page += i;
    page += F("</button>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  */
  page += F("</div>");

  i = 1;
  act = actions[b];
  same_control = false;
  while (act != nullptr) {

    if (trim_page(start, len)) return;

    if (uicontrol != String(act->control + 1) && !(uicontrol.equals("All"))) {
      act = act->next;
      continue;
    }

    if (!same_control) {
      page += F("<div class='card mt-3'>");
      page += F("<div class='card-body'>");
      page += F("<h5 class='card-title'>");
      page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-toggles' viewBox='0 0 20 20'>");
      page += F("<path d='M4.5 9a3.5 3.5 0 1 0 0 7h7a3.5 3.5 0 1 0 0-7h-7zm7 6a2.5 2.5 0 1 1 0-5 2.5 2.5 0 0 1 0 5zm-7-14a2.5 2.5 0 1 0 0 5 2.5 2.5 0 0 0 0-5zm2.45 0A3.49 3.49 0 0 1 8 3.5 3.49 3.49 0 0 1 6.95 6h4.55a2.5 2.5 0 0 0 0-5H6.95zM4.5 0h7a3.5 3.5 0 1 1 0 7h-7a3.5 3.5 0 1 1 0-7z'/>");
      page += F("</svg>");
      page += F(" Control ");
      page += act->control + 1;
      page += F("</h5>");
      page += F("<div class='container'>");
      page += F("<div class='row row-cols-1 row-cols-sm-1 row-cols-md-2 row-cols-xl-3 g-2 g-md-3 g-xl-4'>");
    }
    page += F("<div class='col'>");
    page += F("<div class='d-grid gap-1'>");

    if (trim_page(start, len)) return;

    page += "<input type='hidden' id='pedalMode";
    page += i;
    page += F("' name='pedalMode");
    page += i;
    page +=F("' value='");
    switch (pedals[controls[act->control].pedal1].mode) {
      case PED_MOMENTARY1:
      case PED_MOMENTARY2:
      case PED_MOMENTARY3:
        page +=F("momentary");
        break;

      case PED_LATCH1:
      case PED_LATCH2:
        page +=F("latch");
        break;

      case PED_LADDER:
        page +=F("ladder");
        break;

      case PED_ANALOG:
        page +=F("analog");
        break;

      case PED_ANALOG_PAD:
        page +=F("analogpad");
        break;

      case PED_JOG_WHEEL:
        page +=F("jogwheel");
        break;

      case PED_ULTRASONIC:
        page +=F("ultrasonic");
        break;

      case PED_ANALOG_MOMENTARY:
        page +=F("analogmomentary");
        break;

      case PED_ANALOG_LATCH:
        page +=F("analoglatch");
        break;
    }
    page +=F("'>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    page += F("<div class='w-50'>");
    page += F("<div class='form-floating'>");
    page += F("<select class='form-select' id='onSelect");
    page += i;
    page += F("' onchange='init_action(");
    page += i;
    page += F(")' name='event");
    page += i;
    page += F("'>");
    page += F("<option value='");
    page += PED_EVENT_NONE;
    page += F("'");
    if (act->event == PED_EVENT_NONE) page += F(" selected");
    page += F(">");
    page += F("</option>");
    switch (pedals[controls[act->control].pedal1].mode) {
      case PED_MOMENTARY1:
      case PED_MOMENTARY2:
      case PED_MOMENTARY3:
      case PED_LATCH1:
      case PED_LATCH2:
      case PED_LADDER:
      case PED_ANALOG_MOMENTARY:
      case PED_ANALOG_LATCH:
        if (pedals[controls[act->control].pedal1].mode == PED_ANALOG_MOMENTARY ||
            pedals[controls[act->control].pedal1].mode == PED_ANALOG_LATCH) {
          page += F("<option value='");
          page += PED_EVENT_MOVE;
          page += F("'");
          if (act->event == PED_EVENT_MOVE) page += F(" selected");
          page += F(">");
          page += F("Move</option>");
        }
        page += F("<option value='");
        page += PED_EVENT_PRESS;
        page += F("'");
        if (act->event == PED_EVENT_PRESS) page += F(" selected");
        page += F(">");
        page += F("Press</option>");
        page += F("<option value='");
        page += PED_EVENT_RELEASE;
        page += F("'");
        if (act->event == PED_EVENT_RELEASE) page += F(" selected");
        page += F(">");
        page += F("Release</option>");
        page += F("<option value='");
        page += PED_EVENT_PRESS_RELEASE;
        page += F("'");
        if (act->event == PED_EVENT_PRESS_RELEASE) page += F(" selected");
        page += F(">");
        page += F("Press & Release</option>");
        page += F("<option value='");
        page += PED_EVENT_CLICK;
        page += F("'");
        if (act->event == PED_EVENT_CLICK) page += F(" selected");
        page += F(">");
        page += F("Click</option>");
        page += F("<option value='");
        page += PED_EVENT_DOUBLE_CLICK;
        page += F("'");
        if (act->event == PED_EVENT_DOUBLE_CLICK) page += F(" selected");
        page += F(">");
        page += F("Double Click</option>");
        page += F("<option value='");
        page += PED_EVENT_LONG_PRESS;
        page += F("'");
        if (act->event == PED_EVENT_LONG_PRESS) page += F(" selected");
        page += F(">");
        page += F("Long Press</option>");
        page += F("<option value='");
        page += PED_EVENT_REPEAT;
        page += F("'");
        if (act->event == PED_EVENT_REPEAT) page += F(" selected");
        page += F(">");
        page += F("Repeat Pressed</option>");
        page += F("<option value='");
        page += PED_EVENT_LONG_RELEASED;
        page += F("'");
        if (act->event == PED_EVENT_LONG_RELEASED) page += F(" selected");
        page += F(">");
        page += F("Long Released</option>");
        break;

      case PED_ANALOG:
      case PED_ANALOG4:
      case PED_ULTRASONIC:
        page += F("<option value='");
        page += PED_EVENT_MOVE;
        page += F("'");
        if (act->event == PED_EVENT_MOVE) page += F(" selected");
        page += F(">");
        page += F("Move</option>");
        break;

      case PED_ANALOG_PAD:
        page += F("<option value='");
        page += PED_EVENT_SHOT;
        page += F("'");
        if (act->event == PED_EVENT_SHOT) page += F(" selected");
        page += F(">");
        page += F("Shot</option>");
        break;

      case PED_JOG_WHEEL:
        page += F("<option value='");
        page += PED_EVENT_JOG;
        page += F("'");
        if (act->event == PED_EVENT_JOG) page += F(" selected");
        page += F(">");
        page += F("Jog</option>");
        break;
    }
    page += F("<option value='");
    page += PED_EVENT_OSC;
    page += F("'");
    if (act->event == PED_EVENT_OSC) page += F(" selected");
    page += F(">");
    page += F("OSC Message</option>");
    page += F("</select>");
    page += F("<label for='onSelect");
    page += i;
    page += F("'>On</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='w-25'>");
    page += F("<div class='form-floating'>");
    page += F("<select class='form-select' id='controlSelect' name='control");
    page += i;
    page += F("'");
    page += F(">");
    for (unsigned int c = 1; c <= CONTROLS; c++) {
      if (control_not_defined(c-1)) continue;
      page += F("<option value='");
      page += c;
      page += F("'");
      if (act->control == c - 1) page += F(" selected");
      page += F(">");
      page += c;
      page += F("</option>");

      if (trim_page(start, len)) return;
    }
    page += F("</select>");
    page += F("<label for='buttonSelect'>Control</label>");
    page += F("</div>");
    page += F("</div>");

    page += F("<div class='w-25'>");
    page += F("<button type='submit' name='action' value='delete");
    page += i;
    page += F("' class='btn btn-sm'>");
    page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-trash' viewBox='0 0 16 16'>");
    page += F("<path d='M5.5 5.5A.5.5 0 0 1 6 6v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5zm2.5 0a.5.5 0 0 1 .5.5v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5zm3 .5a.5.5 0 0 0-1 0v6a.5.5 0 0 0 1 0V6z'/>");
    page += F("<path fill-rule='evenodd' d='M14.5 3a1 1 0 0 1-1 1H13v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V4h-.5a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1H6a1 1 0 0 1 1-1h2a1 1 0 0 1 1 1h3.5a1 1 0 0 1 1 1v1zM4.118 4L4 4.059V13a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V4.059L11.882 4H4.118zM2.5 3V2h11v1h-11z'/>");
    page += F("</svg>");
    page += F("</button>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    page += F("<div class='w-50'>");
    page += F("<div class='form-floating'>");
    page += F("<select class='form-select' id='sendSelect");
    page += i;
    page += F("' onchange='init_action(");
    page += i;
    page += F(")' name='message");
    page += i;
    page += F("'>");
    page += F("<option value='");
    page += PED_EMPTY;
    page += F("'");
    if (act->midiMessage == PED_EMPTY) page += F(" selected");
    page += F("></option>");
    page += F("<option value='");
    page += PED_PROGRAM_CHANGE;
    page += F("'");
    if (act->midiMessage == PED_PROGRAM_CHANGE) page += F(" selected");
    page += F(">Program Change</option>");
    page += F("<option value='");
    page += PED_CONTROL_CHANGE;
    page += F("'");
    if (act->midiMessage == PED_CONTROL_CHANGE) page += F(" selected");
    page += F(">Control Change</option>");
    page += F("<option value='");
    page += PED_CONTROL_CHANGE_SNAP;
    page += F("'");
    if (act->midiMessage == PED_CONTROL_CHANGE_SNAP) page += F(" selected");
    page += F(">Control Change Snap</option>");
    page += F("<option value='");
    page += PED_NOTE_ON;
    page += F("'");
    if (act->midiMessage == PED_NOTE_ON) page += F(" selected");
    page += F(">Note On</option>");
    page += F("<option value='");
    page += PED_NOTE_OFF;
    page += F("'");
    if (act->midiMessage == PED_NOTE_OFF) page += F(" selected");
    page += F(">Note Off</option>");

    if (trim_page(start, len)) return;

    page += F("<option value='");
    page += PED_BANK_SELECT_INC;
    page += F("'");
    if (act->midiMessage == PED_BANK_SELECT_INC) page += F(" selected");
    page += F(">Bank Select+</option>");
    page += F("<option value='");
    page += PED_BANK_SELECT_DEC;
    page += F("'");
    if (act->midiMessage == PED_BANK_SELECT_DEC) page += F(" selected");
    page += F(">Bank Select-</option>");
    page += F("<option value='");
    page += PED_PROGRAM_CHANGE_INC;
    page += F("'");
    if (act->midiMessage == PED_PROGRAM_CHANGE_INC) page += F(" selected");
    page += F(">Program Change+</option>");
    page += F("<option value='");
    page += PED_PROGRAM_CHANGE_DEC;
    page += F("'");
    if (act->midiMessage == PED_PROGRAM_CHANGE_DEC) page += F(" selected");
    page += F(">Program Change-</option>");
    page += F("<option value='");
    page += PED_PITCH_BEND;
    page += F("'");
    if (act->midiMessage == PED_PITCH_BEND) page += F(" selected");
    page += F(">Pitch Bend</option>");
    page += F("<option value='");
    page += PED_CHANNEL_PRESSURE;
    page += F("'");
    if (act->midiMessage == PED_CHANNEL_PRESSURE) page += F(" selected");
    page += F(">Channel Pressure</option>");

    if (trim_page(start, len)) return;

    page += F("<option value='");
    page += PED_ACTION_MIDI_CLOCK_MASTER;
    page += F("'");
    if (act->midiMessage == PED_ACTION_MIDI_CLOCK_MASTER) page += F(" selected");
    page += F(">MIDI Clock Master</option>");
    page += F("<option value='");
    page += PED_ACTION_MIDI_CLOCK_SLAVE;
    page += F("'");
    if (act->midiMessage == PED_ACTION_MIDI_CLOCK_SLAVE) page += F(" selected");
    page += F(">MIDI Clock Slave</option>");
    page += F("<option value='");
    page += PED_ACTION_MIDI_CLOCK_OFF;
    page += F("'");
    if (act->midiMessage == PED_ACTION_MIDI_CLOCK_OFF) page += F(" selected");
    page += F(">MIDI Clock Off</option>");
    page += F("<option value='");
    page += PED_MIDI_START;
    page += F("'");
    if (act->midiMessage == PED_MIDI_START) page += F(" selected");
    page += F(">Start</option>");
    page += F("<option value='");
    page += PED_MIDI_STOP;
    page += F("'");
    if (act->midiMessage == PED_MIDI_STOP) page += F(" selected");
    page += F(">Stop</option>");
    page += F("<option value='");
    page += PED_MIDI_CONTINUE;
    page += F("'");
    if (act->midiMessage == PED_MIDI_CONTINUE) page += F(" selected");
    page += F(">Continue</option>");
    page += F("<option value='");
    page += PED_SEQUENCE;
    page += F("'");
    if (act->midiMessage == PED_SEQUENCE) page += F(" selected");
    page += F(">Sequence</option>");
    page += F("<option value='");
    page += PED_SEQUENCE_STEP_BY_STEP_FWD;
    page += F("'");
    if (act->midiMessage == PED_SEQUENCE_STEP_BY_STEP_FWD) page += F(" selected");
    page += F(">Step by Step+</option>");
    page += F("<option value='");
    page += PED_SEQUENCE_STEP_BY_STEP_REV;
    page += F("'");
    if (act->midiMessage == PED_SEQUENCE_STEP_BY_STEP_REV) page += F(" selected");
    page += F(">Step by Step-</option>");
    page += F("<option value='");
    page += PED_ACTION_BANK_PLUS;
    page += F("'");
    if (act->midiMessage == PED_ACTION_BANK_PLUS) page += F(" selected");
    page += F(">Bank+</option>");
    page += F("<option value='");
    page += PED_ACTION_BANK_MINUS;
    page += F("'");
    if (act->midiMessage == PED_ACTION_BANK_MINUS) page += F(" selected");
    page += F(">Bank-</option>");

    if (trim_page(start, len)) return;

    page += F("<option value='");
    page += PED_ACTION_MTC_MASTER;
    page += F("'");
    if (act->midiMessage == PED_ACTION_MTC_MASTER) page += F(" selected");
    page += F(">MTC Master</option>");
    page += F("<option value='");
    page += PED_ACTION_MTC_SLAVE;
    page += F("'");
    if (act->midiMessage == PED_ACTION_MTC_SLAVE) page += F(" selected");
    page += F(">MTC Slave</option>");
    page += F("<option value='");
    page += PED_ACTION_MTC_OFF;
    page += F("'");
    if (act->midiMessage == PED_ACTION_MTC_OFF) page += F(" selected");
    page += F(">MTC Off</option>");
    page += F("<option value='");
    page += PED_ACTION_MTC_TIME_SIGNATURE;
    page += F("'");
    if (act->midiMessage == PED_ACTION_MTC_TIME_SIGNATURE) page += F(" selected");
    page += F(">MTC Time Signature</option>");
    page += F("<option value='");
    page += PED_ACTION_START;
    page += F("'");
    if (act->midiMessage == PED_ACTION_START) page += F(" selected");
    page += F(">MTC Start</option>");
    page += F("<option value='");
    page += PED_ACTION_STOP;
    page += F("'");
    if (act->midiMessage == PED_ACTION_STOP) page += F(" selected");
    page += F(">MTC Stop</option>");
    page += F("<option value='");
    page += PED_ACTION_CONTINUE;
    page += F("'");
    if (act->midiMessage == PED_ACTION_CONTINUE) page += F(" selected");
    page += F(">MTC Continue</option>");
    page += F("<option value='");
    page += PED_ACTION_TAP;
    page += F("'");
    if (act->midiMessage == PED_ACTION_TAP) page += F(" selected");
    page += F(">Tap</option>");
    page += F("<option value='");
    page += PED_ACTION_BPM_PLUS;
    page += F("'");
    if (act->midiMessage == PED_ACTION_BPM_PLUS) page += F(" selected");
    page += F(">BPM+</option>");
    page += F("<option value='");
    page += PED_ACTION_BPM_MINUS;
    page += F("'");
    if (act->midiMessage == PED_ACTION_BPM_MINUS) page += F(" selected");
    page += F(">BPM-</option>");
    page += F("<option value='");
    page += PED_OSC_MESSAGE;
    page += F("'");
    if (act->midiMessage == PED_OSC_MESSAGE) page += F(" selected");
    page += F(">OSC Message</option>");

    if (trim_page(start, len)) return;

    page += F("<option value='");
    page += PED_ACTION_PROFILE_PLUS;
    page += F("'");
    if (act->midiMessage == PED_ACTION_PROFILE_PLUS) page += F(" selected");
    page += F(">Profile+</option>");
    page += F("<option value='");
    page += PED_ACTION_PROFILE_MINUS;
    page += F("'");
    if (act->midiMessage == PED_ACTION_PROFILE_MINUS) page += F(" selected");
    page += F(">Profile-</option>");
    page += F("<option value='");
    page += PED_ACTION_LED_COLOR;
    page += F("'");
    if (act->midiMessage == PED_ACTION_LED_COLOR) page += F(" selected");
    page += F(">Set Led Color</option>");
    page += F("<option value='");
    page += PED_ACTION_REPEAT;
    page += F("'");
    if (act->midiMessage == PED_ACTION_REPEAT) page += F(" selected");
    page += F(">Repeat</option>");
    page += F("<option value='");
    page += PED_ACTION_REPEAT_OVERWRITE;
    page += F("'");
    if (act->midiMessage == PED_ACTION_REPEAT_OVERWRITE) page += F(" selected");
    page += F(">Repeat Overwrite</option>");
    page += F("<option value='");
    page += PED_ACTION_SCAN;
    page += F("'");
    if (act->midiMessage == PED_ACTION_SCAN) page += F(" selected");
    page += F(">Scan</option>");
    page += F("<option value='");
    page += PED_ACTION_DEVICE_INFO;
    page += F("'");
    if (act->midiMessage == PED_ACTION_DEVICE_INFO) page += F(" selected");
    page += F(">Device Info</option>");
    page += F("<option value='");
    page += PED_ACTION_POWER_ON_OFF;
    page += F("'");
    if (act->midiMessage == PED_ACTION_POWER_ON_OFF) page += F(" selected");
    page += F(">Power On/Off</option>");
    page += F("</select>");
    page += F("<label for='sendSelect");
    page += i;
    page += F("'>Send</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='w-25'>");
    page += F("<div class='form-floating' hidden id='channelDiv");
    page += i;
    page += F("'>");
    page += F("<select class='form-select' id='channelSelect");
    page += i;
    page += F("' name='channel");
    page += i;
    page += F("'>");
    for (unsigned int c = 0; c <= 17; c++) {
      page += F("<option value='");
      page += c;
      page += F("'");
      if (act->midiChannel == c) page += F(" selected");
      page += F(">");
      switch (c) {
        case 0:
          page += "None";
          break;
        case 17:
          page += "All";
          break;
        default:  // 1..16
          page += c;
          break;
      }
      page += F("</option>");

      if (trim_page(start, len)) return;
    }
    page += F("</select>");
    page += F("<label for='channelSelect");
    page += i;
    page += F("' id='channelLabel");
    page += i;
    page += F("'>Channel</label>");
    page += F("</div>");
    page += F("<div class='form-floating' hidden id='sequenceDiv");
    page += i;
    page += F("'>");
    page += F("<select class='form-select' id='sequencelSelect");
    page += i;
    page += F("' name='sequence");
    page += i;
    page += F("'>");
    for (unsigned int c = 1; c <= SEQUENCES; c++) {
      page += F("<option value='");
      page += c;
      page += F("'");
      if (act->midiChannel == c) page += F(" selected");
      page += F(">");
      page += c;
      page += F("</option>");
      if (trim_page(start, len)) return;
    }
    page += F("</select>");
    page += F("<label for='sequenceSelect");
    page += i;
    page += F("' id='sequenceLabel");
    page += i;
    page += F("'>Sequence</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='w-25'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='number' class='form-control' id='codeInput");
    page += i;
    page += F("' name='code");
    page += i;
    page += F("' min='0' max='127' value='");
    page += act->midiCode;
    page += F("'>");
    page += F("<label for='codeInput");
    page += i;
    page += F("' id='codeLabel");
    page += i;
    page += F("'>Value</label>");
    page += F("</div>");
    page += F("</div>");
    page += F("</div>");

    page += F("<div class='row g-1'>");
    page += F("<div class='w-50'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='number' class='form-control' id='fromInput");
    page += i;
    page += F("' name='from");
    page += i;
    page += F("' min='0' max='127' value='");
    page += act->midiValue1;
    page += F("'>");
    page += F("<label for='fromInput");
    page += i;
    page += F("' id='fromLabel");
    page += i;
    page += F("'>From Value</label>");
    page += F("</div>");
    page += F("</div>");

    page += F("<div class='w-50'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='number' class='form-control' id='toInput");
    page += i;
    page += F("' name='to");
    page += i;
    page += F("' min='0' max='127' value='");
    page += act->midiValue2;
    page += F("'>");
    page += F("<label for='toInput");
    page += i;
    page += F("' id='toLabel");
    page += i;
    page += F("'>To Value</label>");
    page += F("</div>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    page += F("<div class='w-100'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='text' class='form-control' id ='oscAddress");
    page += i;
    page += F("' name='oscaddress");
    page += i;
    page += F("' maxlength='50' value='");
    page += act->oscAddress;
    page += F("'>");
    page += F("<label for='oscAddress");
    page += i;
    page += F("' id='oscAddressLabel");
    page += i;
    page += F("'>OSC Address</label>");
    page += F("</div>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    page += F("<div class='w-50'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='text' class='form-control' id ='tagOffInput");
    page += i;
    page += F("' name='nameoff");
    page += i;
    page += F("' maxlength='");
    page += MAXACTIONNAME;
    page += F("' value='");
    page += act->tag0;
    page += F("'>");
    page += F("<label for='tagOffInput");
    page += i;
    page += F("' id='tagOffLabel");
    page += i;
    page += F("'>Tag When Off</label>");
    page += F("</div>");
    page += F("</div>");

    page += F("<div class='w-50'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='text' class='form-control' id='tagOnInput");
    page += i;
    page += F("' name='nameon");
    page += i;
    page += F("' maxlength='");
    page += MAXACTIONNAME;
    page += F("' value='");
    page += act->tag1;
    page += F("'>");
    page += F("<label for='tagOnInput");
    page += i;
    page += F("' id='tagOnLabel");
    page += i;
    page += F("'>Tag When On</label>");
    page += F("</div>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    page += F("<div class='w-25'>");
    page += F("<div class='form-floating'>");
    page += F("<select class='form-select' id='ledSelect");
    page += i;
    page += F("' name='led");
    page += i;
    page += F("'>");
    page += F("<option value='0'");
    if (act->led == LEDS) page += F(" selected");
    page += F("></option>");
    page += F("<option value='255'");
    if (act->led == 255) page += F(" selected");
    page += F(">Default</option>");
    for (unsigned int l = 1; l <= LEDS; l++) {
      page += F("<option value='");
      page += l;
      page += F("'");
      if (act->led == l-1) page += F(" selected");
      page += F(">");
      page += l;
      page += F("</option>");

      if (trim_page(start, len)) return;
    }
    page += F("</select>");
    page += F("<label for='ledSelect");
    page += i;
    page += F("' id='ledLabel");
    page += i;
    page += F("'>Led</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='w-25'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='color' class='form-control' id='color0Input");
    page += i;
    page += F("' name='color0-");
    page += i;
    page += F("' value='");
    char color[8];
    sprintf(color, "#%06X", act->color0 & 0xFFFFFF);
    page += color;
    page += F("'>");
    page += F("<label for='color0Input");
    page += i;
    page += F("' id='color0Label");
    page += i;
    page += F("'>Off</label>");
    page += F("</div>");
    page += F("</div>");

    page += F("<div class='w-25'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='color' class='form-control' id='color1Input");
    page += i;
    page += F("' name='color1-");
    page += i;
    page += F("' value='");
    sprintf(color, "#%06X", act->color1 & 0xFFFFFF);
    page += color;
    page += F("'>");
    page += F("<label for='color1Input");
    page += i;
    page += F("' id='color1Label");
    page += i;
    page += F("'>On</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='w-25'>");
    page += F("<div class='form-floating'>");
    page += F("<select class='form-select' id='slotSelect");
    page += i;
    page += F("' name='slot");
    page += i;
    page += F("'>");
    for (unsigned int s = 0; s < SLOTS; s++) {
      page += F("<option value='");
      page += (s+1);
      page += F("'");
      if (act->slot == s) page += F(" selected");
      page += F(">");
      page += (s+1);
      page += F("</option>");

      if (trim_page(start, len)) return;
    }
    page += F("<option value='");
    page += (SLOTS+1);
    page += F("'");
    if (act->slot == SLOTS) page += F(" selected");
    page += F(">");
    page += F("</option>");
    page += F("</select>");
    page += F("<label for='slotSelect");
    page += i;
    page += F("' id='slotLabel");
    page += i;
    page += F("'>Slot</label>");
    page += F("</div>");
    page += F("</div>");
    page += F("</div>");

    page += F("</div>");
    page += F("</div>");

    same_control = (act->next != nullptr && act->control == act->next->control);
    if (!same_control) {
      page += F("</div>");
      page += F("</div>");
      page += F("</div>");
      page += F("</div>");
    }
    act = act->next;
    i++;

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");
  /*
  page += F("<div class='row'>");
  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F(" Apply</button>");
  page += F(" ");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("</div>");
  */
  page += F("</form>");

  if (trim_page(start, len)) return;

  page += F("<script>");
  page += F("document.addEventListener('DOMContentLoaded', init_actions());");
  page += F("function init_actions() {");
  for (byte s = 1; s < i; s++) {
    page += F("  init_action(");
    page += s;
    page += F(");");

    if (trim_page(start, len)) return;
  }

  page += F("};");
  page += F("function init_action(i) {"
            "   let analog;"
            "   switch (document.getElementById('onSelect' + i).options[document.getElementById('onSelect' + i).selectedIndex].text) {"
            "     case 'Move':"
            "     case 'Jog':"
            "       analog = true;"
            "       break;"
            "     default:"
            "       analog = false;"
            "       break;"
            "   };");

  if (trim_page(start, len)) return;

  page += F("   document.getElementById('channelDiv'    + i).removeAttribute('hidden');"
            "   document.getElementById('sequenceDiv'   + i).setAttribute('hidden', 'hidden');"
            "   document.getElementById('codeLabel'     + i).textContent = 'Code';"
            "   document.getElementById('fromLabel'     + i).textContent = 'From Value';"
            "   document.getElementById('toLabel'       + i).textContent = 'To Value';"
            "   document.getElementById('tagOffLabel'   + i).textContent = 'Tag 1';"
            "   document.getElementById('tagOnLabel'    + i).textContent = 'Tag 2';"
            "   document.getElementById('color0Label'   + i).textContent = 'Off';"
            "   document.getElementById('color1Label'   + i).textContent = 'On';"
            "   document.getElementById('channelSelect' + i).disabled = false;"
            "   document.getElementById('codeInput'     + i).disabled = false;"
            "   document.getElementById('fromInput'     + i).disabled = false;"
            "   document.getElementById('toInput'       + i).disabled = false;"
            "   document.getElementById('tagOffInput'   + i).disabled = false;"
            "   document.getElementById('tagOnInput'    + i).disabled = false;"
            "   document.getElementById('oscAddress'    + i).disabled = true;"
            "   document.getElementById('ledSelect'     + i).disabled = false;"
            "   document.getElementById('color0Input'   + i).disabled = false;"
            "   document.getElementById('color1Input'   + i).disabled = false;"
            "   document.getElementById('slotSelect'    + i).disabled = false;");

  if (trim_page(start, len)) return;

  page += F("   switch (document.getElementById('sendSelect' + i).options[document.getElementById('sendSelect' + i).selectedIndex].text) {"
            "     case 'Program Change':"
            "       document.getElementById('codeLabel'     + i).textContent = 'PC';"
            "       document.getElementById('tagOffLabel'   + i).textContent = 'Tag';"
            "       document.getElementById('tagOnLabel'    + i).textContent = 'Tag';"
            "       switch (document.getElementById('pedalMode' + i).value) {"
            "         case 'momentary':"
            "         case 'latch':"
            "         case 'ladder':"
            "           switch (document.getElementById('onSelect' + i).options[document.getElementById('onSelect' + i).selectedIndex].text) {"
            "             case 'Press & Release':"
            "               document.getElementById('fromLabel'     + i).textContent = 'Release PC';"
            "               document.getElementById('toLabel'       + i).textContent = 'Press PC';"
            "               document.getElementById('tagOffLabel'   + i).textContent = 'Release Tag';"
            "               document.getElementById('tagOnLabel'    + i).textContent = 'Press Tag';"
            "               document.getElementById('color0Label'   + i).textContent = 'Release';"
            "               document.getElementById('color1Label'   + i).textContent = 'Press';"
            "               document.getElementById('codeInput'     + i).disabled = true;"
            "               break;"
            "             case 'Release':"
            "               document.getElementById('fromInput'     + i).disabled = true;"
            "               document.getElementById('toInput'       + i).disabled = true;"
            "               document.getElementById('tagOnInput'    + i).disabled = true;"
            "               document.getElementById('color1Input'   + i).disabled = true;"
            "               break;"
            "             default:"
            "               document.getElementById('fromInput'     + i).disabled = true;"
            "               document.getElementById('toInput'       + i).disabled = true;"
            "               document.getElementById('tagOffInput'   + i).disabled = true;"
            "               break;"
            "           }"
            "           break;"
            "       }"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Note On':"
            "     case 'Note Off':"
            "       document.getElementById('codeLabel'     + i).textContent = 'Note';"
            "       switch (document.getElementById('pedalMode' + i).value) {"
            "         case 'momentary':"
            "         case 'latch':"
            "         case 'ladder':"
            "           document.getElementById('fromLabel'     + i).textContent = 'Velocity';"
            "           document.getElementById('toLabel'       + i).textContent = 'Velocity';"
            "           document.getElementById('tagOffLabel'   + i).textContent = 'Tag';"
            "           document.getElementById('tagOnLabel'    + i).textContent = 'Tag';"
            "           switch (document.getElementById('onSelect' + i).options[document.getElementById('onSelect' + i).selectedIndex].text) {"
            "             case 'Press & Release':"
            "               document.getElementById('fromLabel'     + i).textContent = 'Release Velocity';"
            "               document.getElementById('toLabel'       + i).textContent = 'Press Velocity';"
            "               document.getElementById('tagOffLabel'   + i).textContent = 'Release Tag';"
            "               document.getElementById('tagOnLabel'    + i).textContent = 'Press Tag';"
            "               document.getElementById('color0Label'   + i).textContent = 'Release';"
            "               document.getElementById('color1Label'   + i).textContent = 'Press';"
            "               break;"
            "             case 'Release':"
            "               document.getElementById('toInput'       + i).disabled = true;"
            "               document.getElementById('tagOnInput'    + i).disabled = true;"
            "               document.getElementById('color1Input'   + i).disabled = true;"
            "               break;"
            "             default:"
            "               document.getElementById('fromInput'     + i).disabled = true;"
            "               document.getElementById('tagOffInput'   + i).disabled = true;"
            "               document.getElementById('color0Input'   + i).disabled = true;"
            "               break;"
            "           }"
            "           break;"
            "         case 'analog':"
            "         case 'jogwheel':"
            "           document.getElementById('fromLabel'     + i).textContent = 'From Velocity';"
            "           document.getElementById('toLabel'       + i).textContent = 'To Velocity';"
            "           break;"
            "       }"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Control Change':"
            "     case 'Control Change Snap':"
            "       document.getElementById('codeLabel'         + i).textContent = 'CC';"
            "       switch (document.getElementById('pedalMode' + i).value) {"
            "         case 'momentary':"
            "         case 'ladder':"
            "         case 'latch':"
            "           document.getElementById('fromLabel'     + i).textContent = 'Value';"
            "           document.getElementById('toLabel'       + i).textContent = 'Value';"
            "           document.getElementById('tagOffLabel'   + i).textContent = 'Tag';"
            "           document.getElementById('tagOnLabel'    + i).textContent = 'Tag';"
            "           switch (document.getElementById('onSelect' + i).options[document.getElementById('onSelect' + i).selectedIndex].text) {"
            "             case 'Press & Release':"
            "               document.getElementById('fromLabel'     + i).textContent = 'Release Value';"
            "               document.getElementById('toLabel'       + i).textContent = 'Press Value';"
            "               document.getElementById('tagOffLabel'   + i).textContent = 'Release Tag';"
            "               document.getElementById('tagOnLabel'    + i).textContent = 'Press Tag';"
            "               document.getElementById('color0Label'   + i).textContent = 'Release';"
            "               document.getElementById('color1Label'   + i).textContent = 'Press';"
            "               break;"
            "             case 'Release':"
            "               document.getElementById('toInput'       + i).disabled = true;"
            "               document.getElementById('tagOnInput'    + i).disabled = true;"
            "               document.getElementById('color1Input'   + i).disabled = true;"
            "               break;"
            "             default:"
            "               document.getElementById('fromInput'     + i).disabled = true;"
            "               document.getElementById('tagOffInput'   + i).disabled = true;"
            "               document.getElementById('color0Input'   + i).disabled = true;"
            "               break;"
            "           }"
            "           break;"
            "         case 'analog':"
            "         case 'jogwheel':"
            "           document.getElementById('tagOffLabel'   + i).textContent = 'Tag';"
            "           document.getElementById('tagOnLabel'    + i).textContent = 'Tag';"
            "           document.getElementById('tagOffInput'   + i).disabled = true;"
            "           break;"
            "       }"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Program Change+':"
            "     case 'Program Change-':"
            "       document.getElementById('codeLabel'     + i).textContent = 'PC';"
            "       document.getElementById('fromLabel'     + i).textContent = 'From PC';"
            "       document.getElementById('toLabel'       + i).textContent = 'To PC';"
            "       document.getElementById('tagOffLabel'   + i).textContent = 'Tag';"
            "       document.getElementById('tagOnLabel'    + i).textContent = 'Tag';"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('tagOffInput'   + i).disabled = true;"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Bank Select+':"
            "     case 'Bank Select-':"
            "       document.getElementById('fromLabel'     + i).textContent = 'MSB';"
            "       document.getElementById('toLabel'       + i).textContent = 'LSB';"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       break;"
            "     case 'Pitch Bend':"
            "     case 'Channel Pressure':"
            "       document.getElementById('color0Label'   + i).textContent = 'Min';"
            "       document.getElementById('color1Label'   + i).textContent = 'Max';"
            "       document.getElementById('tagOffInput'   + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Bank+':"
            "     case 'Bank-':"
            "       document.getElementById('fromLabel'     + i).textContent = 'From Bank';"
            "       document.getElementById('toLabel'       + i).textContent = 'To Bank';"
            "       document.getElementById('channelSelect' + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('tagOffInput'   + i).disabled = true;"
            "       document.getElementById('tagOnInput'    + i).disabled = true;"
            "       document.getElementById('ledSelect'     + i).disabled = true;"
            "       document.getElementById('color0Input'   + i).disabled = true;"
            "       document.getElementById('color1Input'   + i).disabled = true;"
            "       document.getElementById('slotSelect'    + i).disabled = true;"
            "       break;");

  page += F("     case 'OSC Message':"
            "       document.getElementById('fromLabel'     + i).textContent = 'Min';"
            "       document.getElementById('toLabel'       + i).textContent = 'Max';"
            "       document.getElementById('channelSelect' + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('oscAddress'    + i).disabled = false;"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Profile+':"
            "     case 'Profile-':"
            "       document.getElementById('fromLabel'     + i).textContent = 'From Profile';"
            "       document.getElementById('toLabel'       + i).textContent = 'To Profile';"
            "       document.getElementById('channelSelect' + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('tagOffInput'   + i).disabled = true;"
            "       document.getElementById('tagOnInput'    + i).disabled = true;"
            "       document.getElementById('ledSelect'     + i).disabled = true;"
            "       document.getElementById('color0Input'   + i).disabled = true;"
            "       document.getElementById('color1Input'   + i).disabled = true;"
            "       document.getElementById('slotSelect'    + i).disabled = true;"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Sequence':"
            "       document.getElementById('channelDiv'    + i).setAttribute('hidden', 'hidden');"
            "       document.getElementById('sequenceDiv'   + i).removeAttribute('hidden');"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('fromInput'     + i).disabled = true;"
            "       document.getElementById('toInput'       + i).disabled = true;"
            "       document.getElementById('color0Input'   + i).disabled = true;"
            "       document.getElementById('color1Input'   + i).disabled = true;"
            "       break;"
            "     case 'Step by Step+':"
            "     case 'Step by Step-':"
            "       document.getElementById('channelDiv'    + i).setAttribute('hidden', 'hidden');"
            "       document.getElementById('sequenceDiv'   + i).removeAttribute('hidden');"
            "       document.getElementById('codeLabel'     + i).textContent = 'Step';"
            "       document.getElementById('fromLabel'     + i).textContent = 'Tag 1 Value';"
            "       document.getElementById('toLabel'       + i).textContent = 'Tag 2 Value';"
            "       document.getElementById('color0Input'   + i).disabled = true;"
            "       document.getElementById('color1Input'   + i).disabled = true;"
            "       break;"
            "     case 'Set Led Color':"
            "       switch (document.getElementById('pedalMode' + i).value) {"
            "         case 'analog':"
            "           document.getElementById('color0Label'   + i).textContent = 'Min';"
            "           document.getElementById('color1Label'   + i).textContent = 'Max';"
            "           break;"
            "         default:"
            "           document.getElementById('color0Label'   + i).textContent = 'Color';"
            "           document.getElementById('color1Label'   + i).textContent = 'Color';"
            "           document.getElementById('color1Input'   + i).disabled = true;"
            "           break;"
            "       }"
            "       document.getElementById('channelSelect' + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('fromInput'     + i).disabled = true;"
            "       document.getElementById('toInput'       + i).disabled = true;"
            "       document.getElementById('tagOffInput'   + i).disabled = true;"
            "       document.getElementById('tagOnInput'    + i).disabled = true;"
            "       document.getElementById('slotSelect'    + i).disabled = true;"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Repeat':"
            "       document.getElementById('channelSelect' + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('fromInput'     + i).disabled = true;"
            "       document.getElementById('toInput'       + i).disabled = true;"
            "       document.getElementById('tagOffInput'   + i).disabled = true;"
            "       document.getElementById('tagOnInput'    + i).disabled = true;"
            "       document.getElementById('color0Input'   + i).disabled = true;"
            "       document.getElementById('color1Input'   + i).disabled = true;"
            "       document.getElementById('slotSelect'    + i).disabled = true;"
            "       break;"
            "     case 'Repeat Overwrite':"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('tagOffInput'   + i).disabled = true;"
            "       document.getElementById('tagOnInput'    + i).disabled = true;"
            "       document.getElementById('slotSelect'    + i).disabled = true;"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Start':"
            "     case 'Stop':"
            "     case 'Continue':"
            "     case 'MTC Start':"
            "     case 'MTC Stop':"
            "     case 'MTC Continue':"
            "     case 'Tap':"
            "     case 'BPM+':"
            "     case 'BPM-':"
            "     case 'Device Info':"
            "     case 'Power On/Off':"
            "     default:"
            "       document.getElementById('channelSelect' + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('fromInput'     + i).disabled = true;"
            "       document.getElementById('toInput'       + i).disabled = true;"
            "       document.getElementById('tagOffInput'   + i).disabled = true;"
            "       document.getElementById('tagOnInput'    + i).disabled = true;"
            "       document.getElementById('ledSelect'     + i).disabled = true;"
            "       document.getElementById('color0Input'   + i).disabled = true;"
            "       document.getElementById('color1Input'   + i).disabled = true;"
            "       document.getElementById('slotSelect'    + i).disabled = true;"
            "       break;"
            "   }"
            "};");
  page += F("</script>");

  get_footer_page();

  if (trim_page(start, len, true)) return;
}

void get_pedals_page(unsigned int start, unsigned int len) {

  if (get_top_page(4, start, len)) return;

  page += F("<form method='post'>");
  page += F("<div class='container'>");
  page += F("<div class='row row-cols-1 row-cols-sm-2 row-cols-md-3 row-cols-lg-4 row-cols-xl-4 row-cols-xxl-4 g-2 g-md-3 g-xl-4'>");

  for (unsigned int i = 1; i <= PEDALS; i++) {
    page += F("<div class='col'>");
    page += F("<div class='card'>");
    page += F("<h5 class='card-header'>");
    page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-controller' viewBox='0 0 20 20'>");
    page += F("<path d='M11.5 6.027a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0zm-1.5 1.5a.5.5 0 1 0 0-1 .5.5 0 0 0 0 1zm2.5-.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0zm-1.5 1.5a.5.5 0 1 0 0-1 .5.5 0 0 0 0 1zm-6.5-3h1v1h1v1h-1v1h-1v-1h-1v-1h1v-1z'/>");
    page += F("<path d='M3.051 3.26a.5.5 0 0 1 .354-.613l1.932-.518a.5.5 0 0 1 .62.39c.655-.079 1.35-.117 2.043-.117.72 0 1.443.041 2.12.126a.5.5 0 0 1 .622-.399l1.932.518a.5.5 0 0 1 .306.729c.14.09.266.19.373.297.408.408.78 1.05 1.095 1.772.32.733.599 1.591.805 2.466.206.875.34 1.78.364 2.606.024.816-.059 1.602-.328 2.21a1.42 1.42 0 0 1-1.445.83c-.636-.067-1.115-.394-1.513-.773-.245-.232-.496-.526-.739-.808-.126-.148-.25-.292-.368-.423-.728-.804-1.597-1.527-3.224-1.527-1.627 0-2.496.723-3.224 1.527-.119.131-.242.275-.368.423-.243.282-.494.575-.739.808-.398.38-.877.706-1.513.773a1.42 1.42 0 0 1-1.445-.83c-.27-.608-.352-1.395-.329-2.21.024-.826.16-1.73.365-2.606.206-.875.486-1.733.805-2.466.315-.722.687-1.364 1.094-1.772a2.34 2.34 0 0 1 .433-.335.504.504 0 0 1-.028-.079zm2.036.412c-.877.185-1.469.443-1.733.708-.276.276-.587.783-.885 1.465a13.748 13.748 0 0 0-.748 2.295 12.351 12.351 0 0 0-.339 2.406c-.022.755.062 1.368.243 1.776a.42.42 0 0 0 .426.24c.327-.034.61-.199.929-.502.212-.202.4-.423.615-.674.133-.156.276-.323.44-.504C4.861 9.969 5.978 9.027 8 9.027s3.139.942 3.965 1.855c.164.181.307.348.44.504.214.251.403.472.615.674.318.303.601.468.929.503a.42.42 0 0 0 .426-.241c.18-.408.265-1.02.243-1.776a12.354 12.354 0 0 0-.339-2.406 13.753 13.753 0 0 0-.748-2.295c-.298-.682-.61-1.19-.885-1.465-.264-.265-.856-.523-1.733-.708-.85-.179-1.877-.27-2.913-.27-1.036 0-2.063.091-2.913.27z'/>");
    page += F("</svg>");
    page += F(" Pedal ");
    page += i;
    page += F("</h5>");
    page += F("<div class='card-body'>");
    page += F("<div class='d-grid gap-1'>");
    page += F("<div class='form-floating'>");
    page += F("<select class='form-select' id='modeSelect");
    page += i;
    page += F("' name='mode");
    page += i;
    page += F("' onchange='init_pedal(");
    page += i;
    page += F(")'>");

    if (trim_page(start, len)) return;

    page += F("<option value='");
    page += PED_NONE;
    page += F("'");
    if (pedals[i-1].mode == PED_NONE) page += F(" selected");
    page += F(">None</option>");
    page += F("<option value='");
    page += PED_MOMENTARY1;
    page += F("'");
    if (pedals[i-1].mode == PED_MOMENTARY1) page += F(" selected");
    page += F(">Momentary</option>");
    if (PIN_D(i-1) != PIN_A(i-1)) {
      page += F("<option value='");
      page += PED_LATCH1;
      page += F("'");
      if (pedals[i-1].mode == PED_LATCH1) page += F(" selected");
      page += F(">Latch</option>");
      page += F("<option value='");
      page += PED_ANALOG;
      page += F("'");
      if (pedals[i-1].mode == PED_ANALOG) page += F(" selected");
      page += F(">Analog</option>");
      page += F("<option value='");
      page += PED_ANALOG_PAD;
      page += F("'");
      if (pedals[i-1].mode == PED_ANALOG_PAD) page += F(" selected");
      page += F(">Analog Pad</option>");
      page += F("<option value='");
      page += PED_JOG_WHEEL;
      page += F("'");
      if (pedals[i-1].mode == PED_JOG_WHEEL) page += F(" selected");
      page += F(">Jog Wheel</option>");
      page += F("<option value='");
      page += PED_MOMENTARY2;
      page += F("'");
      if (pedals[i-1].mode == PED_MOMENTARY2) page += F(" selected");
      page += F(">Momentary 2</option>");
      page += F("<option value='");
      page += PED_MOMENTARY3;
      page += F("'");
      if (pedals[i-1].mode == PED_MOMENTARY3) page += F(" selected");
      page += F(">Momentary 3</option>");
      page += F("<option value='");
      page += PED_LATCH2;
      page += F("'");
      if (pedals[i-1].mode == PED_LATCH2) page += F(" selected");
      page += F(">Latch 2</option>");
      page += F("<option value='");
      page += PED_LADDER;
      page += F("'");
      if (pedals[i-1].mode == PED_LADDER) page += F(" selected");
      page += F(">Ladder</option>");
      page += F("<option value='");
      page += PED_ULTRASONIC;
      page += F("'");
      if (pedals[i-1].mode == PED_ULTRASONIC) page += F(" selected");
      page += F(">Ultrasonic</option>");
      page += F("<option value='");
      page += PED_ANALOG_MOMENTARY;
      page += F("'");
      if (pedals[i-1].mode == PED_ANALOG_MOMENTARY) page += F(" selected");
      page += F(">Analog+Momentary</option>");
      page += F("<option value='");
      page += PED_ANALOG_LATCH;
      page += F("'");
      if (pedals[i-1].mode == PED_ANALOG_LATCH) page += F(" selected");
      page += F(">Analog+Latch</option>");
      page += F("<option value='");
      page += PED_ANALOG_PAD_MOMENTARY;
      page += F("'");
      if (pedals[i-1].mode == PED_ANALOG_PAD_MOMENTARY) page += F(" selected");
      page += F(">Analog Pad+Momentary</option>");
      page += F("<option value='");
      page += PED_ANALOG4;
      page += F("'");
      if (pedals[i-1].mode == PED_ANALOG4) page += F(" selected");
      page += F(">Analog 4</option>");
    }

    page += F("</select>");
    page += F("<label for='modeSelect");
    page += i;
    page += F("'>Mode</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-check form-switch' id='polarityCheck_");
    page += i;
    page += F("'>");
    page += F("<input class='form-check-input' type='checkbox' id='polarityCheck");
    page += i;
    page += F("' name='polarity");
    page += i;
    page += F("'");
    if (pedals[i-1].invertPolarity) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='polarityCheck");
    page += i;
    page += F("'>Invert Polarity</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-check form-switch' id='function1Check_");
    page += i;
    page += F("'>");
    page += F("<input class='form-check-input' type='checkbox' id='function1Check");
    page += i;
    page += F("' name='function1");
    page += i;
    page += F("'");
    if (IS_SINGLE_PRESS_ENABLED(pedals[i-1].pressMode)) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='function1Check");
    page += i;
    page += F("'>Single Press</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-check form-switch'id='function2Check_");
    page += i;
    page += F("'>");
    page += F("<input class='form-check-input' type='checkbox' id='function2Check");
    page += i;
    page += F("' name='function2");
    page += i;
    page += F("'");
    if (IS_DOUBLE_PRESS_ENABLED(pedals[i-1].pressMode)) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='function2Check");
    page += i;
    page += F("'>Double Press</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-check form-switch' id='function3Check_");
    page += i;
    page += F("'>");
    page += F("<input class='form-check-input' type='checkbox' id='function3Check");
    page += i;
    page += F("' name='function3");
    page += i;
    page += F("'");
    if (IS_LONG_PRESS_ENABLED(pedals[i-1].pressMode)) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='function3Check");
    page += i;
    page += F("'>Long Press</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-check form-switch' id='latchEmulation_");
    page += i;
    page += F("'>");
    page += F("<input class='form-check-input' type='checkbox' id='latchEmulation");
    page += i;
    page += F("' name='latchemulation");
    page += i;
    page += F("'");
    if (pedals[i-1].latchEmulation) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='latchEmulation");
    page += i;
    page += F("'>Latch Emulation</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-check form-switch' id='autoCheck_");
    page += i;
    page += F("'>");
    page += F("<input class='form-check-input' type='checkbox' id='autoCheck");
    page += i;
    page += F("' name='autosensing");
    page += i;
    page += F("'");
    if (pedals[i-1].autoSensing) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='autoCheck");
    page += i;
    page += F("'>Analog Calibration</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-floating' id='mapSelect_");
    page += i;
    page += F("'>");
    page += F("<select class='form-select' id='mapSelect");
    page += i;
    page += F("' name='map");
    page += i;
    page += F("'>");
    page += F("<option value='");
    page += PED_LINEAR;
    page += F("'");
    if (pedals[i-1].analogResponse == PED_LINEAR) page += F(" selected");
    page += F(">Linear</option>");
    page += F("<option value='");
    page += PED_LOG;
    page += F("'");
    if (pedals[i-1].analogResponse == PED_LOG) page += F(" selected");
    page += F(">Log</option>");
    page += F("<option value='");
    page += PED_ANTILOG;
    page += F("'");
    if (pedals[i-1].analogResponse == PED_ANTILOG) page += F(" selected");
    page += F(">Antilog</option>");
    page += F("</select>");
    page += F("<label for='mapSelect");
    page += i;
    page += F("'>Analog Response</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    page += F("<div class='form-floating w-50' id='minInput_");
    page += i;
    page += F("'>");
    page += F("<input type='number' class='form-control' id='minInput");
    page += i;
    page += F("' name='min");
    page += i;
    page += F("' min='0' max='");
    page += (ADC_RESOLUTION - 1);
    page += F("' value='");
    page += pedals[i-1].expZero;
    page += F("'>");
    page += F("<label for='minInput");
    page += i;
    page += F("' id='minInputLabel");
    page += i;
    page += F("'>Min</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-floating w-50' id='maxInput_");
    page += i;
    page += F("'>");
    page += F("<input type='number' class='form-control' id='maxInput");
    page += i;
    page += F("' name='max");
    page += i;
    page += F("' min='0' max='");
    page += (ADC_RESOLUTION - 1);
    page += F("' value='");
    page += pedals[i-1].expMax;
    page += F("'>");
    page += F("<label for='maxInput");
    page += i;
    page += F("' id='maxInputLabel");
    page += i;
    page += F("'>Max</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div id='easing_");
    page += i;
    page += F("'>");
    page += F("<label for='easing");
    page += i;
    page += F("'>Easing: ");
    page += (4 + lround(log10(pedals[i-1].snapMultiplier)));
    page += F("</label>");
    page += F("<input type='range' class='form-range' min='1' max='3' id='easing");
    page += i;
    page += F("' name='analogeasing");
    page += i;
    page += F("' value='");
    page += (4 + lround(log10(pedals[i-1].snapMultiplier)));
    page += F("' oninput='this.previousElementSibling.textContent = `Easing: ` + this.value'>");
    page += F("</div>");

    page += F("<div id='threshold_");
    page += i;
    page += F("'>");
    page += F("<label for='threshold");
    page += i;
    page += F("'>Activity Threshold: ");
    page += (int)pedals[i-1].activityThreshold;
    page += F("</label>");
    page += F("<input type='range' class='form-range' min='1' max='8' id='threshold");
    page += i;
    page += F("' name='analogthreshold");
    page += i;
    page += F("' value='");
    page += (log10(pedals[i-1].activityThreshold) / log10(2));
    page += F("' oninput='this.previousElementSibling.textContent = `Activity Threshold: ` + Math.pow(2, this.value)'>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    page += F("<div class='form-floating w-50' id='gain_");
    page += i;
    page += F("'>");
    page += F("<input type='number' class='form-control' id='gain");
    page += i;
    page += F("' name='gain");
    page += i;
    page += F("' min='0' max='");
    page += 100;
    page += F("' step='0.01' value='");
    page += String(pedals[i-1].gain, 3);
    page += F("'>");
    page += F("<label for='gain");
    page += i;
    page += F("'>Gain</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-floating w-50' id='retrigger_");
    page += i;
    page += F("'>");
    page += F("<input type='number' class='form-control' id='retrigger");
    page += i;
    page += F("' name='retrigger");
    page += i;
    page += F("' step='0.01' min='0' max='");
    page += 1;
    page += F("' value='");
    page += String(pedals[i-1].retrigger, 3);
    page += F("'>");
    page += F("<label for='retrigger");
    page += i;
    page += F("'>Retrigger</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    page += F("<div class='form-floating w-50' id='scanCycles_");
    page += i;
    page += F("'>");
    page += F("<input type='number' class='form-control' id='scanCycles");
    page += i;
    page += F("' name='scancycles");
    page += i;
    page += F("' min='0' max='");
    page += 100;
    page += F("' value='");
    page += pedals[i-1].scanCycles;
    page += F("'>");
    page += F("<label for='scanCycles");
    page += i;
    page += F("'>Scan</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-floating w-50' id='holdCycles_");
    page += i;
    page += F("'>");
    page += F("<input type='number' class='form-control' id='holdCycles");
    page += i;
    page += F("' name='holdcycles");
    page += i;
    page += F("' min='0' max='");
    page += 100;
    page += F("' value='");
    page += pedals[i-1].holdCycles;
    page += F("'>");
    page += F("<label for='holdCycles");
    page += i;
    page += F("'>Hold</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    page += F("<div class='form-floating w-50' id='crosstalk_");
    page += i;
    page += F("'>");
    page += F("<input type='number' class='form-control' id='crosstalk");
    page += i;
    page += F("' name='crosstalk");
    page += i;
    page += F("' min='0' max='");
    page += 100;
    page += F("' value='");
    page += pedals[i-1].crosstalk;
    page += F("'>");
    page += F("<label for='crosstalk");
    page += i;
    page += F("'>Crosstalk (ms)</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-floating w-50' id='crosstalkRatio_");
    page += i;
    page += F("'>");
    page += F("<input type='number' class='form-control' id='crosstalkRatio");
    page += i;
    page += F("' name='crosstalkratio");
    page += i;
    page += F("' step='0.01' min='0' max='");
    page += 100;
    page += F("' value='");
    page += String(pedals[i-1].crosstalkRatio, 3);
    page += F("'>");
    page += F("<label for='crosstalkRatio");
    page += i;
    page += F("'>Crosstalk Ratio</label>");
    page += F("</div>");
    page += F("</div>");

    page += F("</div>");
    page += F("</div>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");

  page += F("<div class='container'>");
  page += F("<div class='row mt-3'>");
  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F(" Apply</button>");
  page += F(" ");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</form>");

  if (trim_page(start, len)) return;

  page += F("<script>");
  page += F("document.addEventListener('DOMContentLoaded', init_pedals());");
  page += F("function init_pedals() {");
  for (byte s = 1; s <= PEDALS; s++) {
    page += F("  init_pedal(");
    page += s;
    page += F(");");
  }
  page += F("};");
  page += F("function init_pedal(i) {"
            "   document.getElementById('minInputLabel'        + i).textContent = 'Min';"
            "   document.getElementById('maxInputLabel'        + i).textContent = 'Max';"
            "   document.getElementById('polarityCheck_'       + i).style.display = 'none';"
            "   document.getElementById('function1Check_'      + i).style.display = 'none';"
            "   document.getElementById('function2Check_'      + i).style.display = 'none';"
            "   document.getElementById('function3Check_'      + i).style.display = 'none';"
            "   document.getElementById('latchEmulation_'      + i).style.display = 'none';"
            "   document.getElementById('mapSelect_'           + i).style.display = 'none';"
            "   document.getElementById('minInput_'            + i).style.display = 'none';"
            "   document.getElementById('maxInput_'            + i).style.display = 'none';"
            "   document.getElementById('easing_'              + i).style.display = 'none';"
            "   document.getElementById('threshold_'           + i).style.display = 'none';"
            "   document.getElementById('autoCheck_'           + i).style.display = 'none';"
            "   document.getElementById('gain_'                + i).style.display = 'none';"
            "   document.getElementById('retrigger_'           + i).style.display = 'none';"
            "   document.getElementById('scanCycles_'          + i).style.display = 'none';"
            "   document.getElementById('holdCycles_'          + i).style.display = 'none';"
            "   document.getElementById('crosstalk_'           + i).style.display = 'none';"
            "   document.getElementById('crosstalkRatio_'      + i).style.display = 'none';");

  if (trim_page(start, len)) return;

  page += F("   switch (document.getElementById('modeSelect' + i).options[document.getElementById('modeSelect' + i).selectedIndex].text) {"
            "     case 'Momentary':"
            "     case 'Momentary 2':"
            "     case 'Momentary 3':"
            "     case 'Ladder':"
            "       document.getElementById('latchEmulation_'  + i).style.display = 'block';"
            "     case 'Latch':"
            "     case 'Latch 2':"
            "     case 'Jog Wheel':"
            "       document.getElementById('polarityCheck_'   + i).style.display = 'block';"
            "       document.getElementById('function1Check_'  + i).style.display = 'block';"
            "       document.getElementById('function2Check_'  + i).style.display = 'block';"
            "       document.getElementById('function3Check_'  + i).style.display = 'block';"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Ultrasonic':"
            "       document.getElementById('polarityCheck_'   + i).style.display = 'block';"
            "       document.getElementById('mapSelect_'       + i).style.display = 'block';"
            "       document.getElementById('minInput_'        + i).style.display = 'block';"
            "       document.getElementById('maxInput_'        + i).style.display = 'block';"
            "       document.getElementById('easing_'          + i).style.display = 'block';"
            "       document.getElementById('threshold_'       + i).style.display = 'block';"
            "       break;"
            "     case 'Analog':"
            "     case 'Analog 4':"
            "       document.getElementById('polarityCheck_'   + i).style.display = 'block';"
            "       document.getElementById('mapSelect_'       + i).style.display = 'block';"
            "       document.getElementById('minInput_'        + i).style.display = 'block';"
            "       document.getElementById('maxInput_'        + i).style.display = 'block';"
            "       document.getElementById('easing_'          + i).style.display = 'block';"
            "       document.getElementById('threshold_'       + i).style.display = 'block';"
            "       document.getElementById('autoCheck_'       + i).style.display = 'block';"
            "       break;"
            "     case 'Analog Pad+Momentary':"
            "       document.getElementById('polarityCheck_'   + i).style.display = 'block';"
            "       document.getElementById('function1Check_'  + i).style.display = 'block';"
            "       document.getElementById('function2Check_'  + i).style.display = 'block';"
            "       document.getElementById('function3Check_'  + i).style.display = 'block';"
            "       document.getElementById('latchEmulation_'  + i).style.display = 'block';"
            "     case 'Analog Pad':"
            "       document.getElementById('minInputLabel'    + i).textContent   = 'Threshold';"
            "       document.getElementById('maxInputLabel'    + i).textContent   = 'High Cut';"
            "       document.getElementById('mapSelect_'       + i).style.display = 'block';"
            "       document.getElementById('minInput_'        + i).style.display = 'block';"
            "       document.getElementById('maxInput_'        + i).style.display = 'block';"
            "       document.getElementById('gain_'            + i).style.display = 'block';"
            "       document.getElementById('retrigger_'       + i).style.display = 'block';"
            "       document.getElementById('scanCycles_'      + i).style.display = 'block';"
            "       document.getElementById('holdCycles_'      + i).style.display = 'block';"
            "       document.getElementById('crosstalk_'       + i).style.display = 'block';"
            "       document.getElementById('crosstalkRatio_'  + i).style.display = 'block';"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Analog+Momentary':"
            "       document.getElementById('polarityCheck_'   + i).style.display = 'block';"
            "       document.getElementById('function1Check_'  + i).style.display = 'block';"
            "       document.getElementById('function2Check_'  + i).style.display = 'block';"
            "       document.getElementById('function3Check_'  + i).style.display = 'block';"
            "       document.getElementById('latchEmulation_'  + i).style.display = 'block';"
            "       document.getElementById('mapSelect_'       + i).style.display = 'block';"
            "       document.getElementById('minInput_'        + i).style.display = 'block';"
            "       document.getElementById('maxInput_'        + i).style.display = 'block';"
            "       document.getElementById('easing_'          + i).style.display = 'block';"
            "       document.getElementById('threshold_'       + i).style.display = 'block';"
            "       document.getElementById('autoCheck_'       + i).style.display = 'block';"
            "       break;"
            "     case 'Analog+Latch':"
            "       document.getElementById('polarityCheck_'   + i).style.display = 'block';"
            "       document.getElementById('function1Check_'  + i).style.display = 'block';"
            "       document.getElementById('function2Check_'  + i).style.display = 'block';"
            "       document.getElementById('function3Check_'  + i).style.display = 'block';"
            "       document.getElementById('mapSelect_'       + i).style.display = 'block';"
            "       document.getElementById('minInput_'        + i).style.display = 'block';"
            "       document.getElementById('maxInput_'        + i).style.display = 'block';"
            "       document.getElementById('easing_'          + i).style.display = 'block';"
            "       document.getElementById('threshold_'       + i).style.display = 'block';"
            "       document.getElementById('autoCheck_'       + i).style.display = 'block';"
            "       break;"
            "     default:"
            "       break;"
            "   }"
            "};");
  page += F("</script>");

  get_footer_page();

  if (trim_page(start, len, true)) return;
}

void get_controls_page(unsigned int start, unsigned int len) {

  const byte cp = constrain(uicontrolpage.toInt(), 1, CONTROLS / CONTROLS_PER_PAGE);

  if (get_top_page(4, start, len)) return;

  page += F("<div class='btn-group mb-3'>");
  for (unsigned int i = 1; i <= CONTROLS/CONTROLS_PER_PAGE; i++) {
    page += F("<form method='get'><button type='button submit' class='btn");
    page += (uicontrolpage == String(i) ? F(" btn-primary") : F(""));
    page += F("' name='controlpage' value='");
    page += i;
    page += F("'>");
    page += i;
    page += F("</button></form>");
  }
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<form method='post'>");

  page += F("<div class='card mb-3'>");
  page += F("<div class='card-header'>");
  page += F("<div class='row'>");
  page += F("<div class='col-auto me-auto'>");
  page += F("<h5 class='mb-0'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-toggles' viewBox='0 0 20 20'>");
  page += F("<path d='M4.5 9a3.5 3.5 0 1 0 0 7h7a3.5 3.5 0 1 0 0-7h-7zm7 6a2.5 2.5 0 1 1 0-5 2.5 2.5 0 0 1 0 5zm-7-14a2.5 2.5 0 1 0 0 5 2.5 2.5 0 0 0 0-5zm2.45 0A3.49 3.49 0 0 1 8 3.5 3.49 3.49 0 0 1 6.95 6h4.55a2.5 2.5 0 0 0 0-5H6.95zM4.5 0h7a3.5 3.5 0 1 1 0 7h-7a3.5 3.5 0 1 1 0-7z'/>");
  page += F("</svg>");
  page += F(" Controls ");
  page += (cp-1)*CONTROLS_PER_PAGE+1;
  page += F("-");
  page += cp*CONTROLS_PER_PAGE;
  page += F("</h5>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F(" Apply</button>");
  page += F(" ");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  page += F("<div class='card-body'>");

  page += F("<div class='container'>");
  page += F("<div class='row row-cols-1 row-cols-sm-2 row-cols-md-3 row-cols-lg-4 row-cols-xl-5 row-cols-xxl-6 g-2 g-md-3 g-xl-4'>");

  if (trim_page(start, len)) return;

  for (byte i = (cp-1)*CONTROLS_PER_PAGE+1; i <= cp*CONTROLS_PER_PAGE; i++) {
    page += F("<div class='col'>");
    page += F("<div class='card'>");
    page += F("<h5 class='card-header'>");
    page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-toggles' viewBox='0 0 20 20'>");
    page += F("<path d='M4.5 9a3.5 3.5 0 1 0 0 7h7a3.5 3.5 0 1 0 0-7h-7zm7 6a2.5 2.5 0 1 1 0-5 2.5 2.5 0 0 1 0 5zm-7-14a2.5 2.5 0 1 0 0 5 2.5 2.5 0 0 0 0-5zm2.45 0A3.49 3.49 0 0 1 8 3.5 3.49 3.49 0 0 1 6.95 6h4.55a2.5 2.5 0 0 0 0-5H6.95zM4.5 0h7a3.5 3.5 0 1 1 0 7h-7a3.5 3.5 0 1 1 0-7z'/>");
    page += F("</svg>");
    page += F(" Control ");
    page += i;
    page += F("</h5>");
    page += F("<div class='card-body'>");
    page += F("<div class='d-grid gap-1'>");

    if (trim_page(start, len)) return;

    page += F("<div class='row g-1'>");
    for (byte j = 1; j <= 2; j++) {
      page += F("<div class='w-50'>");
      page += F("<div class='form-floating'>");
      page += F("<select class='form-select' id='pedalSelect");
      page += i;
      page += F("' name='pedal");
      page += j;
      page += F("-");
      page += i;
      page += F("'>");
      for (byte p = 0; p <= PEDALS; p++) {
        page += F("<option value='");
        page += p;
        page += F("'");
        switch (j) {
          case 1:
            if ((p == 0 && controls[i-1].pedal1 == PEDALS) || controls[i-1].pedal1 == p-1) page += F(" selected");
            break;
          case 2:
            if ((p == 0 && controls[i-1].pedal2 == PEDALS) || controls[i-1].pedal2 == p-1) page += F(" selected");
            break;
        }
        page += F(">");
        if (p > 0) page += p;
        page += F("</option>");

        if (trim_page(start, len)) return;
      }
      page += F("</select>");
      page += F("<label for='pedalSelect");
      page += i;
      page += F("' id='pedalLabel");
      page += i;
      page += F("'>Pedal</label>");
      page += F("</div>");
      page += F("</div>");

      if (trim_page(start, len)) return;

      page += F("<div class='w-50'>");
      page += F("<div class='form-floating'>");
      page += F("<select class='form-select' id='buttonSelect");
      page += i;
      page += F("' name='button");
      page += j;
      page += F("-");
      page += i;
      page += F("'>");
      for (byte s = 0; s <= LADDER_STEPS; s++) {
        page += F("<option value='");
        page += s;
        page += F("'");
        switch (j) {
          case 1:
            if ((s == 0 && controls[i-1].button1 == LADDER_STEPS) || controls[i-1].button1 == s-1) page += F(" selected");
            break;
          case 2:
            if ((s == 0 && controls[i-1].button2 == LADDER_STEPS) || controls[i-1].button2 == s-1) page += F(" selected");
            break;
        }
        page += F(">");
        if (s > 0) page += s;
        page += F("</option>");

        if (trim_page(start, len)) return;
      }
      page += F("</select>");
      page += F("<label for='buttonSelect");
      page += i;
      page += F("' id='buttonLabel");
      page += i;
      page += F("'>Button</label>");
      page += F("</div>");
      page += F("</div>");

      if (trim_page(start, len)) return;
    }
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-floating w-50'>");
    page += F("<select class='form-select' id='ledSelect");
    page += (i);
    page += F("' name='led");
    page += (i);
    page += F("'>");
    for (unsigned int l = 0; l <= LEDS; l++) {
      page += F("<option value='");
      page += l;
      page += F("'");
      if ((l == 0 && controls[i-1].led == LEDS) || controls[i-1].led == l-1) page += F(" selected");
      page += F(">");
      if (l > 0) page += l;
      page += F("</option>");

      if (trim_page(start, len)) return;
    }
    page += F("</select>");
    page += F("<label for='ledSelect");
    page += (i);
    page += F("' id='ledLabel");
    page += (i);
    page += F("'>Led");
    page += F("</label>");
    page += F("</div>");

    page += F("</div>");
    page += F("</div>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;
  }

  page += F("</div>");
  page += F("</div>");
/*
  page += F("<div class='container'>");
  page += F("<div class='row mt-3'>");
  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F(" Apply</button>");
  page += F(" ");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
*/

  page += F("</div>");
  page += F("</div>");

  page += F("</form>");

  if (trim_page(start, len)) return;

  get_footer_page();

  if (trim_page(start, len, true)) return;
}

void get_interfaces_page(unsigned int start, unsigned int len) {

  if (get_top_page(5, start, len)) return;

  page += F("<form method='post'>");

  page += F("<div class='container'>");
  page += F("<div class='row row-cols-1 row-cols-sm-2 row-cols-md-3 row-cols-lg-4 row-cols-xl-5 row-cols-xxl-6 g-2 g-md-3 g-xl-4'>");
  for (unsigned int i = 1; i <= INTERFACES; i++) {
    page += F("<div class='col'>");
    page += F("<div class='card h-100'>");
    page += F("<h6 class='card-header'>");
    page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-hdd-network' viewBox='0 0 20 20'>");
    page += F("<path d='M4.5 5a.5.5 0 1 0 0-1 .5.5 0 0 0 0 1zM3 4.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0z'/>");
    page += F("<path d='M0 4a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v1a2 2 0 0 1-2 2H8.5v3a1.5 1.5 0 0 1 1.5 1.5h5.5a.5.5 0 0 1 0 1H10A1.5 1.5 0 0 1 8.5 14h-1A1.5 1.5 0 0 1 6 12.5H.5a.5.5 0 0 1 0-1H6A1.5 1.5 0 0 1 7.5 10V7H2a2 2 0 0 1-2-2V4zm1 0v1a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V4a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1zm6 7.5v1a.5.5 0 0 0 .5.5h1a.5.5 0 0 0 .5-.5v-1a.5.5 0 0 0-.5-.5h-1a.5.5 0 0 0-.5.5z'/>");
    page += F("</svg> ");
    page += interfaces[i-1].name;
    page += F("</h6>");
    page += F("<div class='card-body'>");
    page += F("<div class='form-check form-switch'>");
    page += F("<input class='form-check-input' type='checkbox' id='inCheck");
    page += i;
    page += F("' name='in");
    page += i;
    page += F("'");
    if (IS_INTERFACE_ENABLED(interfaces[i-1].midiIn)) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='inCheck");
    page += i;
    page += F("'>In</label>");
    page += F("</div>");
    page += F("<div class='form-check form-switch'>");
    page += F("<input class='form-check-input' type='checkbox' id='outCheck");
    page += i;
    page += F("' name='out");
    page += i;
    page += F("'");
    if (IS_INTERFACE_ENABLED(interfaces[i-1].midiOut)) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='outCheck");
    page += i;
    page += F("'>Out</label>");
    page += F("</div>");
    page += F("<div class='form-check form-switch'>");
    page += F("<input class='form-check-input' type='checkbox' id='thruCheck");
    page += i;
    page += F("' name='thru");
    page += i;
    page += F("'");
    if (interfaces[i-1].midiThru) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='thruCheck");
    page += i;
    page += F("'>Thru</label>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='form-check form-switch'>");
    page += F("<input class='form-check-input' type='checkbox' id='clockCheck");
    page += i;
    page += F("' name='clock");
    page += i;
    page += F("'");
    if (interfaces[i-1].midiClock) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='clockCheck");
    page += i;
    page += F("'>Clock</label>");
    page += F("</div>");
    page += F("<div class='form-check form-switch'>");
    page += F("<input class='form-check-input' type='checkbox' id='showInCheck");
    page += i;
    page += F("' name='showin");
    page += i;
    page += F("'");
    if (IS_SHOW_ENABLED(interfaces[i-1].midiIn)) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='showInCheck");
    page += i;
    page += F("'>Show Incoming</label>");
    page += F("</div>");
    page += F("<div class='form-check form-switch'>");
    page += F("<input class='form-check-input' type='checkbox' id='showOutCheck");
    page += i;
    page += F("' name='showout");
    page += i;
    page += F("'");
    if (IS_SHOW_ENABLED(interfaces[i-1].midiOut)) page += F(" checked");
    page += F(">");
    page += F("<label class='form-check-label' for='showOutCheck");
    page += i;
    page += F("'>Show Outcoming</label>");
    page += F("</div>");
    page += F("</div>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");

  page += F("<div class='container'>");
  page += F("<div class='row mt-3'>");
  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F(" Apply</button>");
  page += F(" ");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  page += F("</form>");

  get_footer_page();

  if (trim_page(start, len, true)) return;
}

void get_sequences_page(unsigned int start, unsigned int len) {

  const byte s = constrain(uisequence.toInt(), 1, SEQUENCES);

  if (get_top_page(6, start, len)) return;

  page += F("<div class='btn-group mb-3'>");
  for (unsigned int i = 1; i <= SEQUENCES; i++) {
    page += F("<form method='get'><button type='button submit' class='btn");
    page += (uisequence == String(i) ? F(" btn-primary") : F(""));
    page += F("' name='sequence' value='");
    page += i;
    page += F("'>");
    page += i;
    page += F("</button></form>");
  }
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<form method='post'>");

  page += F("<div class='card mb-3'>");
  page += F("<div class='card-header'>");
  page += F("<div class='row'>");
  page += F("<div class='col-auto me-auto'>");
  page += F("<h5 class='mb-0'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-camera-reels' viewBox='0 0 20 20'>");
  page += F("<path d='M6 3a3 3 0 1 1-6 0 3 3 0 0 1 6 0zM1 3a2 2 0 1 0 4 0 2 2 0 0 0-4 0z'/>");
  page += F("<path d='M9 6h.5a2 2 0 0 1 1.983 1.738l3.11-1.382A1 1 0 0 1 16 7.269v7.462a1 1 0 0 1-1.406.913l-3.111-1.382A2 2 0 0 1 9.5 16H2a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h7zm6 8.73V7.27l-3.5 1.555v4.35l3.5 1.556zM1 8v6a1 1 0 0 0 1 1h7.5a1 1 0 0 0 1-1V8a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1z'/>");
  page += F("<path d='M9 6a3 3 0 1 0 0-6 3 3 0 0 0 0 6zM7 3a2 2 0 1 1 4 0 2 2 0 0 1-4 0z'/>");
  page += F("</svg>");
  page += F(" Sequence ");
  page += s;
  page += F("</h5>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F(" Apply</button>");
  page += F(" ");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  page += F("<div class='card-body'>");

  if (trim_page(start, len)) return;

  for (unsigned int i = 1; i <= STEPS; i++) {
    page += F("<div class='d-grid mb-1'>");
    page += F("<div class='row g-1'>");
    page += F("<div class='col'>");
    page += F("<div class='form-floating'>");
    page += F("<select class='form-select' id='messageSelect");
    page += i;
    page += F("' onchange='init_line(");
    page += i;
    page += F(")' name='message");
    page += i;
    page += F("'>");
    page += F("<option value='");
    page += PED_EMPTY;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_NONE) page += F(" selected");
    page += F("></option>");
    page += F("<option value='");
    page += PED_PROGRAM_CHANGE;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_PROGRAM_CHANGE) page += F(" selected");
    page += F(">Program Change</option>");
    page += F("<option value='");
    page += PED_CONTROL_CHANGE;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_CONTROL_CHANGE) page += F(" selected");
    page += F(">Control Change</option>");
    page += F("<option value='");
    page += PED_CONTROL_CHANGE_SNAP;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_CONTROL_CHANGE_SNAP) page += F(" selected");
    page += F(">Control Change Snap</option>");
    page += F("<option value='");
    page += PED_NOTE_ON;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_NOTE_ON) page += F(" selected");
    page += F(">Note On</option>");
    page += F("<option value='");
    page += PED_NOTE_OFF;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_NOTE_OFF) page += F(" selected");
    page += F(">Note Off</option>");

    if (trim_page(start, len)) return;

    page += F("<option value='");
    page += PED_BANK_SELECT_INC;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_BANK_SELECT_INC) page += F(" selected");
    page += F(">Bank Select+</option>");
    page += F("<option value='");
    page += PED_BANK_SELECT_DEC;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_BANK_SELECT_DEC) page += F(" selected");
    page += F(">Bank Select-</option>");
    page += F("<option value='");
    page += PED_PROGRAM_CHANGE_INC;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_PROGRAM_CHANGE_INC) page += F(" selected");
    page += F(">Program Change+</option>");
    page += F("<option value='");
    page += PED_PROGRAM_CHANGE_DEC;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_PROGRAM_CHANGE_DEC) page += F(" selected");
    page += F(">Program Change-</option>");
    page += F("<option value='");
    page += PED_PITCH_BEND;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_PITCH_BEND) page += F(" selected");
    page += F(">Pitch Bend</option>");
    page += F("<option value='");
    page += PED_CHANNEL_PRESSURE;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_CHANNEL_PRESSURE) page += F(" selected");
    page += F(">Channel Pressure</option>");

    if (trim_page(start, len)) return;

    page += F("<option value='");
    page += PED_MIDI_START;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_MIDI_START) page += F(" selected");
    page += F(">Start</option>");
    page += F("<option value='");
    page += PED_MIDI_STOP;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_MIDI_STOP) page += F(" selected");
    page += F(">Stop</option>");
    page += F("<option value='");
    page += PED_MIDI_CONTINUE;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_MIDI_CONTINUE) page += F(" selected");
    page += F(">Continue</option>");
    page += F("<option value='");
    page += PED_SEQUENCE;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_SEQUENCE) page += F(" selected");
    page += F(">Sequence</option>");
    page += F("<option value='");
    page += PED_ACTION_BANK;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_ACTION_BANK) page += F(" selected");
    page += F(">Set Bank</option>");
    page += F("<option value='");
    page += PED_ACTION_LED_COLOR;
    page += F("'");
    if (sequences[s-1][i-1].midiMessage == PED_ACTION_LED_COLOR) page += F(" selected");
    page += F(">Set Led Color</option>");
    page += F("</select>");
    page += F("<label for='messageSelect");
    page += i;
    page += F("'>Send</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='col'>");
    page += F("<div class='form-floating' hidden id='channelDiv");
    page += i;
    page += F("'>");
    page += F("<select class='form-select' id='channelSelect");
    page += i;
    page += F("' name='channel");
    page += i;
    page += F("'>");
    for (unsigned int c = 0; c <= 17; c++) {
      page += F("<option value='");
      page += c;
      page += F("'");
      if (sequences[s-1][i-1].midiMessage == PED_ACTION_LED_COLOR) {
        if (c == 0) page += F(" selected");
      }
      else if (sequences[s-1][i-1].midiChannel == c) page += F(" selected");
      page += F(">");
      switch (c) {
        case 0:
          page += "None";
          break;
        case 17:
          page += "All";
          break;
        default:  // 1..16
          page += c;
          break;
      }
      page += F("</option>");

      if (trim_page(start, len)) return;
    }
    page += F("</select>");
    page += F("<label for='channelSelect");
    page += i;
    page += F("' id='channelLabel");
    page += i;
    page += F("'>Channel</label>");
    page += F("</div>");
    page += F("<div class='form-floating' hidden id='sequenceDiv");
    page += i;
    page += F("'>");
    page += F("<select class='form-select' id='sequencelSelect");
    page += i;
    page += F("' name='sequence");
    page += i;
    page += F("'>");
    for (unsigned int c = 1; c <= SEQUENCES; c++) {
      page += F("<option value='");
      page += c;
      page += F("'");
      if (sequences[s-1][i-1].midiChannel == c - 1) page += F(" selected");
      page += F(">");
      page += c;
      page += F("</option>");
      if (trim_page(start, len)) return;
    }
    page += F("</select>");
    page += F("<label for='sequenceSelect");
    page += i;
    page += F("' id='sequenceLabel");
    page += i;
    page += F("'>Sequence</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='col'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='number' class='form-control' id='codeInput");
    page += i;
    page += F("' name='code");
    page += i;
    page += F("' min='0' max='127' value='");
    if (sequences[s-1][i-1].midiMessage == PED_ACTION_LED_COLOR)
      page += 0;
    else
      page += sequences[s-1][i-1].midiCode;
    page += F("'>");
    page += F("<label for='codeInput");
    page += i;
    page += F("' id='codeLabel");
    page += i;
    page += F("'>Code</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='col'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='number' class='form-control' id='valueInput");
    page += i;
    page += F("' name='value");
    page += i;
    page += F("' min='0' max='127' value='");
    if (sequences[s-1][i-1].midiMessage == PED_ACTION_LED_COLOR)
      page += 0;
    else
      page += sequences[s-1][i-1].midiValue;
    page += F("'>");
    page += F("<label for='valueInput");
    page += i;
    page += F("' id='valueLabel");
    page += i;
    page += F("'>Value</label>");
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='col'>");
    page += F("<div class='form-floating'>");
    page += F("<select class='form-select' id='ledSelect");
    page += i;
    page += F("' name='led");
    page += i;
    page += F("'>");
    page += F("<option value='0'");
    if (sequences[s-1][i-1].led == LEDS) page += F(" selected");
    page += F("></option>");
    page += F("<option value='255'");
    if (sequences[s-1][i-1].led == 255) page += F(" selected");
    page += F(">Default</option>");
    for (unsigned int l = 1; l <= LEDS; l++) {
      page += F("<option value='");
      page += l;
      page += F("'");
      if (sequences[s-1][i-1].led == l-1) page += F(" selected");
      page += F(">");
      page += l;
      page += F("</option>");

      if (trim_page(start, len)) return;
    }
    page += F("</select>");
    page += F("<label for='ledSelect");
    page += i;
    page += F("' id='ledLabel");
    page += i;
    page += F("'>Led</label>");
    /*
    page += F("<select class='form-select' id='ledSelect");
    page += i;
    page += F("' name='led");
    page += i;
    page += F("'>");
    for (unsigned int l = 0; l <= LEDS; l++) {
      page += F("<option value='");
      page += (l+1);
      page += F("'");
      if (sequences[s-1][i-1].led == l) page += F(" selected");
      page += F(">");
      if (l != LEDS) page += (l+1);
      page += F("</option>");
    }
    page += F("</select>");
    page += F("<label for='ledSelect");
    page += i;
    page += F("'>Led</label>");
    */
    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;

    page += F("<div class='col'>");
    page += F("<div class='form-floating'>");
    page += F("<input type='color' class='form-control' id='colorInput");
    page += i;
    page += F("' name='color");
    page += i;
    page += F("' value='");
    char color[8] = "#000000";
    sprintf(color, "#%06X", sequences[s-1][i-1].color & 0xFFFFFF);
    page += color;
    page += F("'>");
    page += F("<label for='colorInput");
    page += i;
    page += F("'>Color</label>");
    page += F("</div>");
    page += F("</div>");

    page += F("</div>");
    page += F("</div>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");
  /*
  page += F("<div class='row'>");
  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F(" Apply</button>");
  page += F(" ");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("</div>");
  */
  page += F("</form>");

  if (trim_page(start, len)) return;

  page += F("<script>");
  page += F("document.addEventListener('DOMContentLoaded', init_lines());");
  page += F("function init_lines() {");
  for (byte s = 1; s <= STEPS; s++) {
    page += F("  init_line(");
    page += s;
    page += F(");");
  }
  page += F("};");

  if (trim_page(start, len)) return;

  page += F("function init_line(i) {"
            "   document.getElementById('channelLabel'  + i).textContent = 'Channel';"
            "   document.getElementById('channelDiv'    + i).removeAttribute('hidden');"
            "   document.getElementById('sequenceDiv'   + i).setAttribute('hidden', 'hidden');"
            "   document.getElementById('codeLabel'     + i).textContent = 'Code';"
            "   document.getElementById('valueLabel'    + i).textContent = 'Value';"
            "   document.getElementById('channelSelect' + i).disabled = false;"
            "   document.getElementById('codeInput'     + i).disabled = false;"
            "   document.getElementById('valueInput'    + i).disabled = false;"
            "   document.getElementById('ledSelect'     + i).disabled = false;"
            "   document.getElementById('colorInput'    + i).disabled = false;"
            "   switch (document.getElementById('messageSelect' + i).options[document.getElementById('messageSelect' + i).selectedIndex].text) {"
            "     case 'Program Change':"
            "     case 'Program Change+':"
            "     case 'Program Change-':"
            "       document.getElementById('codeLabel'     + i).textContent = 'PC';"
            "       document.getElementById('valueInput'    + i).disabled = true;"
            "       break;"
            "     case 'Control Change':"
            "       document.getElementById('codeLabel'     + i).textContent = 'CC';"
            "       break;"
            "     case 'Note On':"
            "     case 'Note Off':"
            "       document.getElementById('codeLabel'     + i).textContent = 'Note';"
            "       document.getElementById('valueLabel'    + i).textContent = 'Velocity';"
            "       break;");

  if (trim_page(start, len)) return;

  page += F("     case 'Bank Select+':"
            "     case 'Bank Select-':"
            "       document.getElementById('codeLabel'     + i).textContent = 'MSB';"
            "       document.getElementById('valueLabel'    + i).textContent = 'LSB';"
            "       break;"
            "     case 'Pitch Bend':"
            "     case 'Channel Pressure':"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       break;"
            "     case 'Sequence':"
            "       document.getElementById('channelDiv'    + i).setAttribute('hidden', 'hidden');"
            "       document.getElementById('sequenceDiv'   + i).removeAttribute('hidden');"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('valueInput'    + i).disabled = true;"
            "       document.getElementById('ledSelect'     + i).disabled = true;"
            "       document.getElementById('colorInput'    + i).disabled = true;"
            "       break;"
            "     case 'Set Bank':"
            "       document.getElementById('channelSelect' + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('valueLabel'    + i).textContent = 'Bank [0-20]';"
            "       break;"
            "     case 'Set Led Color':"
            "       document.getElementById('channelSelect' + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('valueInput'    + i).disabled = true;"
            "       break;"
            "     case 'Start':"
            "     case 'Stop':"
            "     case 'Continue':"
            "     case '':"
            "     default:"
            "       document.getElementById('channelSelect' + i).disabled = true;"
            "       document.getElementById('codeInput'     + i).disabled = true;"
            "       document.getElementById('valueInput'    + i).disabled = true;"
            "       document.getElementById('ledSelect'     + i).disabled = true;"
            "       document.getElementById('colorInput'    + i).disabled = true;"
            "       break;"
            "   }"
            "};");
  page += F("</script>");

  get_footer_page();

  if (trim_page(start, len, true)) return;
}

void get_options_page(unsigned int start, unsigned int len) {

  const String  bootswatch[] = { "bootstrap",
                                 "cerulean",
                                 "cosmo",
                                 "cyborg",
                                 "darkly",
                                 "flatly",
                                 "litera",
                                 "journal",
                                 "lumen",
                                 "lux",
                                 "materia",
                                 "minty",
                                 "pulse",
                                 "sandstone",
                                 "simplex",
                                 "sketchy",
                                 "slate",
                                 "solar",
                                 "spacelab",
                                 "superhero",
                                 "united",
                                 "yeti"};

  if (get_top_page(7, start, len)) return;

  page += F("<form method='post'>");

  page += F("<div class='row'>");
  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-cpu' viewBox='0 0 20 20'>");
  page += F("<path d='M5 0a.5.5 0 0 1 .5.5V2h1V.5a.5.5 0 0 1 1 0V2h1V.5a.5.5 0 0 1 1 0V2h1V.5a.5.5 0 0 1 1 0V2A2.5 2.5 0 0 1 14 4.5h1.5a.5.5 0 0 1 0 1H14v1h1.5a.5.5 0 0 1 0 1H14v1h1.5a.5.5 0 0 1 0 1H14v1h1.5a.5.5 0 0 1 0 1H14a2.5 2.5 0 0 1-2.5 2.5v1.5a.5.5 0 0 1-1 0V14h-1v1.5a.5.5 0 0 1-1 0V14h-1v1.5a.5.5 0 0 1-1 0V14h-1v1.5a.5.5 0 0 1-1 0V14A2.5 2.5 0 0 1 2 11.5H.5a.5.5 0 0 1 0-1H2v-1H.5a.5.5 0 0 1 0-1H2v-1H.5a.5.5 0 0 1 0-1H2v-1H.5a.5.5 0 0 1 0-1H2A2.5 2.5 0 0 1 4.5 2V.5A.5.5 0 0 1 5 0zm-.5 3A1.5 1.5 0 0 0 3 4.5v7A1.5 1.5 0 0 0 4.5 13h7a1.5 1.5 0 0 0 1.5-1.5v-7A1.5 1.5 0 0 0 11.5 3h-7zM5 6.5A1.5 1.5 0 0 1 6.5 5h3A1.5 1.5 0 0 1 11 6.5v3A1.5 1.5 0 0 1 9.5 11h-3A1.5 1.5 0 0 1 5 9.5v-3zM6.5 6a.5.5 0 0 0-.5.5v3a.5.5 0 0 0 .5.5h3a.5.5 0 0 0 .5-.5v-3a.5.5 0 0 0-.5-.5h-3z'/>");
  page += F("</svg>");
  page += F(" Device</h5>");
  page += F("<div class='card-body'>");

  if (trim_page(start, len)) return;

  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='32' id='devicename' name='mdnsdevicename' value='");
  page += host;
  page += F("'>");
  page += F("<label for='devicename'>Name</label>");
  page += F("</div>");
  page += F("<small id='devicenameHelpBlock' class='form-text text-muted'>");
  page += F("Each device must have a different name. Enter the device name without .local. Web UI will be available at http://<i>device_name</i>.local<br>");
  page += F("Pedalino will be restarted if you change it.");
  page += F("</small>");
  page += F("<div class='row'>");
  page += F("<div class='col-auto me-auto'>");
  page += F("</div>");
  page += F("<div class='col-auto'>");
  page += F("<a href='/update' class='btn btn-primary btn-sm' role='button' aria-pressed='true'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-cloud-arrow-down' viewBox='0 0 16 16'>");
  page += F("<path fill-rule='evenodd' d='M7.646 10.854a.5.5 0 0 0 .708 0l2-2a.5.5 0 0 0-.708-.708L8.5 9.293V5.5a.5.5 0 0 0-1 0v3.793L6.354 8.146a.5.5 0 1 0-.708.708l2 2z'/>");
  page += F("<path d='M4.406 3.342A5.53 5.53 0 0 1 8 2c2.69 0 4.923 2 5.166 4.579C14.758 6.804 16 8.137 16 9.773 16 11.569 14.502 13 12.687 13H3.781C1.708 13 0 11.366 0 9.318c0-1.763 1.266-3.223 2.942-3.593.143-.863.698-1.723 1.464-2.383zm.653.757c-.757.653-1.153 1.44-1.153 2.056v.448l-.445.049C2.064 6.805 1 7.952 1 9.318 1 10.785 2.23 12 3.781 12h8.906C13.98 12 15 10.988 15 9.773c0-1.216-1.02-2.228-2.313-2.228h-.5v-.5C12.188 4.825 10.328 3 8 3a4.53 4.53 0 0 0-2.941 1.1z'/>");
  page += F("</svg>");
  page += F(" Check for Updates</a>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-power' viewBox='0 0 20 20'>");
  page += F("<path d='M7.5 1v7h1V1h-1z'/>");
  page += F("<path d='M3 8.812a4.999 4.999 0 0 1 2.578-4.375l-.485-.874A6 6 0 1 0 11 3.616l-.501.865A5 5 0 1 1 3 8.812z'/>");
  page += F("</svg>");
  page += F(" Boot Mode</h5>");
  page += F("<div class='card-body'>");
#ifdef WIFI
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='bootModeWifi' name='bootmodewifi'");
  if (bootMode == PED_BOOT_NORMAL ||
      bootMode == PED_BOOT_WIFI   ||
      bootMode == PED_BOOT_AP     ||
      bootMode == PED_BOOT_AP_NO_BLE) page += F(" checked");
  page += F(">");
  page += F("<label class='form-check-label' for='bootModeWifi'>WiFi</label>");
  page += F("</div>");
  page += F("<small id='bootModeWifiHelpBlock' class='form-text text-muted'>");
  page += F("RTP-MIDI, ipMIDI, OSC and web UI require WiFi.");
  page += F("</small>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='bootModeAP' name='bootmodeap'");
  if (bootMode == PED_BOOT_AP ||
      bootMode == PED_BOOT_AP_NO_BLE) page += F(" checked");
  page += F(">");
  page += F("<label class='form-check-label' for='bootModeAP'>Access Point</label>");
  page += F("</div>");
  page += F("<small id='bootModeAPHelpBlock' class='form-text text-muted'>");
  page += F("To enable AP Mode enable WiFi too.");
  page += F("</small>");
#endif
#ifdef BLE
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='bootModeBLE' name='bootmodeble'");
  if (bootMode == PED_BOOT_NORMAL ||
      bootMode == PED_BOOT_BLE    ||
      bootMode == PED_BOOT_AP) page += F(" checked");
  page += F(">");
#ifdef BLECLIENT
  page += F("<label class='form-check-label' for='bootModeBLE'>BLE Client</label>");
#else
  page += F("<label class='form-check-label' for='bootModeBLE'>BLE Server</label>");
#endif
  page += F("</div>");
  page += F("<small id='bootModeBLEHelpBlock' class='form-text text-muted'>");
  page += F("BLE MIDI requires BLE.");
  page += F("</small>");
#endif

  if (trim_page(start, len)) return;

#ifdef BLECLIENT
  page += F("<br><br>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='20' id='bleserver' name='bleServer' value='");
  page += bleServer;
  page += F("'>");
  page += F("<label for='bleserver'>BLE Server Name/Address</label>");
  page += F("</div>");
  page += F("<small id='bootModeBLEServerHelpBlock' class='form-text text-muted'>");
  page += F("Enter the device name of another Pedalino to connect via BLE.<br>");
  page += F("Leave blank to connect to first available BLE MIDI server with MIDI characteristic. ");
  page += F("A BLE address (i.e. f2:c1:d9:36:e7:6b) is accepted too.");
  page += F("</small>");
#endif
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='row'>");
  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-wifi' viewBox='0 0 20 20'>");
  page += F("<path d='M15.385 6.115a.485.485 0 0 0-.048-.736A12.443 12.443 0 0 0 8 3 12.44 12.44 0 0 0 .663 5.379a.485.485 0 0 0-.048.736.518.518 0 0 0 .668.05A11.448 11.448 0 0 1 8 4c2.507 0 4.827.802 6.717 2.164.204.148.489.13.668-.049z'/>");
  page += F("<path d='M13.229 8.271c.216-.216.194-.578-.063-.745A9.456 9.456 0 0 0 8 6c-1.905 0-3.68.56-5.166 1.526a.48.48 0 0 0-.063.745.525.525 0 0 0 .652.065A8.46 8.46 0 0 1 8 7a8.46 8.46 0 0 1 4.577 1.336c.205.132.48.108.652-.065zm-2.183 2.183c.226-.226.185-.605-.1-.75A6.472 6.472 0 0 0 8 9c-1.06 0-2.062.254-2.946.704-.285.145-.326.524-.1.75l.015.015c.16.16.408.19.611.09A5.478 5.478 0 0 1 8 10c.868 0 1.69.201 2.42.56.203.1.45.07.611-.091l.015-.015zM9.06 12.44c.196-.196.198-.52-.04-.66A1.99 1.99 0 0 0 8 11.5a1.99 1.99 0 0 0-1.02.28c-.238.14-.236.464-.04.66l.706.706a.5.5 0 0 0 .708 0l.707-.707z'/>");
  page += F("</svg>");
  page += F(" WiFi Network</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row g-1'>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<select class='form-select' id='wifissid' name='wifiSSID'>");
  networks = WiFi.scanComplete();
  if (networks > 0) {
    for (int i = 0; i < networks; i++) {
      if (!WiFi.SSID(i).isEmpty()) {
        page += F("<option value='");
        page += WiFi.SSID(i);
        page += F("'");
        if (wifiSSID == WiFi.SSID(i)) page += F(" selected");
        page += F(">");
        page += WiFi.SSID(i);
        page += F("</option>");
      }

      if (trim_page(start, len)) return;
    }
  }
  page += F("</select>");
  page += F("<label for='wifissid'>SSID</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='password' maxlength='32' id='wifipassword' name='wifiPassword' value='");
  page += wifiPassword;
  page += F("'>");
  page += F("<label for='wifipassword'>Password</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<small class='form-text text-muted'>");
  page += F("Connect to a wifi network using SSID and password. WiFi networks list is updated only on boot.<br>");
  page += F("Pedalino will be restarted if it is connected to a WiFi network and you change them.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-broadcast-pin' viewBox='0 0 20 20'>");
  page += F("<path d='M3.05 3.05a7 7 0 0 0 0 9.9.5.5 0 0 1-.707.707 8 8 0 0 1 0-11.314.5.5 0 0 1 .707.707zm2.122 2.122a4 4 0 0 0 0 5.656.5.5 0 0 1-.708.708 5 5 0 0 1 0-7.072.5.5 0 0 1 .708.708zm5.656-.708a.5.5 0 0 1 .708 0 5 5 0 0 1 0 7.072.5.5 0 1 1-.708-.708 4 4 0 0 0 0-5.656.5.5 0 0 1 0-.708zm2.122-2.12a.5.5 0 0 1 .707 0 8 8 0 0 1 0 11.313.5.5 0 0 1-.707-.707 7 7 0 0 0 0-9.9.5.5 0 0 1 0-.707zM6 8a2 2 0 1 1 2.5 1.937V15.5a.5.5 0 0 1-1 0V9.937A2 2 0 0 1 6 8z'/>");
  page += F("</svg>");
  page += F(" AP Mode</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row g-1'>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='32' id='ssidsoftap' name='ssidSoftAP' value='");
  page += ssidSoftAP;
  page += F("'>");
  page += F("<label for='wifissidap'>SSID</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='password' maxlength='32' id='passwordsoftap' name='passwordSoftAP' value='");
  page += passwordSoftAP;
  page += F("'>");
  page += F("<label for='passwordsoftap'>Password</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<small class='form-text text-muted'>");
  page += F("Access Point SSID and password.<br>");
  page += F("Pedalino will be restarted if it is in AP mode and you change them.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='row'>");
  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-display' viewBox='0 0 20 20'>");
  page += F("<path d='M0 4s0-2 2-2h12s2 0 2 2v6s0 2-2 2h-4c0 .667.083 1.167.25 1.5H11a.5.5 0 0 1 0 1H5a.5.5 0 0 1 0-1h.75c.167-.333.25-.833.25-1.5H2s-2 0-2-2V4zm1.398-.855a.758.758 0 0 0-.254.302A1.46 1.46 0 0 0 1 4.01V10c0 .325.078.502.145.602.07.105.17.188.302.254a1.464 1.464 0 0 0 .538.143L2.01 11H14c.325 0 .502-.078.602-.145a.758.758 0 0 0 .254-.302 1.464 1.464 0 0 0 .143-.538L15 9.99V4c0-.325-.078-.502-.145-.602a.757.757 0 0 0-.302-.254A1.46 1.46 0 0 0 13.99 3H2c-.325 0-.502.078-.602.145z'/>");
  page += F("</svg>");
  page += F(" Web UI Theme</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='form-floating'>");
  page += F("<select class='form-select' id='bootstraptheme' name='theme'>");
  for (unsigned int i = 0; i < 22; i++) {
    page += F("<option value='");
    page += bootswatch[i];
    page += F("'");
    if (theme == bootswatch[i]) page += F(" selected");
    page += F(">");
    page += bootswatch[i];
    page += F("</option>");

    if (trim_page(start, len)) return;
  }
  page += F("</select>");
  page += F("<label for='theme'>Theme</label>");
  page += F("</div>");
  page += F("<small id='bootstrapthemeHelpBlock' class='form-text text-muted'>");
  page += F("Changing default theme require internet connection because themes are served via a CDN network. Only 'bootstrap' theme has been stored into Pedalino flash memory.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-key' viewBox='0 0 20 20'>");
  page += F("<path d='M0 8a4 4 0 0 1 7.465-2H14a.5.5 0 0 1 .354.146l1.5 1.5a.5.5 0 0 1 0 .708l-1.5 1.5a.5.5 0 0 1-.708 0L13 9.207l-.646.647a.5.5 0 0 1-.708 0L11 9.207l-.646.647a.5.5 0 0 1-.708 0L9 9.207l-.646.647A.5.5 0 0 1 8 10h-.535A4 4 0 0 1 0 8zm4-3a3 3 0 1 0 2.712 4.285A.5.5 0 0 1 7.163 9h.63l.853-.854a.5.5 0 0 1 .708 0l.646.647.646-.647a.5.5 0 0 1 .708 0l.646.647.646-.647a.5.5 0 0 1 .708 0l.646.647.793-.793-1-1h-6.63a.5.5 0 0 1-.451-.285A3 3 0 0 0 4 5z'/>");
  page += F("<path d='M4 8a1 1 0 1 1-2 0 1 1 0 0 1 2 0z'/>");
  page += F("</svg>");
  page += F(" Web UI Login</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row g-1'>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control form-control-sm' type='text' maxlength='32' id='httpusername' name='httpUsername' value='");
  page += httpUsername;
  page += F("'>");
  page += F("<label for='httpusername'>Username</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control form-control-sm' type='password' maxlength='32' id='httppassword' name='httpPassword' value='");
  page += httpPassword;
  page += F("'>");
  page += F("<label for='httppassword'>Password</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<small class='form-text text-muted'>");
  page += F("Web UI administrator username and password. Leave username blank for no login request.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='row'>");
  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100 mb-3'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-toggle-on' viewBox='0 0 20 20'>");
  page += F("<path d='M5 3a5 5 0 0 0 0 10h6a5 5 0 0 0 0-10H5zm6 9a4 4 0 1 1 0-8 4 4 0 0 1 0 8z'/>");
  page += F("</svg>");
  page += F(" Momentary Switches</h5>");
  page += F("<div class='card-body'>");

  if (trim_page(start, len)) return;

  page += F("<div class='row g-1'>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='32' id='debounceInterval' name='debounceinterval' value='");
  page += debounceInterval;
  page += F("'>");
  page += F("<label for='debounceInterval'>Debounce Interval</label>");
  page += F("</div>");
  page += F("<small id='debounceTimeModeHelpBlock' class='form-text text-muted'>");
  page += F("Interval in milliseconds to consider valid and stable a button push. Default value is 5 ms.<br>");
  page += F("When a mechanical switch is pushed in an electrical circuit it makes a series of short contacts for a limited interval of time. ");
  page += F("Increase the value if spurious pressure or release occurs.");
  page += F("</small>");
  page += F("</div>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='32' id='simultaneousGapTime' name='simultaneousgaptime' value='");
  page += simultaneousGapTime;
  page += F("'>");
  page += F("<label for='simultaneousGapTime'>Simultaneous Gap Time</label>");
  page += F("</div>");
  page += F("<small id='simultaneousTimeModeHelpBlock' class='form-text text-muted'>");
  page += F("Two buttons are considered simultaneous pressed if the second is pressed within a certain gap from the first.<br>");
  page += F("Default value is 50 ms.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='row g-1'>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='32' id='pressTime' name='presstime' value='");
  page += pressTime;
  page += F("'>");
  page += F("<label for='pressTime'>Press Time</label>");
  page += F("</div>");
  page += F("<small id='pressTimeModeHelpBlock' class='form-text text-muted'>");
  page += F("Switch press time in milliseconds. Default value is 200.");
  page += F("</small>");
  page += F("</div>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='32' id='doublePressTime' name='doublepresstime' value='");
  page += doublePressTime;
  page += F("'>");
  page += F("<label for='doublePressTime'>Double Press Time</label>");
  page += F("</div>");
  page += F("<small id='doublePressTimeModeHelpBlock' class='form-text text-muted'>");
  page += F("Set double press detection time between each press time in milliseconds. Default value is 400 ms.<br>");
  page += F("A double press is detected if the switch is released and depressed within this time, measured from when the first press is detected.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='row g-1'>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='32' id='longPressTime' name='longpresstime' value='");
  page += longPressTime;
  page += F("'>");
  page += F("<label for='longPressTime'>Long Press Time</label>");
  page += F("</div>");
  page += F("<small id='longPressTimeModeHelpBlock' class='form-text text-muted'>");
  page += F("Set the long press time in milliseconds after which a continuous press and release is deemed a long press, measured from when the first press is detected.<br>");
  page += F("Default value is 500 ms.");
  page += F("</small>");
  page += F("</div>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='32' id='repeatPressTime' name='repeatpresstime' value='");
  page += repeatPressTime;
  page += F("'>");
  page += F("<label for='repeatPressTime'>Repeat Press Time</label>");
  page += F("</div>");
  page += F("<small id='repeatPressTimeModeHelpBlock' class='form-text text-muted'>");
  page += F("Set the repeat time in milliseconds after which a continuous press and hold is treated as a stream of repeated presses, measured from when the first press is detected.<br>");
  page += F("Default value is 1000 ms.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card mb-3'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-ladder' viewBox='0 0 20 20'>");
  page += F("<path d='M4.5 1a.5.5 0 0 1 .5.5V2h6v-.5a.5.5 0 0 1 1 0v14a.5.5 0 0 1-1 0V15H5v.5a.5.5 0 0 1-1 0v-14a.5.5 0 0 1 .5-.5zM5 14h6v-2H5v2zm0-3h6V9H5v2zm0-3h6V6H5v2zm0-3h6V3H5v2z'/>");
  page += F("</svg>");
  page += F(" Resistor Ladder Network</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='d-grid gap-1'>");
  for (byte i = 1; i <= LADDER_STEPS + 1; i++) {
    if ((i - 1) % 4 == 0) page += F("<div class='row g-1'>");
    page += F("<div class='w-25'>");
    page += F("<div class='form-floating'>");
    page += F("<input class='form-control form-control-sm' type='number' id='threshold");
    page += i;
    page += F("' name='threshold");
    page += i;
    page += F("' min='0' max='");
    page += (ADC_RESOLUTION - 1);
    page += F("' value='");
    page += ladderLevels[i-1];
    page += F("'>");
    page += F("<label for='threshold");
    page += i;
    page += F("'>Level ");
    page += i;
    page += F("</label>");
    page += F("</div>");
    page += F("</div>");
    if (i % 4 == 0 || i == LADDER_STEPS + 1) page += F("</div>");

    if (trim_page(start, len)) return;
  }
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='card mb-3'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-joystick' viewBox='0 0 20 20'>");
  page += F("<path d='M10 2a2 2 0 0 1-1.5 1.937v5.087c.863.083 1.5.377 1.5.726 0 .414-.895.75-2 .75s-2-.336-2-.75c0-.35.637-.643 1.5-.726V3.937A2 2 0 1 1 10 2z'/>");
  page += F("<path d='M0 9.665v1.717a1 1 0 0 0 .553.894l6.553 3.277a2 2 0 0 0 1.788 0l6.553-3.277a1 1 0 0 0 .553-.894V9.665c0-.1-.06-.19-.152-.23L9.5 6.715v.993l5.227 2.178a.125.125 0 0 1 .001.23l-5.94 2.546a2 2 0 0 1-1.576 0l-5.94-2.546a.125.125 0 0 1 .001-.23L6.5 7.708l-.013-.988L.152 9.435a.25.25 0 0 0-.152.23z'/>");
  page += F("</svg>");
  page += F(" Encoders</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='form-floating'>");
  page += F("<select class='form-select' name='encodersensitivity'>");
  for (unsigned int s = 1; s <= 10; s++) {
    page += F("<option value='");
    page += s;
    page += F("'");
    if (encoderSensitivity == s) page += F(" selected");
    page += F(">");
    page += s;
    page += F("</option>");

    if (trim_page(start, len)) return;
  }
  page += F("</select>");
  page += F("<label for='encodersensitivity'>Encoder Sensitivity</label>");
  page += F("</div>");
  page += F("<small id='encoderSensitivityHelpBlock' class='form-text text-muted'>");
  page += F("Default value is 5.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='card'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-display' viewBox='0 0 20 20'>");
  page += F("<path d='M0 4s0-2 2-2h12s2 0 2 2v6s0 2-2 2h-4c0 .667.083 1.167.25 1.5H11a.5.5 0 0 1 0 1H5a.5.5 0 0 1 0-1h.75c.167-.333.25-.833.25-1.5H2s-2 0-2-2V4zm1.398-.855a.758.758 0 0 0-.254.302A1.46 1.46 0 0 0 1 4.01V10c0 .325.078.502.145.602.07.105.17.188.302.254a1.464 1.464 0 0 0 .538.143L2.01 11H14c.325 0 .502-.078.602-.145a.758.758 0 0 0 .254-.302 1.464 1.464 0 0 0 .143-.538L15 9.99V4c0-.325-.078-.502-.145-.602a.757.757 0 0 0-.302-.254A1.46 1.46 0 0 0 13.99 3H2c-.325 0-.502.078-.602.145z'/>");
  page += F("</svg>");
  page += F(" Screen Saver</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control form-control-sm' type='number' id='screenSaverTimeout' name='screensavertimeout' min='0' max='1440' value='");
  page += screenSaverTimeout / 60000;
  page += F("'>");
  page += F("<label for='timeout'>Timeout</label>");
  page += F("<small id='screenSaverTimeoutHelpBlock' class='form-text text-muted'>");
  page += F("Power off display and leds when inactive for a certain amount of time in minutes.<br>Min 0 (never) - Default 60 (1 hour) - Max 1440 (24 hours).");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='row'>");
  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-sliders' viewBox='0 0 20 20'>");
  page += F("<path fill-rule='evenodd' d='M11.5 2a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3zM9.05 3a2.5 2.5 0 0 1 4.9 0H16v1h-2.05a2.5 2.5 0 0 1-4.9 0H0V3h9.05zM4.5 7a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3zM2.05 8a2.5 2.5 0 0 1 4.9 0H16v1H6.95a2.5 2.5 0 0 1-4.9 0H0V8h2.05zm9.45 4a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3zm-2.45 1a2.5 2.5 0 0 1 4.9 0H16v1h-2.05a2.5 2.5 0 0 1-4.9 0H0v-1h9.05z'/>");
  page += F("</svg>");
  page += F(" Additional Features</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='flipScreen' name='flipscreen'");
  if (flipScreen) page += F(" checked");
  page += F(">");
  page += F("<label class='form-check-label' for='flipScreen'>Flip Screen</label>");
  page += F("</div>");
  page += F("<small id='flidScreenHelpBlock' class='form-text text-muted'>");
  page += F("Vertical flip of screen.");
  page += F("</small>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='tapDanceMode' name='tapdancemode'");
  if (tapDanceMode) page += F(" checked");
  page += F(">");
  page += F("<label class='form-check-label' for='tapDanceMode'>Tap Dance Mode</label>");
  page += F("</div>");
  page += F("<small id='tapDanceModeHelpBlock' class='form-text text-muted'>");
  page += F("The first press of pedal X switch to bank X, the second press of any pedal send the MIDI event.");
  page += F("</small>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='repeatOnBankSwitch' name='repeatonbankswitch'");
  if (repeatOnBankSwitch) page += F(" checked");
  page += F(">");
  page += F("<label class='form-check-label' for='repeatOnBankSwitch'>Bank Switch Repeat</label>");
  page += F("</div>");
  page += F("<small id='repeatOnBankSwitchModeHelpBlock' class='form-text text-muted'>");
  page += F("On bank switch repeat the last MIDI message that was sent for that bank");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-brightness-high' viewBox='0 0 20 20'>");
  page += F("<path d='M8 11a3 3 0 1 1 0-6 3 3 0 0 1 0 6zm0 1a4 4 0 1 0 0-8 4 4 0 0 0 0 8zM8 0a.5.5 0 0 1 .5.5v2a.5.5 0 0 1-1 0v-2A.5.5 0 0 1 8 0zm0 13a.5.5 0 0 1 .5.5v2a.5.5 0 0 1-1 0v-2A.5.5 0 0 1 8 13zm8-5a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1 0-1h2a.5.5 0 0 1 .5.5zM3 8a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1 0-1h2A.5.5 0 0 1 3 8zm10.657-5.657a.5.5 0 0 1 0 .707l-1.414 1.415a.5.5 0 1 1-.707-.708l1.414-1.414a.5.5 0 0 1 .707 0zm-9.193 9.193a.5.5 0 0 1 0 .707L3.05 13.657a.5.5 0 0 1-.707-.707l1.414-1.414a.5.5 0 0 1 .707 0zm9.193 2.121a.5.5 0 0 1-.707 0l-1.414-1.414a.5.5 0 0 1 .707-.707l1.414 1.414a.5.5 0 0 1 0 .707zM4.464 4.465a.5.5 0 0 1-.707 0L2.343 3.05a.5.5 0 1 1 .707-.707l1.414 1.414a.5.5 0 0 1 0 .708z'/>");
  page += F("</svg>");
  page += F(" Leds</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row g-1'>");
  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control form-control-sm' type='number' id='leds' name='leds' min='0' max='254' value='");
  page += LEDS;
  page += F("'>");
  page += F("<label for='leds'>Leds</label>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='w-50'>");
  page += F("<div class='form-floating'>");
  page += F("<select class='form-select' name='rgborder'>");
  page += F("<option value='");
  page += RGB;
  page += F("'");
  if (rgbOrder == RGB) page += F(" selected");
  page += F(">RGB</option>");
  page += F("<option value='");
  page += RBG;
  page += F("'");
  if (rgbOrder == RBG) page += F(" selected");
  page += F(">RBG</option>");
  page += F("<option value='");
  page += GRB;
  page += F("'");
  if (rgbOrder == GRB) page += F(" selected");
  page += F(">GRB</option>");
  page += F("<option value='");
  page += GBR;
  page += F("'");
  if (rgbOrder == GBR) page += F(" selected");
  page += F(">GBR</option>");
  page += F("<option value='");
  page += BRG;
  page += F("'");
  if (rgbOrder == BRG) page += F(" selected");
  page += F(">BRG</option>");
  page += F("<option value='");
  page += BGR;
  page += F("'");
  if (rgbOrder == BGR) page += F(" selected");
  page += F(">BGR</option>");
  page += F("</select>");
  page += F("<label for='rgborder'>RGB Order</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<label for='brightnessOn'>Leds On Brightness: ");
  page += ledsOnBrightness;
  page += F("</label>");
  page += F("<input type='range' class='form-range' min='0' max='255' id='brightnessOn' name='ledsonbrightness'");
  page += F(" value='");
  page += ledsOnBrightness;
  page += F("' oninput='this.previousElementSibling.textContent = `Leds On Brightness: ` + this.value'>");
  page += F("<label for='brightnessOff'>Leds Off Brightness: ");
  page += ledsOffBrightness;
  page += F("</label>");
  page += F("<input type='range' class='form-range' min='0' max='255' id='brightnessOff' name='ledsoffbrightness'");
  page += F(" value='");
  page += ledsOffBrightness;
  page += F("' oninput='this.previousElementSibling.textContent = `Leds Off Brightness: ` + this.value'>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='row'>");
  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-ear' viewBox='0 0 20 20'>");
  page += F("<path d='M8.5 1A4.5 4.5 0 0 0 4 5.5v7.047a2.453 2.453 0 0 0 4.75.861l.512-1.363a5.553 5.553 0 0 1 .816-1.46l2.008-2.581A4.34 4.34 0 0 0 8.66 1H8.5ZM3 5.5A5.5 5.5 0 0 1 8.5 0h.16a5.34 5.34 0 0 1 4.215 8.618l-2.008 2.581a4.555 4.555 0 0 0-.67 1.197l-.51 1.363A3.453 3.453 0 0 1 3 12.547V5.5ZM8.5 4A1.5 1.5 0 0 0 7 5.5v2.695c.112-.06.223-.123.332-.192.327-.208.577-.44.72-.727a.5.5 0 1 1 .895.448c-.256.513-.673.865-1.079 1.123A8.538 8.538 0 0 1 7 9.313V11.5a.5.5 0 0 1-1 0v-6a2.5 2.5 0 0 1 5 0V6a.5.5 0 0 1-1 0v-.5A1.5 1.5 0 0 0 8.5 4Z'/>");
  page += F("</svg>");
  page += F(" OSC Local</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row g-1'>");
  page += F("<div class='w-75'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' readonly id='localip' name='localip' value='");
  page += host;
  page += F(".local - ");
  page += WiFi.localIP().toString();
  page += F("'>");
  page += F("<label for='wifissid'>Local Host - IP Address</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<div class='w-25'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='number' min='0' max='65535' id='oscLocalPort' name='osclocalport' value='");
  page += oscLocalPort;
  page += F("'>");
  page += F("<label for='osclocalport'>Port</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<small class='form-text text-muted'>");
  page += F("OSC local address and port to receive OSC messages.<br>");
  page += F("Broadcast OSC messages on configured port are received too.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-md-6 col-12 mb-3'>");
  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-megaphone' viewBox='0 0 20 20'>");
  page += F("<path d='M13 2.5a1.5 1.5 0 0 1 3 0v11a1.5 1.5 0 0 1-3 0v-.214c-2.162-1.241-4.49-1.843-6.912-2.083l.405 2.712A1 1 0 0 1 5.51 15.1h-.548a1 1 0 0 1-.916-.599l-1.85-3.49a68.14 68.14 0 0 0-.202-.003A2.014 2.014 0 0 1 0 9V7a2.02 2.02 0 0 1 1.992-2.013 74.663 74.663 0 0 0 2.483-.075c3.043-.154 6.148-.849 8.525-2.199V2.5zm1 0v11a.5.5 0 0 0 1 0v-11a.5.5 0 0 0-1 0zm-1 1.35c-2.344 1.205-5.209 1.842-8 2.033v4.233c.18.01.359.022.537.036 2.568.189 5.093.744 7.463 1.993V3.85zm-9 6.215v-4.13a95.09 95.09 0 0 1-1.992.052A1.02 1.02 0 0 0 1 7v2c0 .55.448 1.002 1.006 1.009A60.49 60.49 0 0 1 4 10.065zm-.657.975 1.609 3.037.01.024h.548l-.002-.014-.443-2.966a68.019 68.019 0 0 0-1.722-.082z'/>");
  page += F("</svg>");
  page += F(" OSC Remote</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row g-1'>");
  page += F("<div class='w-75'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='text' maxlength='32' id='oscRemoteHost' name='oscremotehost' value='");
  page += oscRemoteHost;
  page += F("'>");
  page += F("<label for='oscremotehost'>Remote Host/IP</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<div class='w-25'>");
  page += F("<div class='form-floating'>");
  page += F("<input class='form-control' type='number' min='0' max='65535' id='oscRemotePort' name='oscremoteport' value='");
  page += oscRemotePort;
  page += F("'>");
  page += F("<label for='oscremoteport'>Port</label>");
  page += F("</div>");
  page += F("</div>");
  page += F("<small class='form-text text-muted'>");
  page += F("OSC remote host name without .local, or IP address, and port to send OSC messages.<br>");
  page += F("If you don't know them use the subnet broadcast address 255.255.255.255.<br>");
  if (oscRemoteHost != oscRemoteIp.toString()) {
    page += oscRemoteHost;
    page += F(" resolved to ");
    page += oscRemoteIp.toString();
    page += F(".<br>");
  }
  page += F("If host name not found 255.255.255.255 is used.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='row'>");
  page += F("<div class='col-auto me-auto'>");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F(" Apply</button> ");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='factorydefault' class='btn btn-danger btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-wrench' viewBox='0 0 16 16'>");
  page += F("<path d='M.102 2.223A3.004 3.004 0 0 0 3.78 5.897l6.341 6.252A3.003 3.003 0 0 0 13 16a3 3 0 1 0-.851-5.878L5.897 3.781A3.004 3.004 0 0 0 2.223.1l2.141 2.142L4 4l-1.757.364L.102 2.223zm13.37 9.019l.528.026.287.445.445.287.026.529L15 13l-.242.471-.026.529-.445.287-.287.445-.529.026L13 15l-.471-.242-.529-.026-.287-.445-.445-.287-.026-.529L11 13l.242-.471.026-.529.445-.287.287-.445.529-.026L13 11l.471.242z'/>");
  page += F("</svg>");

  if (trim_page(start, len)) return;

  page += F(" Reset to Factory Default</button> ");
  page += F("<button type='submit' name='action' value='reboot' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-bootstrap-reboot' viewBox='0 0 16 16'>");
  page += F("<path d='M1.161 8a6.84 6.84 0 1 0 6.842-6.84.58.58 0 0 1 0-1.16 8 8 0 1 1-6.556 3.412l-.663-.577a.58.58 0 0 1 .227-.997l2.52-.69a.58.58 0 0 1 .728.633l-.332 2.592a.58.58 0 0 1-.956.364l-.643-.56A6.812 6.812 0 0 0 1.16 8z'>");
  page += F("<path d='M6.641 11.671V8.843h1.57l1.498 2.828h1.314L9.377 8.665c.897-.3 1.427-1.106 1.427-2.1 0-1.37-.943-2.246-2.456-2.246H5.5v7.352h1.141zm0-3.75V5.277h1.57c.881 0 1.416.499 1.416 1.32 0 .84-.504 1.324-1.386 1.324h-1.6z'>");
  page += F("</svg>");
  page += F(" Reboot</button> ");
  page += F("<button type='submit' name='action' value='poweroff' class='btn btn-primary btn-sm'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-power' viewBox='0 0 16 16'>");
  page += F("<path d='M7.5 1v7h1V1h-1z'/>");
  page += F("<path d='M3 8.812a4.999 4.999 0 0 1 2.578-4.375l-.485-.874A6 6 0 1 0 11 3.616l-.501.865A5 5 0 1 1 3 8.812z'/>");
  page += F("</svg>");
  page += F(" Power Off</button>");
  page += F("</div>");
  page += F("</div>");

  page += F("</form>");

  get_footer_page();

  if (trim_page(start, len, true)) return;
}


void get_configurations_page(unsigned int start, unsigned int len) {

  if (get_top_page(8, start, len)) return;

  page += F("<div class='row'>");

  page += F("<div class='col-lg-6 col-12 mb-3'>");

  page += F("<div class='card h-100'>");
  page += F("<form method='post'>");
  page += F("<div class='card-header'>");
  page += F("<div class='row'>");
  page += F("<div class='col-auto me-auto'>");
  page += F("<h5 class='mb-0'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-file-code' viewBox='0 0 20 20'>");
  page += F("<path d='M6.646 5.646a.5.5 0 1 1 .708.708L5.707 8l1.647 1.646a.5.5 0 0 1-.708.708l-2-2a.5.5 0 0 1 0-.708l2-2zm2.708 0a.5.5 0 1 0-.708.708L10.293 8 8.646 9.646a.5.5 0 0 0 .708.708l2-2a.5.5 0 0 0 0-.708l-2-2z'/>");
  page += F("<path d='M2 2a2 2 0 0 1 2-2h8a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V2zm10-1H4a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1z'/>");
  page += F("</svg>");
  page += F(" New Configuration");
  page += F("</h5>");
  page += F("</div>");
  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='new' class='btn btn-primary btn-sm' style='width: 100%;'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" New</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='card-body'>");
  page += F("<div class='row'>");
  page += F("<div class='col-8'>");
  page += F("<input class='form-control' type='text' maxlength='26' id='newconfiguration' name='newconfiguration' value=''>");
  page += F("<small id='newconfigurationHelpBlock' class='form-text text-muted'>");
  page += F("Type a name, select what to include and press 'New' to save current profile with a name. An existing configuration with the same name will be overridden without further notice.");
  page += F("</small><br>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-4'>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='actionsCheck1' name='actions1' checked>");
  page += F("<label class='form-check-label' for='actionsCheck1'>Actions</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='pedalsCheck1' name='pedals1' checked>");
  page += F("<label class='form-check-label' for='pedalsCheck1'>Pedals</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='controlsCheck1' name='controls1' checked>");
  page += F("<label class='form-check-label' for='controlsCheck1'>Controls</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='interfacesCheck1' name='interfaces1' checked>");
  page += F("<label class='form-check-label' for='interfacesCheck1'>Interfaces</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='sequencesCheck1' name='sequences1' checked>");
  page += F("<label class='form-check-label' for='sequencesCheck1'>Sequences</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='optionsCheck1' name='options1' checked>");
  page += F("<label class='form-check-label' for='optionsCheck1'>Options</label>");
  page += F("</div>");
  page += F("</div>");

  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</form>");

  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-lg-6 col-12 mb-3'>");

  page += F("<div class='card h-100'>");
  page += F("<form method='post' action='/configurations' enctype='multipart/form-data'>");
  page += F("<div class='card-header'>");
  page += F("<div class='row'>");
  page += F("<div class='col-auto me-auto'>");
  page += F("<h5 class='mb-0'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-upload' viewBox='0 0 20 20'>");
  page += F("<path d='M.5 9.9a.5.5 0 0 1 .5.5v2.5a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1v-2.5a.5.5 0 0 1 1 0v2.5a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2v-2.5a.5.5 0 0 1 .5-.5z'/>");
  page += F("<path d='M7.646 1.146a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 2.707V11.5a.5.5 0 0 1-1 0V2.707L5.354 4.854a.5.5 0 1 1-.708-.708l3-3z'/>");
  page += F("</svg>");
  page += F(" Upload Configuration");
  page += F("</h5>");
  page += F("</div>");
  page += F("<div class='col-auto'>");
  page += F("<button type='submit' name='action' value='upload' class='btn btn-primary btn-sm' style='width: 100%;'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-upload' viewBox='0 0 16 16'>");
  page += F("<path d='M.5 9.9a.5.5 0 0 1 .5.5v2.5a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1v-2.5a.5.5 0 0 1 1 0v2.5a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2v-2.5a.5.5 0 0 1 .5-.5z'/>");
  page += F("<path d='M7.646 1.146a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 2.707V11.5a.5.5 0 0 1-1 0V2.707L5.354 4.854a.5.5 0 1 1-.708-.708l3-3z'/>");
  page += F("</svg>");
  page += F(" Upload</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='card-body'>");
  page += F("<div class='row'>");
  page += F("<div class='col'>");
  page += F("<div class='input-group'>");
  page += F("<input type='file' class='form-control' id='customfile' name='upload'>");
  page += F("</div>");
  page += F("<small id='uploadHelpBlock' class='form-text text-muted'>");
  page += F("SPIFFS is not a high performance FS. It is designed to balance safety, wear levelling and performance for bare flash devices. ");
  page += F("If you want good performance from SPIFFS keep the % utilisation low. Currently used ");
  page += (SPIFFS.usedBytes() * 100) / SPIFFS.totalBytes();
  page += F("%.");
  page += F("</small>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  page += F("</div>");
  page += F("</form>");

  page += F("</div>");

  if (trim_page(start, len)) return;

  DPRINT("Looking for configuration files on SPIFFS root ...\n");
  int     availableconf = 0;
  String  confoptions;
  File    root = SPIFFS.open("/");
  File    file = root.openNextFile();
  while (file) {
    String c = file.name();
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    c = "/" + c;
#endif
    if (c.length() > 4 && c.lastIndexOf(".cfg") == (c.length() - 4)) {
      availableconf++;
      DPRINT("%s\n", c.c_str());
      confoptions += F("<option value='");
      confoptions += c + F("'>");
      confoptions += c.substring(1, c.length() - 4);
      confoptions + F("</option>");
    }
    file = root.openNextFile();
  }
  DPRINT("done.\n");

  if (trim_page(start, len)) return;

  page += F("<div class='col-lg-6 col-12 mb-3'>");

  page += F("<div class='card h-100'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-files' viewBox='0 0 20 20'>");
  page += F("<path d='M13 0H6a2 2 0 0 0-2 2 2 2 0 0 0-2 2v10a2 2 0 0 0 2 2h7a2 2 0 0 0 2-2 2 2 0 0 0 2-2V2a2 2 0 0 0-2-2zm0 13V4a2 2 0 0 0-2-2H5a1 1 0 0 1 1-1h7a1 1 0 0 1 1 1v10a1 1 0 0 1-1 1zM3 4a1 1 0 0 1 1-1h7a1 1 0 0 1 1 1v10a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V4z'/>");
  page += F("</svg>");
  page += F(" Available Configurations</h5>");
  page += F("<div class='card-body'>");
  page += F("<form method='post'>");
  page += F("<div class='row'>");
  page += F("<div class='col-5'>");
  page += F("<select class='form-select' id='filename' name='filename'>");
  page += confoptions;
  page += F("</select>");
  page += F("<small id='filenameHelpBlock' class='form-text text-muted'>");
  page += F("'Apply' to load configuration into current profile.<br>");
  page += F("'Apply & Save' to load configuration into current profile and save the profile.<br>");
  page += F("'Append' to load configuration into current profile. Existing Actions are not deleted.<br>");
  page += F("'Append & Save' to load configuration into current profile and save the profile. Existing Actions are not deleted.<br>");
  page += F("'Download' to download configuration to local computer.<br>");
  page += F("'Delete' to remove configuration.");
  page += F("</small>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-4'>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='actionsCheck2' name='actions2' checked>");
  page += F("<label class='form-check-label' for='actionsCheck2'>Actions</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='pedalsCheck2' name='pedals2' checked>");
  page += F("<label class='form-check-label' for='pedalsCheck2'>Pedals</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='controlsCheck2' name='controls2' checked>");
  page += F("<label class='form-check-label' for='controlsCheck2'>Controls</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='interfacesCheck2' name='interfaces2' checked>");
  page += F("<label class='form-check-label' for='interfacesCheck2'>Interfaces</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='sequencesCheck2' name='sequences2' checked>");
  page += F("<label class='form-check-label' for='sequencesCheck2'>Sequences</label>");
  page += F("</div>");
  page += F("<div class='form-check form-switch'>");
  page += F("<input class='form-check-input' type='checkbox' id='optionsCheck2' name='options2' checked>");
  page += F("<label class='form-check-label' for='optionsCheck2'>Options</label>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-3'>");
  page += F("<button type='submit' name='action' value='apply' class='btn btn-primary btn-sm' style='width: 100%;'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-check2-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M2.5 8a5.5 5.5 0 0 1 8.25-4.764.5.5 0 0 0 .5-.866A6.5 6.5 0 1 0 14.5 8a.5.5 0 0 0-1 0 5.5 5.5 0 1 1-11 0z'/>");
  page += F("<path d='M15.354 3.354a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l7-7z'/>");
  page += F("</svg>");
  page += F("<br>Apply</button><p></p>");
  page += F("<button type='submit' name='action' value='save' class='btn btn-primary btn-sm' style='width: 100%;'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F("<br>Apply & Save</button><p></p>");

  if (trim_page(start, len)) return;

  page += F("<button type='submit' name='action' value='append' class='btn btn-primary btn-sm' style='width: 100%;'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-plus-circle-dotted' viewBox='0 0 16 16'>");
  page += F("<path d='M8 0c-.176 0-.35.006-.523.017l.064.998a7.117 7.117 0 0 1 .918 0l.064-.998A8.113 8.113 0 0 0 8 0zM6.44.152c-.346.069-.684.16-1.012.27l.321.948c.287-.098.582-.177.884-.237L6.44.153zm4.132.271a7.946 7.946 0 0 0-1.011-.27l-.194.98c.302.06.597.14.884.237l.321-.947zm1.873.925a8 8 0 0 0-.906-.524l-.443.896c.275.136.54.29.793.459l.556-.831zM4.46.824c-.314.155-.616.33-.905.524l.556.83a7.07 7.07 0 0 1 .793-.458L4.46.824zM2.725 1.985c-.262.23-.51.478-.74.74l.752.66c.202-.23.418-.446.648-.648l-.66-.752zm11.29.74a8.058 8.058 0 0 0-.74-.74l-.66.752c.23.202.447.418.648.648l.752-.66zm1.161 1.735a7.98 7.98 0 0 0-.524-.905l-.83.556c.169.253.322.518.458.793l.896-.443zM1.348 3.555c-.194.289-.37.591-.524.906l.896.443c.136-.275.29-.54.459-.793l-.831-.556zM.423 5.428a7.945 7.945 0 0 0-.27 1.011l.98.194c.06-.302.14-.597.237-.884l-.947-.321zM15.848 6.44a7.943 7.943 0 0 0-.27-1.012l-.948.321c.098.287.177.582.237.884l.98-.194zM.017 7.477a8.113 8.113 0 0 0 0 1.046l.998-.064a7.117 7.117 0 0 1 0-.918l-.998-.064zM16 8a8.1 8.1 0 0 0-.017-.523l-.998.064a7.11 7.11 0 0 1 0 .918l.998.064A8.1 8.1 0 0 0 16 8zM.152 9.56c.069.346.16.684.27 1.012l.948-.321a6.944 6.944 0 0 1-.237-.884l-.98.194zm15.425 1.012c.112-.328.202-.666.27-1.011l-.98-.194c-.06.302-.14.597-.237.884l.947.321zM.824 11.54a8 8 0 0 0 .524.905l.83-.556a6.999 6.999 0 0 1-.458-.793l-.896.443zm13.828.905c.194-.289.37-.591.524-.906l-.896-.443c-.136.275-.29.54-.459.793l.831.556zm-12.667.83c.23.262.478.51.74.74l.66-.752a7.047 7.047 0 0 1-.648-.648l-.752.66zm11.29.74c.262-.23.51-.478.74-.74l-.752-.66c-.201.23-.418.447-.648.648l.66.752zm-1.735 1.161c.314-.155.616-.33.905-.524l-.556-.83a7.07 7.07 0 0 1-.793.458l.443.896zm-7.985-.524c.289.194.591.37.906.524l.443-.896a6.998 6.998 0 0 1-.793-.459l-.556.831zm1.873.925c.328.112.666.202 1.011.27l.194-.98a6.953 6.953 0 0 1-.884-.237l-.321.947zm4.132.271a7.944 7.944 0 0 0 1.012-.27l-.321-.948a6.954 6.954 0 0 1-.884.237l.194.98zm-2.083.135a8.1 8.1 0 0 0 1.046 0l-.064-.998a7.11 7.11 0 0 1-.918 0l-.064.998zM8.5 4.5a.5.5 0 0 0-1 0v3h-3a.5.5 0 0 0 0 1h3v3a.5.5 0 0 0 1 0v-3h3a.5.5 0 0 0 0-1h-3v-3z'/>");
  page += F("</svg>");
  page += F("<br>Append</button><p></p>");
  page += F("<button type='submit' name='action' value='appendsave' class='btn btn-primary btn-sm' style='width: 100%;'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-plus-circle' viewBox='0 0 16 16'>");
  page += F("<path d='M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14zm0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16z'/>");
  page += F("<path d='M8 4a.5.5 0 0 1 .5.5v3h3a.5.5 0 0 1 0 1h-3v3a.5.5 0 0 1-1 0v-3h-3a.5.5 0 0 1 0-1h3v-3A.5.5 0 0 1 8 4z'/>");
  page += F("</svg>");
  page += F("<br>Append & Save</button><p></p>");

  if (trim_page(start, len)) return;

  page += F("<button type='submit' name='action' value='download' class='btn btn-primary btn-sm' style='width: 100%;'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-download' viewBox='0 0 16 16'>");
  page += F("<path d='M.5 9.9a.5.5 0 0 1 .5.5v2.5a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1v-2.5a.5.5 0 0 1 1 0v2.5a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2v-2.5a.5.5 0 0 1 .5-.5z'/>");
  page += F("<path d='M7.646 11.854a.5.5 0 0 0 .708 0l3-3a.5.5 0 0 0-.708-.708L8.5 10.293V1.5a.5.5 0 0 0-1 0v8.793L5.354 8.146a.5.5 0 1 0-.708.708l3 3z'/>");
  page += F("</svg>");
  page += F("<br>Download</button><p></p>");
  page += F("<button type='submit' name='action' value='delete' class='btn btn-danger btn-sm' style='width: 100%;'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-trash' viewBox='0 0 16 16'>");
  page += F("<path d='M5.5 5.5A.5.5 0 0 1 6 6v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5zm2.5 0a.5.5 0 0 1 .5.5v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5zm3 .5a.5.5 0 0 0-1 0v6a.5.5 0 0 0 1 0V6z'/>");
  page += F("<path fill-rule='evenodd' d='M14.5 3a1 1 0 0 1-1 1H13v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V4h-.5a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1H6a1 1 0 0 1 1-1h2a1 1 0 0 1 1 1h3.5a1 1 0 0 1 1 1v1zM4.118 4L4 4.059V13a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V4.059L11.882 4H4.118zM2.5 3V2h11v1h-11z'/>");
  page += F("</svg>");
  page += F("<br>Delete</button>");
  page += F("</div>");
  page += F("</form>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='col-lg-6 col-12 mb-3'>");

  page += F("<div class='card h-100'>");

  page += F("<div class='card-header'>");
  page += F("<div class='row'>");
  page += F("<div class='col-auto me-auto'>");
  page += F("<h5 class='mb-0'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-pencil-square' viewBox='0 0 20 20'>");
  page += F("<path d='M15.502 1.94a.5.5 0 0 1 0 .706L14.459 3.69l-2-2L13.502.646a.5.5 0 0 1 .707 0l1.293 1.293zm-1.75 2.456l-2-2L4.939 9.21a.5.5 0 0 0-.121.196l-.805 2.414a.25.25 0 0 0 .316.316l2.414-.805a.5.5 0 0 0 .196-.12l6.813-6.814z'/>");
  page += F("<path fill-rule='evenodd' d='M1 13.5A1.5 1.5 0 0 0 2.5 15h11a1.5 1.5 0 0 0 1.5-1.5v-6a.5.5 0 0 0-1 0v6a.5.5 0 0 1-.5.5h-11a.5.5 0 0 1-.5-.5v-11a.5.5 0 0 1 .5-.5H9a.5.5 0 0 0 0-1H2.5A1.5 1.5 0 0 0 1 2.5v11z'/>");
  page += F("</svg>");
  page += F(" Configuration Editor");
  page += F("</h5>");
  page += F("</div>");
  page += F("<div class='col-auto'>");
  page += F("<button class='btn btn-primary btn-sm' style='width: 100%;' id='saveButton'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-save' viewBox='0 0 16 16'>");
  page += F("<path d='M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z'>");
  page += F("</svg>");
  page += F(" Save</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  page += F("<div class='card-body'>");
  page += F("<div id='jsoneditor' style='width: 100%; height: 480px;'></div>");
  page += F("</div>");

  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<link href='https://cdn.jsdelivr.net/npm/jsoneditor@latest/dist/jsoneditor.min.css' rel='stylesheet' type='text/css'>");
  page += F("<script src='https://cdn.jsdelivr.net/npm/jsoneditor@latest/dist/jsoneditor.min.js'></script>");
  page += F("<script>");
  page += F("var container = document.getElementById('jsoneditor');\n"
            "var schema;\n"
            "const options = {schema: schema,\n"
                             "mode: 'tree',\n"
                             "modes: ['code', 'form', 'text', 'tree', 'view', 'preview'],\n"
                             "search: true};\n"
            "var editor = new JSONEditor(container, options);\n"

            "async function loadConfiguration(name) {\n"
                "fetch('/schema.json').then( response => response.json() ).then( json => editor.setSchema(json) );\n"
                "fetch('/files/' + name + '.cfg').then( response => response.text() ).then( text => editor.setText(text) );\n"
                "editor.setName(name);\n"
            "};\n"

            "function saveConfiguration() {\n"
                "const data = new FormData();\n"
                "data.append('file', new File([new Blob([editor.getText()])], editor.getName()));\n"
                "const xhr = new XMLHttpRequest();\n"
                "xhr.open('POST', '/configurations');\n"
                "xhr.send(data);\n"
            "};\n"

            "loadConfiguration(document.getElementById('filename').options[0].text);\n"

            "document.querySelector('#filename').addEventListener('change', function(e) { loadConfiguration(e.target.value.substring(1, e.target.value.length - 4)); });\n"

            "document.querySelector('#saveButton').addEventListener('click', function() { saveConfiguration(); });\n");
  page += F("</script>");

  page += F("</div>");
  page += F("</div>");

  get_footer_page();

  if (trim_page(start, len, true)) return;
}

void get_update_page(unsigned int start, unsigned int len) {

#ifdef WEBCONFIG

  if (get_top_page(0, start, len)) return;

  page += F("<div class='card mb-3' id='downloadCard'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-download' viewBox='0 0 20 20'>");
  page += F("<path d='M.5 9.9a.5.5 0 0 1 .5.5v2.5a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1v-2.5a.5.5 0 0 1 1 0v2.5a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2v-2.5a.5.5 0 0 1 .5-.5z'/>");
  page += F("<path d='M7.646 11.854a.5.5 0 0 0 .708 0l3-3a.5.5 0 0 0-.708-.708L8.5 10.293V1.5a.5.5 0 0 0-1 0v8.793L5.354 8.146a.5.5 0 1 0-.708.708l3 3z'/>");
  page += F("</svg>");
  page += F(" Download Firmware from Cloud</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row'>");
  page += F("<div class='col-4'>");
  page += F("Current version: <b><div>");
  page += VERSION;
  page += F("</div></b></div>");
  page += F("<div class='col-4'>");
  page += F("Latest public version: <b>");
  page += F("<div id='latestFirmwareVersion' w3-include-html='https://raw.githubusercontent.com/alf45tar/PedalinoMini/master/firmware/");
  page += xstr(PLATFORMIO_ENV);
  page += F("/version.txt?");
  page += (rand() % (999999 - 100000 + 1) + 100000);
  page += F("'></div>");
  page += F("</b></div>");
  page += F("<div class='col-1'>");
  page += F("</div>");
  page += F("<div class='col-3'>");
  page += F("<button type='submit' name='action' value='cloudupdate' class='btn btn-primary btn-sm' id='updateButton'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-download' viewBox='0 0 16 16'>");
  page += F("<path d='M.5 9.9a.5.5 0 0 1 .5.5v2.5a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1v-2.5a.5.5 0 0 1 1 0v2.5a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2v-2.5a.5.5 0 0 1 .5-.5z'/>");
  page += F("<path d='M7.646 11.854a.5.5 0 0 0 .708 0l3-3a.5.5 0 0 0-.708-.708L8.5 10.293V1.5a.5.5 0 0 0-1 0v8.793L5.354 8.146a.5.5 0 1 0-.708.708l3 3z'/>");
  page += F("</svg>");
  page += F(" Update</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("<div class='row'>");
  page += F("<div class='col-4'>");
  page += F("<small>");
  page += sketchMD5;
  page += F("</small>");
  page += F("</div>");
  page += F("<div class='col-4'>");
  page += F("<small>");
  page += F("<div id='latestFirmwareVersion' w3-include-html='https://raw.githubusercontent.com/alf45tar/PedalinoMini/master/firmware/");
  page += xstr(PLATFORMIO_ENV);
  page += F("/firmware.bin.md5?");
  page += (rand() % (999999 - 100000 + 1) + 100000);
  page += F("'></div>");
  page += F("</small>");
  page += F("</div>");
  page += F("<div class='col-1'>");
  page += F("</div>");
  page += F("<div class='col-3'>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='card mb-3' id='uploadCard'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-upload' viewBox='0 0 20 20'>");
  page += F("<path d='M.5 9.9a.5.5 0 0 1 .5.5v2.5a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1v-2.5a.5.5 0 0 1 1 0v2.5a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2v-2.5a.5.5 0 0 1 .5-.5z'/>");
  page += F("<path d='M7.646 1.146a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 2.707V11.5a.5.5 0 0 1-1 0V2.707L5.354 4.854a.5.5 0 1 1-.708-.708l3-3z'/>");
  page += F("</svg>");
  page += F(" Upload Firmware from Local Computer</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row'>");
  page += F("<div class='col-8'>");
  page += F("<div class='input-group'>");
  page += F("<input type='file' class='form-control' id='firmwareFile' name='upload'>");
  page += F("</div>");
  page += F("<small id='uploadHelpBlock' class='form-text text-muted'>");
  page += F("Select firmware.bin or spiffs.bin and press Upload to upgrade firmware or file system image. Max file size is ");
  page += FIRMWARE_MAX_SIZE;
  page += F(" bytes.");
  page += F("</small>");
  page += F("</div>");
  page += F("<div class='col-1'>");
  page += F("</div>");
  page += F("<div class='col-3'>");
  page += F("<button name='action' value='fileupdate' class='btn btn-primary btn-sm' id='uploadButton'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='currentColor' class='bi bi-upload' viewBox='0 0 16 16'>");
  page += F("<path d='M.5 9.9a.5.5 0 0 1 .5.5v2.5a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1v-2.5a.5.5 0 0 1 1 0v2.5a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2v-2.5a.5.5 0 0 1 .5-.5z'/>");
  page += F("<path d='M7.646 1.146a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 2.707V11.5a.5.5 0 0 1-1 0V2.707L5.354 4.854a.5.5 0 1 1-.708-.708l3-3z'/>");
  page += F("</svg>");
  page += F(" Upload</button>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='card' id='installCard'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill='currentColor' class='bi bi-usb-symbol' viewBox='0 0 20 20'>");
  page += F("<path d='m7.792.312-1.533 2.3A.25.25 0 0 0 6.467 3H7.5v7.319a2.5 2.5 0 0 0-.515-.298L5.909 9.56A1.5 1.5 0 0 1 5 8.18v-.266a1.5 1.5 0 1 0-1 0v.266a2.5 2.5 0 0 0 1.515 2.298l1.076.461a1.5 1.5 0 0 1 .888 1.129 2.001 2.001 0 1 0 1.021-.006v-.902a1.5 1.5 0 0 1 .756-1.303l1.484-.848A2.5 2.5 0 0 0 11.995 7h.755a.25.25 0 0 0 .25-.25v-2.5a.25.25 0 0 0-.25-.25h-2.5a.25.25 0 0 0-.25.25v2.5c0 .138.112.25.25.25h.741a1.5 1.5 0 0 1-.747 1.142L8.76 8.99a2.584 2.584 0 0 0-.26.17V3h1.033a.25.25 0 0 0 .208-.389L8.208.312a.25.25 0 0 0-.416 0Z'/>");
  page += F("</svg>");
  page += F(" Installer</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row'>");
  page += F("<div class='col-8'>");
  page += F("<small id='installHelpBlock' class='form-text text-muted'>");
  page += F("<a target='_blank' href='https://alf45tar.github.io/PedalinoMini/installer'>PedalinoMini™ Installer</a>");
  page += F("</small>");
  page += F("</div>");
  page += F("<div class='col-1'>");
  page += F("</div>");
  page += F("<div class='col-3'>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<div class='card' style='display: none;' id='progressCard'>");
  page += F("<h5 class='card-header'>");
  page += F("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' fill=ìcurrentColor' class='bi bi-hourglass-split' viewBox='0 0 20 20'>");
  page += F("<path d='M2.5 15a.5.5 0 1 1 0-1h1v-1a4.5 4.5 0 0 1 2.557-4.06c.29-.139.443-.377.443-.59v-.7c0-.213-.154-.451-.443-.59A4.5 4.5 0 0 1 3.5 3V2h-1a.5.5 0 0 1 0-1h11a.5.5 0 0 1 0 1h-1v1a4.5 4.5 0 0 1-2.557 4.06c-.29.139-.443.377-.443.59v.7c0 .213.154.451.443.59A4.5 4.5 0 0 1 12.5 13v1h1a.5.5 0 0 1 0 1h-11zm2-13v1c0 .537.12 1.045.337 1.5h6.326c.216-.455.337-.963.337-1.5V2h-7zm3 6.35c0 .701-.478 1.236-1.011 1.492A3.5 3.5 0 0 0 4.5 13s.866-1.299 3-1.48V8.35zm1 0v3.17c2.134.181 3 1.48 3 1.48a3.5 3.5 0 0 0-1.989-3.158C8.978 9.586 8.5 9.052 8.5 8.351z'/>");
  page += F("</svg>");
  page += F(" Progress</h5>");
  page += F("<div class='card-body'>");
  page += F("<div class='row'>");
  page += F("<div class='col'>");
  page += F("<small id='progressBarDescription' class='form-text text-muted'>");
  page += F("</small>");
  page += F("<div class='progress' id='progressBar'>");
  page += F("<div class='progress-bar' role='progressbar' style='width: 0%;' aria-valuenow='0' aria-valuemin='0' aria-valuemax='100'>0%</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");
  page += F("</div>");

  if (trim_page(start, len)) return;

  page += F("<script>");

  page += F("window.addEventListener('load', includeHTML);"
            "function includeHTML() {"
              //"/* Loop through a collection of all HTML elements: */"
              "let z = document.getElementsByTagName('*');"
              "for (let i = 0; i < z.length; i++) {"
                "let elmnt = z[i];"
                //"/* Search for elements with a certain atrribute: */"
                "let file = elmnt.getAttribute('w3-include-html');"
                "if (file) {"
                  //"/* Make an HTTP request using the attribute value as the file name: */"
                  "let xhttp = new XMLHttpRequest();"
                  "xhttp.onreadystatechange = function() {"
                    "if (this.readyState === 4) {"
                      "if (this.status === 200) {elmnt.innerHTML = this.responseText;}"
                      "if (this.status === 404) {elmnt.innerHTML = 'Page not found.';}"
                      //"/* Remove the attribute, and call this function once more: */"
                      "elmnt.removeAttribute('w3-include-html');"
                      "includeHTML();"
                    "};"
                  "};"
                  "xhttp.open('GET', file, true);"
                  "xhttp.send();"
                  //"/* Exit the function: */"
                  "return;"
                "};"
              "};"
            "};");

  if (trim_page(start, len)) return;

  page += F("const bars = document.getElementsByClassName('progress-bar');"

            "let progressRequested = false;\n"

            "async function onProgressCloud() {\n"
                "if (progressRequested) { return; }\n\n"

                "progressRequested = true;\n"
                "let response = await fetch('/progress');\n"

                "if (!response.ok) {\n"
                  "throw new Error(response.statusText);\n"
                "}\n"

                "let text = await response.text();\n"

                "const percent = +text;\n"
                "if (percent != undefined) {\n"
                    "bars[0].style.width = percent.toFixed(0) + '%';\n"
                    "bars[0].textContent = percent.toFixed(0) + '%';\n"
                "} else {\n"
                    "clearInterval(timer);\n"
                    "bars[0].style.width = '100%';\n"
                    "bars[0].textContent = '100%';\n"
                    "await new Promise(r => setTimeout(r, 1000));\n"
                    "document.getElementById('progressBar').style.height = '1px';\n"
                    "document.getElementById('progressBarDescription').innerHTML = text;\n"
                    "setInterval(countDown, 150);\n"
                "}\n"

                "progressRequested = false;\n"
            "}\n");

  if (trim_page(start, len)) return;

  page += F("document.querySelector('#updateButton').addEventListener('click', function() {\n"

	              "if (!confirm('Do you want to update firmware from cloud?')) { return; };\n"

                "document.getElementById('downloadCard').style.display = 'none';\n"
                "document.getElementById('uploadCard').style.display = 'none';\n"
                "document.getElementById('installCard').style.display = 'none';\n"
                "document.getElementById('progressBarDescription').innerHTML = 'Updating firmware...';\n"

                "bars[0].style.width = '0%';\n"
                "bars[0].textContent = '0%';\n"
                "document.getElementById('progressCard').style.display = 'block';\n"

                "let timer = setInterval(onProgressCloud, 2000);\n"

                "const data = new FormData();\n"
                "data.append('action', 'cloudupdate');\n"

                "const xhr = new XMLHttpRequest();\n"
                "xhr.open('POST', '/update');\n"
                "xhr.send(data);\n"
            "});\n");

  if (trim_page(start, len)) return;

  page += F("async function onProgress() {\n"
                "if (progressRequested) { return; }\n\n"

                "progressRequested = true;\n"
                "let response = await fetch('/progress');\n"

                "if (!response.ok) {\n"
                  "throw new Error(response.statusText);\n"
                "}\n"

                "let text = await response.text();\n"

                "const percent = +text;\n"
                "bars[0].style.width = percent.toFixed(0) + '%';\n"
                "bars[0].textContent = percent.toFixed(0) + '%';\n"

                "progressRequested = false;\n"
            "}\n");

  if (trim_page(start, len)) return;

  page += F("function countDown() {\n"
                "let percent = parseInt(bars[0].style.width, 10);\n"
                "percent--;"
                "bars[0].style.width = percent.toFixed(0) + '%';\n"
                "bars[0].textContent = percent.toFixed(0) + '%';\n"
            "}\n"

            "document.querySelector('#uploadButton').addEventListener('click', function() {\n"

                "const file = document.querySelector('#firmwareFile').files[0];\n"

	              "if (!file) {\n"
		                "alert('No file selected');\n"
		                "return;\n"
	              "}\n"

	              "const allowed_mime_types = [ 'application/octet-stream', 'application/macbinary' ];\n"

	              "const allowed_size = ");
  page += FIRMWARE_MAX_SIZE;
  page += F(";\n");

  if (trim_page(start, len)) return;

  page += F("if(allowed_mime_types.indexOf(file.type) == -1) {\n"
		                "alert('Incorrect file type \"' + file.type + '\"');\n"
		                "return;"
                "}\n"

	              "if(file.size > allowed_size) {\n"
		                "alert('File size exceeed allowed size');\n"
		                "return;"
	              "}\n"

	              "if (!confirm('Do you want to update firmware with \"' + file.name + '\"?')) { return; };\n"

                "document.getElementById('downloadCard').style.display = 'none';\n"
                "document.getElementById('uploadCard').style.display = 'none';\n"
                "document.getElementById('installCard').style.display = 'none';\n"
                "document.getElementById('progressBarDescription').innerHTML = 'Updating firmware...';\n"
                "bars[0].style.width = '0%';\n"
                "bars[0].textContent = '0%';\n"
                "document.getElementById('progressCard').style.display = 'block';\n");

  if (trim_page(start, len)) return;

  page += F("   let timer = setInterval(onProgress, 2000);\n"

                "const data = new FormData();\n"
                "data.append('file', file);\n"
                "data.append('action', 'fileupdate');\n"

                "const xhr = new XMLHttpRequest();\n"
                "xhr.open('POST', '/update');\n"

                "xhr.addEventListener('load', async function(e) {\n"
                    "clearInterval(timer);\n"
                    "progressRequested = false;\n"
                    "bars[0].style.width = '100%';\n"
                    "bars[0].textContent = '100%';\n"
                    "await new Promise(r => setTimeout(r, 1000));\n"
                    "document.getElementById('progressBar').style.height = '1px';\n"
                    "document.getElementById('progressBarDescription').innerHTML = xhr.response;\n"
                    "setInterval(countDown, 150);\n"
                "});\n"

                "xhr.send(data);\n"
            "});\n");

  page += F("</script>");

  get_footer_page();

  if (trim_page(start, len, true)) return;

#else
  page += F("<!doctype html>");
  page += F("<html lang='en'>");
  page += F("<head>");
  page += F("<title>PedalinoMini&trade;</title>");
  page += F("</head>");
  page += F("<body>");
  page += F("<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  page += F("</body>");
  page += F("</html>");
#endif
}

size_t get_root_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  get_root_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

size_t get_live_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  get_live_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

size_t get_actions_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  get_actions_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

size_t get_pedals_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  get_pedals_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

size_t get_controls_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  get_controls_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

size_t get_interfaces_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  get_interfaces_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

size_t get_sequences_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  get_sequences_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

size_t get_options_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  get_options_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

size_t get_configurations_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  get_configurations_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

size_t get_update_page_chunked(uint8_t *buffer, size_t maxLen, size_t index) {

  page = "";
  //if (index == 0) latestFirmwareVersion = get_latest_firmware_version();
  get_update_page(index, maxLen - 1);
  page.getBytes(buffer, maxLen, 0);
  buffer[maxLen-1] = 0; // CWE-126
  size_t byteWritten = strlen((const char *)buffer);
  if (byteWritten == 0) {
    page = "";
    alert = "";
    alertError = "";
    fullPageCompleted = true;
  }
  return byteWritten;
}

void http_handle_login(AsyncWebServerRequest *request) {
  if (httpUsername.isEmpty()) {
    authenticated = true;
    request->redirect("/");
  } else {
    authenticated = false;
    get_login_page();
    request->send(200, "text/html", page.c_str());
  }
}

void http_handle_post_login(AsyncWebServerRequest *request) {
  if (request->hasArg("username")) {
    if (request->arg("username") == httpUsername)
      if (request->hasArg("password"))
        if (request->arg("password") == httpPassword) {
          authenticated = true;
          return request->redirect("/");
        }
  }
  get_login_page();
  request->send(200, "text/html", page.c_str());
}

void http_handle_globals(AsyncWebServerRequest *request) {

  if (request->hasArg("profile")) {
    uiprofile = request->arg("profile");
    currentProfile = constrain(uiprofile.toInt() - 1, 0, PROFILES - 1);
    reloadProfile = true;
  }

  if (request->hasArg("theme") ) {
    if(request->arg("theme") != theme) {
      theme = request->arg("theme");
    }
  }
}

void http_handle_root(AsyncWebServerRequest *request) {
  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_root_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_live(AsyncWebServerRequest *request) {
  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_live_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_actions(AsyncWebServerRequest *request) {

  String list;
  action *a[BANKS];
  char    n[BANKS][MAXBANKNAME+1];

  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
  if (request->hasArg("bank"))        uibank    = request->arg("bank");
  if (request->hasArg("control"))     uicontrol = request->arg("control");
  if (request->hasArg("banksorder"))  list      = request->arg("banksorder");

  for (byte b = 0; b < BANKS; b++) {
    a[b] = actions[b];
    strlcpy(n[b], banknames[b], MAXBANKNAME+1);
  }

  int  i1 = 0;
  int  i2 = list.indexOf(",");
  byte to = 0;
  while (to < BANKS && i2 != -1) {
    long from = constrain(list.substring(i1, i2).toInt(), 0, BANKS - 1);
    a[to] = actions[from];
    strlcpy(n[to], banknames[from], MAXBANKNAME+1);
    to++;
    i1 = i2 + 1;
    i2 = (i2 == list.lastIndexOf(",")) ? list.length() : list.indexOf(",", i1);
  }

  for (byte b = 0; b < BANKS; b++) {
    actions[b] = a[b];
    strlcpy(banknames[b], n[b], MAXBANKNAME+1);
  }

  create_banks();

  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_actions_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_pedals(AsyncWebServerRequest *request) {
  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_pedals_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_controls(AsyncWebServerRequest *request) {
  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
    if (request->hasArg("controlpage")) uicontrolpage = request->arg("controlpage");
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_controls_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_interfaces(AsyncWebServerRequest *request) {
  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_interfaces_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_sequences(AsyncWebServerRequest *request) {
  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
  if (request->hasArg("sequence")) uisequence = request->arg("sequence");
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_sequences_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_options(AsyncWebServerRequest *request) {
  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
  //networks = WiFi.scanNetworks(true); // Async scan
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_options_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_configurations(AsyncWebServerRequest *request) {
  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_configurations_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_update(AsyncWebServerRequest *request) {
  if (!httpUsername.isEmpty() && !request->authenticate(httpUsername.c_str(), httpPassword.c_str())) return request->requestAuthentication();
  http_handle_globals(request);
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_update_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_progress(AsyncWebServerRequest *request) {
  if (firmwareUpdate == PED_UPDATE_CLOUD)
    request->send(200, "text/plain", String(HttpsOTA.status() == HTTPS_OTA_UPDATING ? 100 * otaProgress / FIRMWARE_MAX_SIZE : 0));
  else
    request->send(200, "text/plain", String(Update.isRunning() && Update.size() ? 100 * Update.progress() / Update.size() : 0));
}

void http_handle_post_live(AsyncWebServerRequest *request) {

  String a;

  a = request->arg("profile");
  currentProfile = a.toInt();

  alert = "Saved";

  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_live_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_post_actions(AsyncWebServerRequest *request) {

  const byte b = constrain(uibank.toInt(), 0, BANKS - 1);
  const byte c = constrain(uicontrol.toInt() - 1, 0, CONTROLS - 1);

  strlcpy(banknames[b], request->arg(String("bankname")).c_str(), MAXBANKNAME+1);

  if (request->arg("action").equals("new")) {
    action *act = actions[b];
    if (act == nullptr) {
       act = actions[b] = (action*)malloc(sizeof(action));
       assert(act != nullptr);
    }
    else {
      while (act->next != nullptr) act = act->next;
      act->next = (action*)malloc(sizeof(action));
      assert(act->next != nullptr);
      act = act->next;
    }
    act->tag0[0]       = 0;
    act->tag1[0]       = 0;
    act->control       = 0;
    act->led           = 255;
    act->color0        = CRGB::Black;
    act->color1        = CRGB::Black;
    act->event         = PED_EVENT_NONE;
    act->midiMessage   = PED_EMPTY;
    act->midiChannel   = 1;
    act->midiCode      = 0;
    act->midiValue1    = 0;
    act->midiValue2    = 127;
    act->oscAddress[0] = 0;
    act->slot          = SLOTS;
    act->next          = nullptr;
    sort_actions();
    create_banks();
    alert = "";
  }
  else if (request->arg("action").startsWith("new")) {
    action *act = actions[b];
    if (act == nullptr)
       act = actions[b] = (action*)malloc(sizeof(action));
    else {
      while (act->next != nullptr) act = act->next;
      act->next = (action*)malloc(sizeof(action));
      assert(act->next != nullptr);
      act = act->next;
    }
    act->tag0[0]       = 0;
    act->tag1[0]       = 0;
    act->control       = constrain(request->arg("action").substring(3).toInt() - 1, 0, CONTROLS - 1);
    act->led           = 255;
    act->color0        = CRGB::Black;
    act->color1        = CRGB::Black;
    act->event         = PED_EVENT_NONE;
    act->midiMessage   = PED_EMPTY;
    act->midiChannel   = 1;
    act->midiCode      = 0;
    act->midiValue1    = 0;
    act->midiValue2    = 127;
    act->oscAddress[0] = 0;
    act->slot          = SLOTS;
    act->next          = nullptr;
    sort_actions();
    create_banks();
    alert = "";
  }
  else if (request->arg("action").startsWith("delete")) {
    unsigned int i       = 0;
    action      *act     = actions[b];
    action      *actPrev = nullptr;
    action      *actNext = (act == nullptr) ? nullptr : act->next;

    while (act != nullptr) {
      if (act->control == c || uicontrol.equals("All")) {
        i++;
        if (request->arg("action").equals(String("delete") + String(i))) {
          if (actPrev == nullptr) {         // first
            actions[b] = actNext;
            actNext = (actions[b] == nullptr) ? nullptr : actions[b]->next;
            free(act);
            act = actions[b];
          }
          else if (actNext == nullptr) {    // last
            actPrev->next = nullptr;
            free(act);
            act = nullptr;
          }
          else {                            // in the middle
            actPrev->next = actNext;
            free(act);
            act = actNext;
            actNext = (act == nullptr) ? nullptr : act->next;
          }
        }
        else {                              // next action
          actPrev = act;
          act = act->next;
          actNext = (act == nullptr) ? nullptr : act->next;
        }
      }
      else {
        actPrev = act;
        act = act->next;
        actNext = (act == nullptr) ? nullptr : act->next;
      }
    }
    create_banks();
    alert = F("Selected action(s) deleted.");
  }
  else if (request->arg("action").startsWith("copy")) {
    if (actions[b] != nullptr) {
      byte destination = request->arg("action").substring(4).toInt();
      destination = constrain(destination, 0, BANKS - 1);
      action *act = actions[b];
      while (act != nullptr) {
        action *a = (action*)malloc(sizeof(action));
        assert(a != nullptr);
        memcpy(a, act, sizeof(action));
        a->next = actions[destination];
        actions[destination] = a;
        act = act->next;
      }
      sort_actions();
      create_banks();
      alert = F("Bank copied.");
    }
  }
  else if (request->arg("action").equals("apply") || request->arg("action").equals("save")) {
    unsigned int i      = 0;
    action      *act    = actions[b];
    while (act != nullptr) {
      if (act->control == c || uicontrol.equals("All")) {
        i++;
        act->control      = constrain(request->arg(String("control")    + String(i)).toInt() - 1, 0, CONTROLS - 1);
        act->event        = constrain(request->arg(String("event")      + String(i)).toInt(), 0, 255);
        act->midiMessage  = constrain(request->arg(String("message")    + String(i)).toInt(), 0, 255);
        act->midiCode     = constrain(request->arg(String("code")       + String(i)).toInt(), 0, MIDI_RESOLUTION - 1);
        act->midiValue1   = constrain(request->arg(String("from")       + String(i)).toInt(), 0, MIDI_RESOLUTION - 1);
        act->midiValue2   = constrain(request->arg(String("to")         + String(i)).toInt(), 0, MIDI_RESOLUTION - 1);
        act->midiChannel  = constrain(request->arg(String("channel")    + String(i)).toInt(), 0, 17);
        act->slot         = constrain(request->arg(String("slot")       + String(i)).toInt() - 1, 0, SLOTS);
        strlcpy(act->tag0,            request->arg(String("nameoff")    + String(i)).c_str(), MAXACTIONNAME + 1);
        strlcpy(act->tag1,            request->arg(String("nameon")     + String(i)).c_str(), MAXACTIONNAME + 1);
        strlcpy(act->oscAddress,      request->arg(String("oscaddress") + String(i)).c_str(), sizeof(act->oscAddress));
        switch (act->midiMessage) {
          case PED_SEQUENCE:
          case PED_SEQUENCE_STEP_BY_STEP_FWD:
          case PED_SEQUENCE_STEP_BY_STEP_REV:
            act->midiChannel  = constrain(request->arg(String("sequence") + String(i)).toInt(), 1, SEQUENCES);
            act->color0   = CRGB::Black;
            act->color1   = CRGB::Black;
            break;
          case PED_ACTION_BANK_PLUS:
          case PED_ACTION_BANK_MINUS:
          case PED_ACTION_PROFILE_PLUS:
          case PED_ACTION_PROFILE_MINUS:
          case PED_ACTION_DEVICE_INFO:
          case PED_ACTION_POWER_ON_OFF:
            act->led      = 255;
            act->color0   = CRGB::Black;
            act->color1   = CRGB::Black;
            break;
          default: {
            unsigned int red, green, blue;
            act->led      = constrain(request->arg(String("led")        + String(i)).toInt(), 0, 255);
            if (act->led != 255) act->led = (act->led == 0 ? LEDS : constrain(request->arg(String("led") + String(i)).toInt() - 1, 0, LEDS - 1));
            act->color0   = CRGB::Black;
            red = green = blue = 0;
            sscanf(request->arg(String("color0-") + String(i)).c_str(), "#%02x%02x%02x", &red, &green, &blue);
            act->color0 = ((red & 0xff) << 16) | ((green & 0xff) << 8) | (blue & 0xff);
            act->color1   = CRGB::Black;
            red = green = blue = 0;
            sscanf(request->arg(String("color1-") + String(i)).c_str(), "#%02x%02x%02x", &red, &green, &blue);
            act->color1 = ((red & 0xff) << 16) | ((green & 0xff) << 8) | (blue & 0xff);
           }
        }
      }
      act = act->next;
    }
    sort_actions();
    create_banks();
    leds_refresh();
    alert = F("Changes applied. Changes will be lost on next reboot or on profile switch if not saved.");
    if (request->arg("action").equals("save")) {
      eeprom_update_profile();
      eeprom_update_current_profile(currentProfile);
      alert = "Changes saved.";
    }
  }

  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_actions_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_post_pedals(AsyncWebServerRequest *request) {

  String       a;
  const String checked = "on";

  //httpServer.sendHeader("Connection", "close");
  for (unsigned int i = 0; i < PEDALS; i++) {
    a = request->arg(String("autosensing") + String(i+1));
    pedals[i].autoSensing = (a == checked) ? PED_ENABLE : PED_DISABLE;

    a = request->arg(String("mode") + String(i+1));
    pedals[i].mode = a.toInt();

    a = request->arg(String("function1") + String(i+1));
    pedals[i].pressMode  = ((a == checked) ? PED_PRESS_1 : 0);

    a = request->arg(String("function2") + String(i+1));
    pedals[i].pressMode += ((a == checked) ? PED_PRESS_2 : 0);

    a = request->arg(String("function3") + String(i+1));
    pedals[i].pressMode += ((a == checked) ? PED_PRESS_L : 0);

    a = request->arg(String("polarity") + String(i+1));
    pedals[i].invertPolarity = (a == checked) ? PED_ENABLE : PED_DISABLE;

    a = request->arg(String("latchemulation") + String(i+1));
    pedals[i].latchEmulation = (a == checked) ? PED_ENABLE : PED_DISABLE;

    a = request->arg(String("map") + String(i+1));
    pedals[i].analogResponse = a.toInt();

    a = request->arg(String("min") + String(i+1));
    pedals[i].expZero = constrain(a.toInt(), 0, ADC_RESOLUTION - 1);

    a = request->arg(String("max") + String(i+1));
    pedals[i].expMax = constrain(a.toInt(), 0, ADC_RESOLUTION - 1);

    a = request->arg(String("analogeasing") + String(i+1));
    pedals[i].snapMultiplier = pow(10, -constrain(4 - a.toInt(), 1, 3));

    a = request->arg(String("analogthreshold") + String(i+1));
    pedals[i].activityThreshold = pow(2, constrain(a.toInt(), 1, 8));

    a = request->arg(String("gain") + String(i+1));
    pedals[i].gain = constrain(a.toFloat(), 0.0, 100.0);

    a = request->arg(String("retrigger") + String(i+1));
    pedals[i].retrigger = constrain(a.toFloat(), 0.0, 1.0);

    a = request->arg(String("scancycles") + String(i+1));
    pedals[i].scanCycles = constrain(a.toInt(), 0, 100);

    a = request->arg(String("holdcycles") + String(i+1));
    pedals[i].holdCycles = constrain(a.toInt(), 0, 100);

    a = request->arg(String("crosstalk") + String(i+1));
    pedals[i].crosstalk = constrain(a.toInt(), 0, 100);

    a = request->arg(String("crosstalkratio") + String(i+1));
    pedals[i].crosstalkRatio = constrain(a.toFloat(), 0.0, 100.0);

    if (pedals[i].expMax < pedals[i].expZero) {
      int t;
      t = pedals[i].expMax;
      pedals[i].expMax = pedals[i].expZero;
      pedals[i].expZero = t;
    }
    if (pedals[i].autoSensing == PED_ENABLE) {
      pedals[i].expZero = ADC_RESOLUTION - 1;
      pedals[i].expMax = 0;
    }
    if (pedals[i].mode == PED_ANALOG_PAD || pedals[i].mode == PED_ANALOG_PAD_MOMENTARY)
      pedals[i].autoSensing = PED_DISABLE;
  }

  if (request->arg("action").equals("apply")) {
    loadConfig = true;
    alert = F("Changes applied. Changes will be lost on next reboot or on profile switch if not saved.");
  }
  else if (request->arg("action").equals("save")) {
    eeprom_update_profile();
    eeprom_update_current_profile(currentProfile);
    loadConfig = true;
    alert = "Changes saved.";
  }

  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_pedals_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_post_controls(AsyncWebServerRequest *request) {

  String     a;
  const byte p = constrain(uicontrolpage.toInt() - 1, 0, CONTROLS / CONTROLS_PER_PAGE - 1);

  for (unsigned int i = p*CONTROLS_PER_PAGE; i < (p+1)*CONTROLS_PER_PAGE; i++) {
    a = request->arg(String("pedal1-") + String(i+1));
    controls[i].pedal1 = (a.toInt() == 0 ? PEDALS : constrain(a.toInt() - 1, 0, PEDALS - 1));

    a = request->arg(String("button1-") + String(i+1));
    controls[i].button1 = (a.toInt() == 0 ? LADDER_STEPS : constrain(a.toInt() - 1, 0, LADDER_STEPS - 1));

    a = request->arg(String("pedal2-") + String(i+1));
    controls[i].pedal2 = (a.toInt() == 0 ? PEDALS : constrain(a.toInt() - 1, 0, PEDALS - 1));

    a = request->arg(String("button2-") + String(i+1));
    controls[i].button2 = (a.toInt() == 0 ? LADDER_STEPS : constrain(a.toInt() - 1, 0, LADDER_STEPS - 1));

    a = request->arg(String("led") + String(i+1));
    controls[i].led = (a.toInt() == 0 ? LEDS : constrain(a.toInt() - 1, 0, LEDS - 1));
  }

  if (request->arg("action").equals("apply")) {
    loadConfig = true;
    alert = F("Changes applied. Changes will be lost on next reboot or on profile switch if not saved.");
  }
  else if (request->arg("action").equals("save")) {
    eeprom_update_profile();
    eeprom_update_current_profile(currentProfile);
    loadConfig = true;
    alert = "Changes saved.";
  }

  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_controls_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_post_interfaces(AsyncWebServerRequest *request) {

  String       a;
  const String checked = "on";

  for (unsigned int i = 0; i < INTERFACES; i++) {
    a = request->arg(String("in") + String(i+1));
    interfaces[i].midiIn = (a == checked) ? PED_ENABLE : PED_DISABLE;

    a = request->arg(String("showin") + String(i+1));
    interfaces[i].midiIn += (a == checked) ? PED_SHOW : 0;

    a = request->arg(String("out") + String(i+1));
    interfaces[i].midiOut = (a == checked) ? PED_ENABLE : PED_DISABLE;

    a = request->arg(String("showout") + String(i+1));
    interfaces[i].midiOut += (a == checked) ? PED_SHOW : 0;

    a = request->arg(String("thru") + String(i+1));
    interfaces[i].midiThru = (a == checked) ? PED_ENABLE : PED_DISABLE;

    a = request->arg(String("clock") + String(i+1));
    interfaces[i].midiClock = (a == checked) ? PED_ENABLE : PED_DISABLE;
  }
  if (request->arg("action").equals("apply")) {
    alert = F("Changes applied. Changes will be lost on next reboot or on profile switch if not saved.");
  }
  else if (request->arg("action").equals("save")) {
    eeprom_update_profile();
    eeprom_update_current_profile(currentProfile);
    alert = "Changes saved.";
  }

  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_interfaces_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_post_sequences(AsyncWebServerRequest *request) {

  String     a;
  const byte s = constrain(uisequence.toInt() - 1, 0, SEQUENCES - 1);
  unsigned int red, green, blue;

  for (unsigned int i = 0; i < STEPS; i++) {

    a = request->arg(String("message") + String(i+1));
    sequences[s][i].midiMessage = constrain(a.toInt(), 0, 255);

    a = request->arg(String("channel") + String(i+1));
    sequences[s][i].midiChannel = constrain(a.toInt(), 0, 17);

    switch (sequences[s][i].midiMessage) {
      case PED_SEQUENCE:
        a = request->arg(String("sequence") + String(i+1));
        sequences[s][i].midiChannel  = constrain(a.toInt() - 1, 0, SEQUENCES - 1);
        break;
    }

    a = request->arg(String("code") + String(i+1));
    sequences[s][i].midiCode = constrain(a.toInt(), 0, MIDI_RESOLUTION - 1);

    a = request->arg(String("value") + String(i+1));
    sequences[s][i].midiValue = constrain(a.toInt(), 0, MIDI_RESOLUTION - 1);

    a = request->arg(String("led") + String(i+1));
    sequences[s][i].led = constrain(a.toInt(), 0, 255);
    if (sequences[s][i].led != 255) sequences[s][i].led = (sequences[s][i].led == 0 ? LEDS : constrain(a.toInt() - 1, 0, LEDS - 1));

    a = request->arg(String("color") + String(i+1));
    red = green = blue = 0;
    sscanf(request->arg(String("color") + String(i+1)).c_str(), "#%02x%02x%02x", &red, &green, &blue);
    sequences[s][i].color = ((red & 0xff) << 16) | ((green & 0xff) << 8) | (blue & 0xff);
  }
  if (request->arg("action").equals("apply")) {
    alert = F("Changes applied. Changes will be lost on next reboot or on profile switch if not saved.");
  }
  else if (request->arg("action").equals("save")) {
    eeprom_update_profile();
    eeprom_update_current_profile(currentProfile);
    alert = "Changes saved.";
  }

  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_sequences_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

void http_handle_post_options(AsyncWebServerRequest *request) {

  const String checked = "on";
  bool  restartRequired = false;
  bool  poweroffRequired = false;

  http_handle_globals(request);

  if (request->arg("mdnsdevicename") != host) {
    host = request->arg("mdnsdevicename");
    eeprom_update_device_name(host);
    // Postpone the restart until after all changes are committed to EEPROM.
    restartRequired = true;
  }

  int newBootMode = bootMode;
  if (request->arg("bootmodewifi") == checked)
    if (request->arg("bootmodeap") == checked)
      if (request->arg("bootmodeble") == checked)
        newBootMode = PED_BOOT_AP;
      else newBootMode = PED_BOOT_AP_NO_BLE;
    else if (request->arg("bootmodeble") == checked)
           newBootMode = PED_BOOT_NORMAL;
         else newBootMode = PED_BOOT_WIFI;
  else if (request->arg("bootmodeble") == checked) newBootMode = PED_BOOT_BLE;
       else newBootMode = PED_BOOT_NORMAL;
  if (newBootMode != bootMode) {
    bootMode = newBootMode;
    //eeprom_update_boot_mode(bootMode);
  }

#ifdef BLE
  if (request->arg("bleServer") != bleServer) {
    bleServer = request->arg("bleServer");
    BLEBLE_MIDI.end();
    BLEBLE_MIDI.setName(bleServer.c_str());
    BLEBLE_MIDI.begin();
    restartRequired = false;
  }
#endif

  if (request->arg("wifiSSID") != wifiSSID || request->arg("wifiPassword") != wifiPassword) {
    wifiSSID      = request->arg("wifiSSID");
    wifiPassword  = request->arg("wifiPassword");
    restartRequired = (bootMode == PED_BOOT_NORMAL || bootMode == PED_BOOT_WIFI);
    if (restartRequired) eeprom_update_sta_wifi_credentials();
  }

  if (request->arg("ssidSoftAP") != ssidSoftAP || request->arg("passwordSoftAP") != passwordSoftAP) {
    ssidSoftAP      = request->arg("ssidSoftAP");
    passwordSoftAP  = request->arg("passwordSoftAP");
    restartRequired = (bootMode == PED_BOOT_AP || bootMode == PED_BOOT_AP_NO_BLE);
    if (restartRequired) eeprom_update_ap_wifi_credentials(ssidSoftAP, passwordSoftAP);
  }

  if (request->arg("httpUsername") != httpUsername || request->arg("httpPassword") != httpPassword) {
    httpUsername  = request->arg("httpUsername");
    httpPassword  = request->arg("httpPassword");
    restartRequired = false;
  }

  if (request->arg("debounceinterval").toInt() != debounceInterval) {
    debounceInterval = request->arg("debounceinterval").toInt();
    loadConfig = true;
  }
  if (request->arg("simultaneousgaptime").toInt() != simultaneousGapTime) {
    simultaneousGapTime = request->arg("simultaneousgaptime").toInt();
    loadConfig = true;
  }
  if (request->arg("presstime").toInt() != pressTime) {
    pressTime = request->arg("presstime").toInt();
    loadConfig = true;
  }
  if (request->arg("doublepresstime").toInt() != doublePressTime) {
    doublePressTime = request->arg("doublepresstime").toInt();
    loadConfig = true;
  }
  if (request->arg("longpresstime").toInt() != longPressTime) {
    longPressTime = request->arg("longpresstime").toInt();
    loadConfig = true;
  }
  if (request->arg("repeatpresstime").toInt() != repeatPressTime) {
    repeatPressTime = request->arg("repeatpresstime").toInt();
    loadConfig = true;
  }

  //if (pressTimeChanged) eeprom_update_press_time(debounceInterval, simultaneousGapTime, pressTime, doublePressTime,longPressTime, repeatPressTime);

  bool newFlipScreen = (request->arg("flipscreen") == checked);
  if (newFlipScreen != flipScreen) {
    flipScreen = newFlipScreen;
    displayInit = true;
  }

  bool newTapDanceMode = (request->arg("tapdancemode") == checked);
  if (newTapDanceMode != tapDanceMode) {
    tapDanceBank = newTapDanceMode;
    //eeprom_update_tap_dance(tapDanceMode);
  }

  bool newRepeatOnBankSwitch = (request->arg("repeatonbankswitch") == checked);
  if (newRepeatOnBankSwitch != repeatOnBankSwitch) {
    repeatOnBankSwitch = newRepeatOnBankSwitch;
    //eeprom_update_repeat_on_bank_switch(repeatOnBankSwitch);
  }

  bool newLadder = false;
  for (byte i = 0; i < LADDER_STEPS; i++) {
    String a = request->arg(String("threshold") + String(i+1));
    newLadder = newLadder || ladderLevels[i] != a.toInt();
    ladderLevels[i] = a.toInt();
  }
  // if (newLadder) eeprom_update_ladder();

  if (request->arg("encodersensitivity").toInt() != encoderSensitivity) {
    encoderSensitivity = request->arg("encodersensitivity").toInt();
    //eeprom_update_encoder_sensitivity(encoderSensitivity);
    loadConfig = true;
  }

  if (request->arg("screensavertimeout").toInt() != screenSaverTimeout / 60000) {
    screenSaverTimeout = request->arg("screensavertimeout").toInt() * 60000;
  }

  if (request->arg("rgborder").toInt() != rgbOrder) {
    rgbOrder = (EOrder)request->arg("rgborder").toInt();
  }

  if (request->arg("ledsonbrightness").toInt() != ledsOnBrightness) {
    ledsOnBrightness = request->arg("ledsonbrightness").toInt();
  }

  if (request->arg("ledsoffbrightness").toInt() != ledsOffBrightness) {
    ledsOffBrightness = request->arg("ledsoffbrightness").toInt();
  }

  if (request->arg("osclocalport").toInt() != oscLocalPort) {
    oscLocalPort = request->arg("osclocalport").toInt();
    MDNS.addService("_osc", "_udp", oscLocalPort);
    oscUDPin.listen(oscLocalPort);
  }

  bool oscReconnect = false;
  if (request->arg("oscremotehost") != oscRemoteHost ||
      !oscRemoteHost.equals(oscRemoteIp.toString()) && oscRemoteIp.toString().equals("255.255.255.255")) {
    oscRemoteHost = request->arg("oscremotehost");
    if (!oscRemoteIp.fromString(oscRemoteHost)) {
      oscRemoteIp = MDNS.queryHost(oscRemoteHost);
      if (oscRemoteIp.toString().equals("0.0.0.0")) {
        oscRemoteIp = IPADDR_BROADCAST;
        DPRINT("Host %s not found via mDNS. Using broadcast IP address 255.255.255.255.\n", oscRemoteHost.c_str());
      }
      else {
        DPRINT("Resolved host %s to %s via mDNS.\n", oscRemoteHost.c_str(), oscRemoteIp.toString().c_str());
      }
    }
    oscReconnect = true;
  }

  if (request->arg("oscremoteport").toInt() != oscRemotePort) {
    oscRemotePort = request->arg("oscremoteport").toInt();
    oscReconnect = true;
  }

  if (oscReconnect) {
    oscUDPout.close();
    oscUDPout.connect(oscRemoteIp, oscRemotePort);
  }

  if (request->arg("action").equals("apply")) {
    alert = F("Changes applied. Changes will be lost on next reboot or on profile switch if not saved.");
  }
  else if (request->arg("action").equals("save")) {
    eeprom_update_globals();
    alert = "Changes saved.";
  }
  else if (request->arg("action").equals("factorydefault")) {
    eeprom_initialize();
    restartRequired = true;
  }
  else if (request->arg("action").equals("reboot")) {
    restartRequired = true;
  }
  else if (request->arg("action").equals("poweroff")) {
    poweroffRequired = true;
  }

  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_options_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);

  // Restart only after all changes have been committed to EEPROM, and the response has been sent to the HTTP client.
  if (restartRequired) {
    delay(1000);
    ESP.restart();
  }

  if (poweroffRequired) {
    display_off();
    leds_off();
#ifdef BLE
    esp_bluedroid_disable();
    esp_bt_controller_disable();
#endif
    esp_wifi_stop();
    //adc_power_off();
    adc_power_release();
    delay(200);
    for (byte b = 0; b < BANKS; b++) {
      action *act = actions[b];
      while (act != nullptr) {
        if (act->midiMessage == PED_ACTION_POWER_ON_OFF) {
          rtc_gpio_pullup_en((gpio_num_t)PIN_D(controls[act->control].pedal1));
          rtc_gpio_pulldown_dis((gpio_num_t)PIN_D(controls[act->control].pedal1));
          esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_D(controls[act->control].pedal1), 0);
          b = BANKS;
          break;
        }
        act = act->next;
      }
    }
    esp_deep_sleep_start();
  }
}

void http_handle_post_configurations(AsyncWebServerRequest *request) {

  const String checked = "on";

  if (request->arg("action").equals("new")) {
    if (request->arg("newconfiguration").isEmpty())  {
      alertError = F("Configuration not saved. No configuration name provided.");
    }
    else {
      String configname("/" + request->arg("newconfiguration") + ".cfg");

      File file = SPIFFS.open(configname, FILE_WRITE);
      if (file) {
        file.close();
        spiffs_save_config(configname,
                           request->arg("actions1")    == checked,
                           request->arg("pedals1")     == checked,
                           request->arg("controls1")   == checked,
                           request->arg("interfaces1") == checked,
                           request->arg("sequences1")  == checked,
                           request->arg("options1")    == checked);
        alert = F("Current profile setup saved as '");
        alert += request->arg("newconfiguration");
        alert += F("'.");
      }
      else {
        alertError = F("Cannot create '");
        alertError += request->arg("newconfiguration");
        alertError += F("'.");
      }
    }
  }
  else if (request->arg("action").equals("upload")) {
    if (alert.isEmpty())  {
      alertError = F("No file selected. Choose file using Browse button.");
    }
  }
  else if (request->arg("action").equals("apply") || request->arg("action").equals("append")) {
    String config = request->arg("filename");
    spiffs_load_config(config,
                       request->arg("actions2")    == checked,
                       request->arg("pedals2")     == checked,
                       request->arg("controls2")   == checked,
                       request->arg("interfaces2") == checked,
                       request->arg("sequences2")  == checked,
                       request->arg("options2")    == checked,
                       request->arg("action").equals("append"));
    sort_actions();
    create_banks();
    loadConfig = true;
    config = config.substring(1, config.length() - 4);
    alert = F("Configuration '");
    alert += config + F("' loaded into current profile and running. Profile not saved.");
  }
  else if (request->arg("action").equals("save") || request->arg("action").equals("appendsave")) {
    String config = request->arg("filename");
    spiffs_load_config(config,
                       request->arg("actions2")    == checked,
                       request->arg("pedals2")     == checked,
                       request->arg("controls2")   == checked,
                       request->arg("interfaces2") == checked,
                       request->arg("sequences2")  == checked,
                       request->arg("options2")    == checked,
                       request->arg("action").equals("appendsave"));
    sort_actions();
    create_banks();
    eeprom_update_globals();
    eeprom_update_profile();
    reloadProfile = true;
    config = config.substring(1, config.length() - 4);
    alert = F("Configuration '");
    alert += config + F("' loaded and saved into current profile.");
  }
  else if (request->arg("action").equals("download")) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, request->arg("filename"), String(), true);
    request->send(response);
    return;
  }
  else if (request->arg("action").equals("delete")) {
    String config = request->arg("filename");
    if (SPIFFS.remove(config)) {
      config = config.substring(1, config.length() - 4);
      alert = F("Configuration '");
      alert += config + F("' deleted.");
    }
    else {
      alertError = F("Cannot delete '");
      alertError += config + F("'.");
    }
  }
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", get_configurations_page_chunked);
  response->addHeader("Connection", "close");
  request->send(response);
}

// handler for the file upload

void http_handle_configuration_file_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

  //Upload handler chunks in data
  if (!index) {
    alert       = "";
    alertError  = "";

    String c = "/" + filename;
    if (!(c.length() > 4 && c.lastIndexOf(".cfg") == (c.length() - 4))) c += ".cfg";   // add .cfg extension if not present
/*
    if (SPIFFS.exists(c)) {
      alertError = F("File '");
      alertError += c +  "' already exists. Delete existing configuration before upload again.";
      request->redirect("/configurations");
      DPRINT("%s\n", alertError.c_str());
      return;
    }
*/
    // open the file on first call and store the file handle in the request object
    request->_tempFile = SPIFFS.open(c, FILE_WRITE);

    if (request->_tempFile) {
      DPRINT("Upload start: %s\n", c.c_str());
    }
    else {
      alertError = F("Cannot upload file ");
      alertError += filename +  ".";
      DPRINT("Upload start fail: %s\n", c.c_str());
    }
  }

  // stream the incoming chunk to the opened file
  if (!request->_tempFile || request->_tempFile.write(data,len) != len) {
    alertError = F("Upload of '");
    alertError += filename +  "' failed.";
    DPRINT("Upload fail: %s\n", filename.c_str());
  }

  // if the final flag is set then this is the last frame of data
  if (request->_tempFile && final) {
    DPRINT("Upload end: %s, %u bytes\n", filename.c_str(), index+len);
    request->_tempFile.close();
    alert = F("Upload of '");
    alert += filename +  "' completed.";
  }
}


void http_handle_post_update(AsyncWebServerRequest *request) {

  if (request->arg("action").equals("cloudupdate")) {
    firmwareUpdate = PED_UPDATE_CLOUD;
    request->send(200, "text/plain", "");
  } else if (request->arg("action").equals("fileupdate")) {
    // handler for the /update form POST (once file upload finishes)
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", (Update.hasError()) ? "Update fail!" : "<META http-equiv='refresh' content='15;URL=/update'>Update Success! Rebooting...\n");
    response->addHeader("Connection", "close");
    request->send(response);
    delay(3000);
    ESP.restart();
  }
}

// handler for the file upload, get's the sketch bytes, and writes
// them through the Update object

void http_handle_update_file_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

  //Upload handler chunks in data
  if (!index) {
    // Disconnect, not to interfere with OTA process
#ifdef WEBSOCKET
    webSocket.enable(false);
    webSocket.closeAll();
#endif
    firmwareUpdate = PED_UPDATE_HTTP;
    delay(100);
    DPRINT("Update Start: %s\n", filename.c_str());

    int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
    // Start with max available size
    if (Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
      DPRINT("Update start\n");
#if defined(ARDUINO_LILYGO_T_DISPLAY) || defined(ARDUINO_LILYGO_T_DISPLAY_S3)
    display_clear();
    display_progress_bar_title("OTA Update");
#else
#endif
    }
    else {
      StreamString str;
      Update.printError(str);
      DPRINT("Update start fail: %s", str.c_str());
      firmwareUpdate = PED_UPDATE_NONE;
    }
  }

  if (!Update.hasError()) {
    // Write chunked data to the free sketch space
    if (Update.write(data, len) == len) {
      if (Update.size()) {
        DPRINT("Progress: %5.1f%%\n", 100.0 * Update.progress() / Update.size());
#if defined(ARDUINO_LILYGO_T_DISPLAY) || defined(ARDUINO_LILYGO_T_DISPLAY_S3)
        display_progress_bar_update(Update.progress(), Update.size());
#else
#endif
      }
    }
    else {
      StreamString str;
      Update.printError(str);
      DPRINT("Update fail: %s", str.c_str());
#if defined(ARDUINO_LILYGO_T_DISPLAY) || defined(ARDUINO_LILYGO_T_DISPLAY_S3)
      display_clear();
      display_progress_bar_title2("Fail!", str.c_str());
#else
#endif
    }
  }

  // if the final flag is set then this is the last frame of data
  if (final) {
    if (Update.end(true)) {   //true to set the size to the current progress
      DPRINT("Update Success: %uB\n", index+len);
#if defined(ARDUINO_LILYGO_T_DISPLAY) || defined(ARDUINO_LILYGO_T_DISPLAY_S3)
      display_clear();
      display_progress_bar_title("Success!");
#else
#endif
    } else {
      StreamString str;
      Update.printError(str);
      DPRINT("Update fail: %s", str.c_str());
#if defined(ARDUINO_LILYGO_T_DISPLAY) || defined(ARDUINO_LILYGO_T_DISPLAY_S3)
      display_clear();
      display_progress_bar_title2("Fail!", str.c_str());
#else
#endif
    }
  }
}

void http_handle_not_found(AsyncWebServerRequest *request) {

  if (request->method() == HTTP_OPTIONS) {

    request->send(200);

  } else {

    String message = "File Not Found\n\n";

    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += request->args();
    message += "\n";

    for (uint8_t i = 0; i < request->args(); i++) {
      message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
    }

    request->send(404, "text/plain", message);
  }
}


#ifdef WEBSOCKET
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

  //static bool connected = false;

  if(type == WS_EVT_CONNECT){
    //client connected
    DPRINT("ws[%s][%u] connect\n", server->url(), client->id());
    //client->printf("Hello Client %u :)", client->id());
    //client->ping();
    //client->keepAlivePeriod(1);
    //connected = true;
    wsClient = client;
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    DPRINT("ws[%s][%u] disconnect\n", server->url(), client->id());
    //connected = false;
    wsClient = NULL;
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    DPRINT("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    DPRINT("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      DPRINT("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        DPRINT("%s\n", (char*)data);
      } else {
        for (size_t i = 0; i < info->len; i++) {
          DPRINT("%02x ", data[i]);
        }
        data[info->len-1] = 0;
        DPRINT(" %s\n", (char*)data);
        //DPRINT("%d\n", ESP.getFreeHeap());
        if (strcmp((const char *)data, ".") == 0) {
          //AsyncWebSocketMessageBuffer *buffer = webSocket.makeBuffer(128*64);
          //memcpy(buffer->get(), display.buffer, 128*64);
          //if (connected && buffer) {client->binary(buffer); delete buffer; buffer = NULL;}
          //client->binary(display.buffer, 128*64);
        }
        else if (strcmp((const char *)data, "start") == 0)
          mtc_start();
        else if (strcmp((const char *)data, "stop") == 0)
          mtc_stop();
        else if (strcmp((const char *)data, "continue") == 0)
          mtc_continue();
        else if (strcmp((const char *)data, "tap") == 0)
          mtc_tap();
        else if (strcmp((const char *)data, "clock-master") == 0) {
          MTC.setMode(MidiTimeCode::SynchroClockMaster);
          bpm = (bpm == 0) ? 120 : bpm;
          MTC.setBpm(bpm);
          currentMidiTimeCode = PED_MIDI_CLOCK_MASTER;
        }
        else if (strcmp((const char *)data, "clock-slave") == 0) {
          MTC.setMode(MidiTimeCode::SynchroClockSlave);
          currentMidiTimeCode = PED_MIDI_CLOCK_SLAVE;
          bpm = 0;
        }
        else if (strcmp((const char *)data, "mtc-master") == 0) {
          MTC.setMode(MidiTimeCode::SynchroMTCMaster);
          MTC.sendPosition(0, 0, 0, 0);
          currentMidiTimeCode = PED_MTC_MASTER_24;
        }
        else if (strcmp((const char *)data, "mtc-slave") == 0) {
          MTC.setMode(MidiTimeCode::SynchroMTCSlave);
          currentMidiTimeCode = PED_MTC_SLAVE;
        }
        else if (strcmp((const char *)data, "4/4") == 0) {
          timeSignature = PED_TIMESIGNATURE_4_4;
          MTC.setBeat(4);
        }
        else if (strcmp((const char *)data, "3/4") == 0) {
          timeSignature = PED_TIMESIGNATURE_3_4;
          MTC.setBeat(3);
        }
        else if (strcmp((const char *)data, "2/4") == 0) {
          timeSignature = PED_TIMESIGNATURE_2_4;
          MTC.setBeat(2);
        }
        else if (strcmp((const char *)data, "3/8") == 0) {
          timeSignature = PED_TIMESIGNATURE_3_8;
          MTC.setBeat(3);
        }
        else if (strcmp((const char *)data, "6/8") == 0) {
          timeSignature = PED_TIMESIGNATURE_6_8;
          MTC.setBeat(3);
        }
        else if (strcmp((const char *)data, "9/8") == 0) {
          timeSignature = PED_TIMESIGNATURE_9_8;
          MTC.setBeat(3);
        }
        else if (strcmp((const char *)data, "12/8") == 0) {
          timeSignature = PED_TIMESIGNATURE_12_8;
          MTC.setBeat(3);
        }
        else {
          int b;
          if (sscanf((const char *)data, "bank%d", &b) == 1)
            currentBank = constrain(b, 0, BANKS - 1);
            update_current_step();
            leds_refresh();
        }
      }
      /*
      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
      */
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          DPRINT("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        DPRINT("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      DPRINT("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        data[len] = 0;
        DPRINT("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < len; i++){
          DPRINT("%02x ", data[i]);
        }
        DPRINT("\n");
      }

      if((info->index + len) == info->len){
        DPRINT("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          DPRINT("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          /*
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
          */
        }
      }
    }
  }
}
#endif  // NO_WEBSOCKET


void http_setup() {

#ifdef WEBCONFIG
#ifdef WEBSOCKET
  webSocket.onEvent(onWsEvent);
  httpServer.addHandler(&webSocket);
  //events.setAuthentication("user", "pass");
  httpServer.addHandler(&events);
#endif
/*
  httpServer.serveStatic("/favicon.ico",                SPIFFS, "/favicon.ico").setDefaultFile("/favicon.ico").setCacheControl("max-age=600");
  httpServer.serveStatic("/logo.png",                   SPIFFS, "/logo.png").setDefaultFile("/logo.png").setCacheControl("max-age=600");
  httpServer.serveStatic("/css/bootstrap.min.css",      SPIFFS, "/css/bootstrap.min.css").setDefaultFile("/css/bootstrap.min.css").setCacheControl("max-age=600");
  httpServer.serveStatic("/js/bootstrap.bundle.min.js", SPIFFS, "/js/bootstrap.bundle.min.js").setDefaultFile("/js/bootstrap.bundle.min.js").setCacheControl("max-age=600");
  httpServer.serveStatic("/js/Sortable.min.js",         SPIFFS, "/js/Sortable.min.js").setDefaultFile("/js/Sortable.min.js").setCacheControl("max-age=600");
  httpServer.serveStatic("/schema.json",                SPIFFS, "/schema.json").setDefaultFile("/schema.json").setCacheControl("max-age=600");
  httpServer.serveStatic("/files",                      SPIFFS, "/").setDefaultFile("").setAuthentication(httpUsername.c_str(), httpPassword.c_str());
*/
  httpServer.serveStatic("/favicon.ico",                SPIFFS, "/favicon.ico").setCacheControl("max-age=600");
  httpServer.serveStatic("/logo.png",                   SPIFFS, "/logo.png").setCacheControl("max-age=600");
  httpServer.serveStatic("/css/bootstrap.min.css",      SPIFFS, "/css/bootstrap.min.css").setCacheControl("max-age=600");
  httpServer.serveStatic("/js/bootstrap.bundle.min.js", SPIFFS, "/js/bootstrap.bundle.min.js").setCacheControl("max-age=600");
  httpServer.serveStatic("/js/Sortable.min.js",         SPIFFS, "/js/Sortable.min.js").setCacheControl("max-age=600");
  httpServer.serveStatic("/schema.json",                SPIFFS, "/schema.json").setCacheControl("max-age=600");
  httpServer.serveStatic("/files",                      SPIFFS, "/").setDefaultFile("").setAuthentication(httpUsername.c_str(), httpPassword.c_str());

  httpServer.on("/",                            http_handle_root);
  httpServer.on("/login",           HTTP_GET,   http_handle_login);
  httpServer.on("/login",           HTTP_POST,  http_handle_post_login);
  httpServer.on("/live",            HTTP_GET,   http_handle_live);
  httpServer.on("/live",            HTTP_POST,  http_handle_post_live);
  httpServer.on("/actions",         HTTP_GET,   http_handle_actions);
  httpServer.on("/actions",         HTTP_POST,  http_handle_post_actions);
  httpServer.on("/pedals",          HTTP_GET,   http_handle_pedals);
  httpServer.on("/pedals",          HTTP_POST,  http_handle_post_pedals);
  httpServer.on("/virtualpedals",   HTTP_GET,   http_handle_controls);
  httpServer.on("/virtualpedals",   HTTP_POST,  http_handle_post_controls);
  httpServer.on("/sequences",       HTTP_GET,   http_handle_sequences);
  httpServer.on("/sequences",       HTTP_POST,  http_handle_post_sequences);
  httpServer.on("/interfaces",      HTTP_GET,   http_handle_interfaces);
  httpServer.on("/interfaces",      HTTP_POST,  http_handle_post_interfaces);
  httpServer.on("/options",         HTTP_GET,   http_handle_options);
  httpServer.on("/options",         HTTP_POST,  http_handle_post_options);
  httpServer.on("/configurations",  HTTP_GET,   http_handle_configurations);
  httpServer.on("/configurations",  HTTP_POST,  http_handle_post_configurations, http_handle_configuration_file_upload);

  httpServer.on("/update",          HTTP_GET,   http_handle_update);
  httpServer.on("/update",          HTTP_POST,  http_handle_post_update, http_handle_update_file_upload);
  httpServer.on("/progress",        HTTP_GET,   http_handle_progress);

  httpServer.onNotFound(http_handle_not_found);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  httpServer.begin();

  // Setup a 10Hz timer (every 100 ms)
  Timer2Attach(100);

  // Allocate memory for web page
  page.reserve(WEBPAGE_MEMORY_ALLOCATION);
#endif  // WEBCONFIG
}

#endif  // WEBCONFIG

inline void http_run() {

  if (startWebServer) {
    startWebServer = false;
    http_setup();
    DPRINT("HTTP server started on port 80\n");
    DPRINT("Connect to http://%s.local/update for firmware update\n", host.c_str());
    DPRINT("Connect to http://%s.local for configuration\n", host.c_str());
  }

  if (interruptCounter2 > 0) {

    interruptCounter2 = 0;

#ifdef WEBSOCKET
    //webSocket.binaryAll(display.buffer, 128*64);
#if defined(ARDUINO_LILYGO_T_DISPLAY) || defined(ARDUINO_LILYGO_T_DISPLAY_S3)
#else
    if (wsClient) wsClient->binary(display.buffer, 128*64);
#endif
     // Limits the number of clients by closing the oldest client
     // when the maximum number of clients has been exceeded
    webSocket.cleanupClients();
#endif

/*
    if (!buffer) {
      buffer = webSocket.makeBuffer(128*64);
      memcpy(buffer->get(), display.buffer, 128*64);
    }
    */
  }
}

#endif  // WIFI
/* AquaOS 1.0 - minik kabuk isletim sistemi (Arduino / AVR uyumlu, tasinabilir)
   Seri monitor: 9600 baud, satir sonu: Newline veya CR.
   50 komut: sistem, LED/GPIO, matematik, bellek, EEPROM dosya sistemi,
   50 parametreli egitilebilir mini yapay zeka (3 giris - 8 gizli - 2 cikis).
*/
#include <Arduino.h>
#include <EEPROM.h>

#define P(s)  Serial.print(F(s))
#define PL(s) Serial.println(F(s))
#define FS0 64          // dosya sistemi EEPROM baslangici
#define NSL 32          // en fazla dosya sayisi (16 bayt/slot)

// Komut listesi (sira, switch icindeki numaralarla ayni olmali)
const char CMDS[] PROGMEM =
  "help ver uname whoami hostname uptime free clear reboot echo "
  "on off toggle blink pwm fade sos strobe pinmode dw dr ar pulse sleep rand "
  "add sub mul div mod hex bin seq peek poke eer eew ls cat save rm "
  "ai-add ai-train ai-run ai-show ai-reset ai-save ai-load pwd df";

char buf[40];
uint8_t blen = 0;
char *av[7];
uint8_t ac = 0;

int8_t W[50];       // AI agirliklari: W1[8][3]=0..23, B1[8]=24..31, W2[2][8]=32..47, B2[2]=48..49
int8_t S[8][5];     // egitim ornekleri: 3 giris + 2 hedef
uint8_t nS = 0;

// ---------- yardimcilar ----------
long N(uint8_t i, long d = 0) { return i < ac ? atol(av[i]) : d; }

uint8_t PN(uint8_t i, uint8_t d) {
  if (i >= ac) return d;
  char c = av[i][0];
  if (c == 'a' || c == 'A') return A0 + atoi(av[i] + 1);
  return atoi(av[i]);
}

int freeRam() {
#ifdef __AVR__
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
#else
  return -1;
#endif
}

int8_t findCmd(const char *w) {
  uint8_t idx = 0, i = 0;
  bool ok = true;
  const char *p = CMDS;
  for (;;) {
    char c = pgm_read_byte(p++);
    if (c == ' ' || c == 0) {
      if (ok && w[i] == 0) return idx;
      if (c == 0) return -1;
      idx++; i = 0; ok = true;
    } else {
      if (ok && w[i] == c) i++; else ok = false;
    }
  }
}

void pl(uint8_t p, int ms) {
  digitalWrite(p, HIGH); delay(ms);
  digitalWrite(p, LOW);  delay(ms);
}

void doReboot() {
#if defined(__AVR__)
  ((void (*)())0)();
#elif defined(ESP8266) || defined(ESP32)
  ESP.restart();
#else
  PL("desteklenmiyor");
#endif
}

// ---------- EEPROM dosya sistemi ----------
uint8_t nsl() {
  int n = ((int)EEPROM.length() - FS0) / 16;
  return n > NSL ? NSL : (n < 0 ? 0 : n);
}

bool slotUsed(uint8_t i) {
  uint8_t c = EEPROM.read(FS0 + i * 16);
  return c != 0 && c != 255;
}

int8_t fsFind(const char *n) {
  for (uint8_t i = 0; i < nsl(); i++) {
    if (!slotUsed(i)) continue;
    int a = FS0 + i * 16;
    uint8_t j = 0;
    while (j < 5) {
      char s = EEPROM.read(a + j);
      if (s != n[j]) break;
      if (!s) { j = 5; break; }
      j++;
    }
    if (j == 5) return i;
  }
  return -1;
}

void putStr(int a, const char *s, uint8_t m) {
  uint8_t k = 0;
  for (; k < m && s[k]; k++) EEPROM.update(a + k, s[k]);
  for (; k <= m; k++) EEPROM.update(a + k, 0);
}

// ---------- mini yapay zeka ----------
void fwd(const int8_t *x, int16_t *o) {
  int8_t h[8];
  for (uint8_t j = 0; j < 8; j++) {
    long s = 0;
    for (uint8_t i = 0; i < 3; i++) s += (long)W[j * 3 + i] * x[i];
    s = (s >> 4) + W[24 + j];
    h[j] = s < 0 ? 0 : (s > 127 ? 127 : s);
  }
  for (uint8_t k = 0; k < 2; k++) {
    long s = 0;
    for (uint8_t j = 0; j < 8; j++) s += (long)W[32 + k * 8 + j] * h[j];
    s = (s >> 4) + W[48 + k];
    o[k] = s < -127 ? -127 : (s > 127 ? 127 : s);
  }
}

long aiErr() {
  long e = 0;
  int16_t o[2];
  for (uint8_t i = 0; i < nS; i++) {
    fwd(S[i], o);
    e += abs(o[0] - S[i][3]) + abs(o[1] - S[i][4]);
  }
  return e;
}

void aiReset() {
  randomSeed(analogRead(A0) + micros());
  for (uint8_t i = 0; i < 50; i++) W[i] = random(-40, 41);
  nS = 0;
}

// ---------- komut calistirici ----------
void run() {
  ac = 0;
  char *p = buf;
  while (*p && ac < 7) {
    while (*p == ' ') p++;
    if (!*p) break;
    av[ac++] = p;
    while (*p && *p != ' ') p++;
    if (*p) *p++ = 0;
  }
  if (!ac) return;

  switch (findCmd(av[0])) {
    case 0: Serial.println((const __FlashStringHelper *)CMDS); break;
    case 1: PL("AquaOS 1.0"); break;
    case 2: P("AquaOS 1.0 "); Serial.print(F_CPU / 1000000UL); PL("MHz"); break;
    case 3: PL("root"); break;
    case 4: PL("aqua"); break;
    case 5: Serial.print(millis() / 1000); PL("s"); break;
    case 6: P("bos RAM: "); Serial.print(freeRam()); PL(" B"); break;
    case 7: Serial.write(27); P("[2J"); Serial.write(27); P("[H"); break;
    case 8: doReboot(); break;
    case 9:
      for (uint8_t i = 1; i < ac; i++) { Serial.print(av[i]); Serial.print(' '); }
      Serial.println();
      break;

    // --- LED / GPIO ---
    case 10: { uint8_t q = PN(1, 13); pinMode(q, OUTPUT); digitalWrite(q, HIGH); } break;
    case 11: { uint8_t q = PN(1, 13); pinMode(q, OUTPUT); digitalWrite(q, LOW); } break;
    case 12: { uint8_t q = PN(1, 13); pinMode(q, OUTPUT); digitalWrite(q, !digitalRead(q)); } break;
    case 13: {
      uint8_t q = PN(1, 13); uint8_t n = N(2, 5); int ms = N(3, 200);
      pinMode(q, OUTPUT);
      for (uint8_t i = 0; i < n; i++) pl(q, ms);
    } break;
    case 14: { uint8_t q = PN(1, 9); pinMode(q, OUTPUT); analogWrite(q, constrain(N(2, 128), 0, 255)); } break;
    case 15: {
      uint8_t q = PN(1, 9); pinMode(q, OUTPUT);
      for (int v = 0; v <= 255; v += 5) { analogWrite(q, v); delay(10); }
      for (int v = 255; v >= 0; v -= 5) { analogWrite(q, v); delay(10); }
    } break;
    case 16: {
      uint8_t q = PN(1, 13); pinMode(q, OUTPUT);
      for (uint8_t r = 0; r < 3; r++) {
        int d = (r == 1) ? 600 : 150;
        for (uint8_t i = 0; i < 3; i++) pl(q, d);
        delay(300);
      }
    } break;
    case 17: {
      uint8_t q = PN(1, 13); uint8_t n = N(2, 20);
      pinMode(q, OUTPUT);
      for (uint8_t i = 0; i < n; i++) pl(q, 10);
    } break;
    case 18: {
      uint8_t q = PN(1, 13);
      char m = ac > 2 ? av[2][0] : 'o';
      pinMode(q, m == 'i' ? INPUT : (m == 'p' ? INPUT_PULLUP : OUTPUT));
    } break;
    case 19: { uint8_t q = PN(1, 13); pinMode(q, OUTPUT); digitalWrite(q, N(2) ? HIGH : LOW); } break;
    case 20: Serial.println(digitalRead(PN(1, 2))); break;
    case 21: Serial.println(analogRead(PN(1, A0))); break;
    case 22: {
      uint8_t q = PN(1, 13); pinMode(q, OUTPUT);
      digitalWrite(q, HIGH); delay(N(2, 100)); digitalWrite(q, LOW);
    } break;
    case 23: delay(N(1, 1000)); break;
    case 24: Serial.println(random(N(1, 100))); break;

    // --- matematik ---
    case 25: Serial.println(N(1) + N(2)); break;
    case 26: Serial.println(N(1) - N(2)); break;
    case 27: Serial.println(N(1) * N(2)); break;
    case 28: if (N(2)) Serial.println(N(1) / N(2)); else PL("sifira bolme"); break;
    case 29: if (N(2)) Serial.println(N(1) % N(2)); else PL("sifira bolme"); break;
    case 30: Serial.println(N(1), HEX); break;
    case 31: Serial.println(N(1), BIN); break;
    case 32: {
      long a = N(1, 1), b = N(2, 10);
      if (b - a > 50) b = a + 50;
      for (long i = a; i <= b; i++) { Serial.print(i); Serial.print(' '); }
      Serial.println();
    } break;

    // --- bellek / EEPROM ---
    case 33: Serial.println(*(volatile uint8_t *)(uintptr_t)N(1)); break;
    case 34: *(volatile uint8_t *)(uintptr_t)N(1) = N(2); break;
    case 35: Serial.println(EEPROM.read(N(1))); break;
    case 36: if (N(1) < (long)EEPROM.length()) EEPROM.update(N(1), N(2)); break;

    // --- dosya sistemi ---
    case 37:
      for (uint8_t i = 0; i < nsl(); i++) {
        if (!slotUsed(i)) continue;
        for (uint8_t k = 0; k < 5; k++) { char c = EEPROM.read(FS0 + i * 16 + k); if (c) Serial.print(c); }
        Serial.println();
      }
      break;
    case 38: {
      int8_t i = ac > 1 ? fsFind(av[1]) : -1;
      if (i < 0) { PL("dosya yok"); break; }
      for (uint8_t k = 6; k < 16; k++) { char c = EEPROM.read(FS0 + i * 16 + k); if (!c) break; Serial.print(c); }
      Serial.println();
    } break;
    case 39: {
      if (ac < 3) { PL("kullanim: save ad metin"); break; }
      for (char *q = av[2]; q < buf + blen; q++) if (!*q) *q = ' ';
      int8_t i = fsFind(av[1]);
      if (i < 0) for (uint8_t k = 0; k < nsl(); k++) if (!slotUsed(k)) { i = k; break; }
      if (i < 0) { PL("dolu"); break; }
      int a = FS0 + i * 16;
      putStr(a, av[1], 5);
      putStr(a + 6, av[2], 9);
    } break;
    case 40: {
      int8_t i = ac > 1 ? fsFind(av[1]) : -1;
      if (i < 0) PL("dosya yok"); else EEPROM.update(FS0 + i * 16, 0);
    } break;

    // --- yapay zeka ---
    case 41:
      if (ac < 6 || nS >= 8) { PL("ai-add g1 g2 g3 h1 h2 (max 8 ornek)"); break; }
      for (uint8_t k = 0; k < 5; k++) S[nS][k] = constrain(N(k + 1), -100, 100);
      nS++;
      Serial.println(nS);
      break;
    case 42: {
      if (!nS) { PL("once ai-add ile veri ekle"); break; }
      uint16_t n = constrain(N(1, 500), 1, 5000);
      long best = aiErr();
      for (uint16_t t = 0; t < n && best > 0; t++) {
        uint8_t i = random(50);
        int8_t old = W[i];
        W[i] = constrain(old + (int)random(-8, 9), -127, 127);
        long e = aiErr();
        if (e <= best) best = e; else W[i] = old;
      }
      P("hata: "); Serial.println(best);
    } break;
    case 43: {
      int8_t x[3];
      int16_t o[2];
      for (uint8_t k = 0; k < 3; k++) x[k] = constrain(N(k + 1), -100, 100);
      fwd(x, o);
      Serial.print(o[0]); Serial.print(' '); Serial.println(o[1]);
    } break;
    case 44:
      for (uint8_t i = 0; i < 50; i++) {
        Serial.print(W[i]); Serial.print(' ');
        if (i % 10 == 9) Serial.println();
      }
      P("ornek: "); Serial.println(nS);
      break;
    case 45: aiReset(); PL("sifirlandi"); break;
    case 46:
      for (uint8_t i = 0; i < 50; i++) EEPROM.update(i, W[i]);
      EEPROM.update(50, 0xA5);
      PL("kaydedildi");
      break;
    case 47:
      if (EEPROM.read(50) != 0xA5) { PL("kayit yok"); break; }
      for (uint8_t i = 0; i < 50; i++) W[i] = EEPROM.read(i);
      PL("yuklendi");
      break;

    case 48: PL("/"); break;
    case 49: {
      uint8_t u = 0;
      for (uint8_t i = 0; i < nsl(); i++) if (slotUsed(i)) u++;
      P("EEPROM "); Serial.print(EEPROM.length());
      P(" B, dosya "); Serial.print(u); P("/"); Serial.println(nsl());
    } break;

    default: P("bulunamadi: "); Serial.println(av[0]); break;
  }
}

void prompt() { P("aqua> "); }

void setup() {
  Serial.begin(9600);
  aiReset();
  PL("\nAquaOS 1.0 - 'help' yaz");
  prompt();
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (c == '\n' && !blen) continue;
      if (blen) { buf[blen] = 0; run(); blen = 0; }
      prompt();
    } else if (c == 8 || c == 127) {
      if (blen) blen--;
    } else if (blen < sizeof(buf) - 1) {
      buf[blen++] = c;
    }
  }
}

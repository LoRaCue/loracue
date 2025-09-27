# LoRaCue: Enterprise-Grade Blueprint für einen Präsentations-Clicker mit großer Reichweite auf ESP32-Basis

## Executive Summary: Eine Enterprise-Grade-Architektur für einen Präsentations-Clicker mit großer Reichweite

Dieses Dokument beschreibt einen umfassenden, produktionsreifen
Blueprint für die Entwicklung von **LoRaCue**, einem hochzuverlässigen
Präsentations-Clicker mit großer Reichweite. Die vorgeschlagene Lösung
basiert auf Heltec LoRa V3 (ESP32-S3) Entwicklungsboards, die als
Sender- und Empfängereinheit fungieren. Die Kernarchitektur stützt sich
auf drei strategische Säulen: die ausschließliche Verwendung des
Espressif IoT Development Framework (ESP-IDF) für maximale Leistung und
Kontrolle, die Implementierung eines proprietären, latenzarmen
LoRa-Protokolls mit robuster AES-Verschlüsselung und eine durchdachte
Hardware- und Softwarearchitektur, die auf Zuverlässigkeit, Sicherheit
und Wartbarkeit ausgelegt ist. Dieser Blueprint priorisiert systematisch
die Anforderungen von Unternehmensanwendungen, einschließlich sicherer
Pairing-Mechanismen, mehrerer Update-Pfade (OTA, USB) und einheitlicher
Konfigurationsschnittstellen (OLED, Web, CLI). Das Dokument ist als
finale technische Spezifikation konzipiert und bereit für die
Implementierung.

## 1. Systemarchitektur und Technologie-Stack

### 1.1. High-Level-Systemübersicht

Die Systemarchitektur besteht aus funktional identischen
Hardware-Einheiten, die durch Software in unterschiedliche Rollen
konfiguriert werden. Die Topologie ist als **One-to-Many-System**
ausgelegt, das aus einer zentralen Empfängereinheit (PC-Modul) und
mehreren Sendereinheiten (STAGE-Module) besteht.

- **STAGE-Einheit (Transmitter):** Diese Einheit wird von einem Akku
  gespeist und verfügt über physische Tasten zur Steuerung der
  Präsentation, ein OLED-Display zur Statusanzeige (Verbindungsqualität,
  Akkustand) und das LoRa-Funkmodul zur Übertragung von Befehlen.

- **PC-Einheit (Receiver):** Diese Einheit wird über den USB-Anschluss
  eines Host-Computers mit Strom versorgt. Sie empfängt die LoRa-Befehle
  von allen gekoppelten STAGE-Einheiten, entschlüsselt sie und übersetzt
  sie in Standard-USB-HID-Tastatureingaben (Human Interface Device), die
  von jeder Präsentationssoftware nativ verstanden werden.

Die Kommunikation zwischen den Einheiten erfolgt über ein speziell
entwickeltes, verschlüsseltes und latenzarmes LoRa-Protokoll, das eine
sichere und reaktionsschnelle Übertragung über Distanzen gewährleistet,
die weit über die von Standard-Bluetooth oder 2.4-GHz-Funktechnologien
hinausgehen.

Das folgende Diagramm veranschaulicht die Architektur:

Code-Snippet

graph TD\
subgraph Transmitter (Clicker)\
A \-- GPIO \--\> B{ESP32-S3\<br\>(ESP-IDF)};\
B \-- I2C \<\--\> C;\
B \-- SPI \<\--\> D;\
end\
\
subgraph Receiver (USB Dongle)\
E \-- SPI \<\--\> F{ESP32-S3\<br\>(ESP-IDF)};\
F \-- USB HID \--\> G\[Host PC\];\
end\
\
D \-- Proprietäres, verschlüsseltes\<br\>Low-Latency LoRa Protokoll
\--\> E;\
E \-- ACK \--\> D;

### 1.2. Framework-Entscheidung: ESP-IDF

Als Grundlage für dieses Projekt wird das Espressif IoT Development
Framework (ESP-IDF) verwendet. Diese Entscheidung ist strategisch, um
die für ein Enterprise-Produkt erforderliche Leistung, Stabilität und
Sicherheit zu gewährleisten. Das ESP-IDF bietet direkten Zugriff auf die
Hardware-Peripherie, ein Echtzeitbetriebssystem (FreeRTOS) für
deterministisches Multitasking und robuste, integrierte Funktionen für
Sicherheit und Over-the-Air (OTA) Updates.

Beim Setup ist zu beachten, dass eine reine ESP-IDF-Toolchain ohne
Arduino-Kompatibilitätsschichten verwendet wird, um maximale Stabilität
zu gewährleisten. Die Konfiguration des Projekts erfolgt über das
menuconfig-Werkzeug, das eine nachvollziehbare und reproduzierbare
Konfiguration aller Systemparameter, einschließlich der
Partitionstabellen für OTA-Updates, ermöglicht.

### 1.3. Auswahl der Kernkomponenten (ESP-IDF-Ökosystem)

Um die Stabilität und Wartbarkeit des Projekts zu maximieren, wird eine
reine ESP-IDF-Toolchain ohne Arduino-Kompatibilitätsschichten verwendet.
Für die Ansteuerung der Kernkomponenten werden folgende, native ESP-IDF
Treiber eingesetzt:

- **LoRa-Treiber:** Für den SX1262-Chip wird der native
  ESP-IDF-C-Treiber nopnop2002/esp-idf-sx126x eingesetzt.^1^ Es
  existieren Alternativen wie die C++-basierte Universalbibliothek
  RadioLib ^2^ oder andere C-Treiber.^3^ Die Entscheidung für\
  nopnop2002/esp-idf-sx126x ist strategisch: Als reiner C-Treiber, der
  speziell für ESP-IDF entwickelt wurde, vermeidet er potenzielle
  Konflikte und den Overhead von C++-Abstraktionen oder
  Arduino-Kompatibilitätsschichten.^1^ Dies gewährleistet die maximale
  Leistung und Stabilität, die für dieses Enterprise-Projekt
  erforderlich ist.

- **USB-Stack:** Für die Implementierung des USB-Geräts am Empfänger
  wird zwingend die offizielle TinyUSB-Komponente des ESP-IDF verwendet.
  Diese ist gut dokumentiert, wird von Espressif unterstützt und bietet
  die notwendige Funktionalität zur Erstellung eines Composite-Geräts.

- **Display-Treiber:** Zur Ansteuerung des SH1106-OLED-Displays wird der
  native ESP-IDF-Treiber tny-robotics/sh1106-esp-idf verwendet. Dies
  gewährleistet eine nahtlose Integration in die
  ESP-IDF-Grafik-Subsysteme und vermeidet Kompatibilitätsprobleme.

## 2. Hardware-Abstraktion und Board Support Package (BSP)

Ein dediziertes Board Support Package (BSP) wird als
Hardware-Abstraktionsschicht erstellt. Dieses Modul kapselt alle
hardwarespezifischen Details und stellt dem Rest der Anwendung eine
saubere, plattformunabhängige API zur Verfügung. Während der MVP auf das
Heltec LoRa V3 Board abzielt, ermöglicht dieser architektonische Ansatz
die zukünftige Unterstützung anderer ESP32-basierter Boards durch die
einfache Erstellung eines neuen, hardwarespezifischen BSP, ohne die
Kernanwendungslogik ändern zu müssen.

### 2.1. Heltec LoRa V3 Pinout und Ressourcenzuweisung

Basierend auf den offiziellen Schaltplänen ^4^ und
Community-Erkenntnissen ^5^ wird die folgende Pin-Belegung für die
Anwendung festgelegt:

- **SPI-Bus (SPI2_HOST):** Exklusiv für den SX1262 LoRa-Chip reserviert.

  - MOSI: GPIO11

  - MISO: GPIO10

  - SCK: GPIO9

  - CS (Chip Select): GPIO8

  - BUSY: GPIO13

  - DIO1 (IRQ): GPIO14

  - RESET: GPIO12

- **I2C-Bus (I2C0):** Exklusiv für das SH1106 OLED-Display reserviert.

  - SDA: GPIO17

  - SCL: GPIO18

- **GPIO-Eingänge (Transmitter):**

  - Button \'Next\': GPIO45

  - Button \'Previous/Black Screen\': GPIO46

  - Die internen Pull-up-Widerstände des ESP32-S3 werden für diese Pins
    aktiviert.

- **ADC-Eingang (Transmitter):**

  - Batteriespannungsmessung: GPIO1 (ADC1_CHANNEL_0).^7^

- **GPIO-Ausgang (Transmitter):**

  - Steuerung des Spannungsteilers für die Batteriemessung: GPIO37.^9^

### 2.2. Konfiguration der Peripherietreiber (ESP-IDF)

Das BSP wird Initialisierungsroutinen enthalten, die die ESP-IDF-Treiber
für die oben genannten Peripheriegeräte konfigurieren.

- **SPI-Treiber:** Initialisierung des SPI2_HOST-Busses mit der
  korrekten Taktfrequenz und dem richtigen SPI-Modus für den SX1262.

- **I2C-Treiber:** Initialisierung des I2C0-Busses für die Kommunikation
  mit dem SH1106-Display.

- **GPIO-Treiber:** Konfiguration der Tasten-Pins als Eingänge mit
  aktivierten Pull-ups. Es werden GPIO-Interrupts konfiguriert, um auf
  Tastendrücke sofort und energieeffizient reagieren zu können, anstatt
  die Pins kontinuierlich abzufragen (Polling).

### 2.3. Energiemanagement und Batterieüberwachung

Das Heltec-Board verfügt über ein integriertes
Lithium-Batterie-Managementsystem, das Lade- und Entladevorgänge
steuert. Die Firmware nutzt dies für eine detaillierte Überwachung.

Die Batterieüberwachung erfordert eine spezifische Ansteuerungssequenz,
die im BSP gekapselt wird. Widersprüchliche Berichte in Community-Foren
über die korrekte Logik des Steuerpins GPIO37 ^9^ deuten auf mögliche
Hardware-Revisionen hin. Ein Enterprise-Produkt muss die Variabilität
über eine gesamte Produktionsserie berücksichtigen. Die korrekte
Vorgehensweise besteht darin, den Mechanismus nicht auf Annahmen zu
stützen, sondern eine robuste Routine zu implementieren, die gegen den
spezifischen Schaltplan der verwendeten Board-Revision verifiziert
wird.^4^

Die implementierte Prozedur wird wie folgt aussehen:

1. Setzen von GPIO37 auf den korrekten Zustand (LOW oder HIGH, je nach
    Revision), um den Spannungsteiler für die Messung zu aktivieren.

2. Eine kurze Wartezeit von einigen Millisekunden, um eine
    Stabilisierung der Spannung zu ermöglichen.

3. Durchführung mehrerer ADC-Messungen am GPIO1.

4. Mittelwertbildung der Messwerte, um Rauschen zu reduzieren und die
    Genauigkeit zu erhöhen.

5. Rückversetzung von GPIO37 in einen hochohmigen Zustand (oder den
    Ruhezustand), um den Stromverbrauch durch den Spannungsteiler zu
    unterbinden und die Batterie zu schonen.^10^

Die Umrechnung des rohen 12-Bit-ADC-Wertes (ADC_raw) in die tatsächliche
Batteriespannung (V_BAT) erfolgt über die folgende Formel, die den
390kΩ/100kΩ-Spannungsteiler ^7^ und die Referenzspannung des ADC (

V_ref, typ. 3.3V) berücksichtigt:

\$\$ V\_{BAT} = \\frac{ADC\_{raw}}{4095} \\cdot V\_{ref} \\cdot
\\frac{R1+R2}{R2} = \\frac{ADC\_{raw}}{4095} \\cdot 3.3V \\cdot
\\frac{390k\\Omega + 100k\\Omega}{100k\\Omega} \$\$

VBAT​=4095ADCraw​​⋅3.3V⋅4.9

Zusätzlich wird der Ladezustand durch die Überwachung der VBUS-Spannung
am USB-Anschluss erkannt.^7^ Wenn VBUS aktiv ist (d.h. das Gerät ist an
eine Stromquelle angeschlossen), wird dies als Ladezustand interpretiert
und auf dem Display angezeigt.

### 2.4. Architekturentscheidung: Point-to-Point vs. Mesh-Netzwerk

Für diese Anwendung wurde bewusst eine Point-to-Point-Architektur
anstelle eines Mesh-Netzwerks gewählt. Während Mesh-Netzwerke wie
LoRaMesher für bestimmte Anwendungsfälle (z. B. dezentrale Kommunikation
über große Gebiete ohne feste Infrastruktur) hervorragend geeignet sind,
sind sie für den vorliegenden Anwendungsfall eines
Präsentations-Clickers kontraproduktiv. Die Gründe dafür sind:

- **Latenz:** Die wichtigste Anforderung ist eine Latenz von unter 100
  ms. Mesh-Netzwerke führen naturgemäß eine erhebliche Latenz ein, da
  jede Weiterleitung (Hop) eines Pakets von einem Knoten zum nächsten
  Zeit für Empfang, Verarbeitung und erneutes Senden benötigt. Schon ein
  einziger Hop kann die Latenz über das akzeptable Maß hinaus erhöhen
  und die Benutzererfahrung ruinieren.

- **Komplexität und Ressourcen:** Mesh-Protokolle fügen eine erhebliche
  Schicht an Komplexität, Code-Größe und Speicherbedarf hinzu. Dies
  steht im Widerspruch zum Ziel, eine schlanke, robuste und wartbare
  Firmware zu erstellen.

- **Anwendungsfall:** Das Szenario (Bühne zu FOH-Pult) ist ein
  klassischer Point-to-Point-Fall. Es gibt keine dazwischenliegenden
  Knoten, die als Repeater fungieren könnten oder müssten. Die
  erforderliche Reichweite von einigen hundert Metern wird durch eine
  optimierte LoRa-Konfiguration problemlos direkt erreicht.

Daher ist eine direkte, optimierte Point-to-Point-Verbindung mit einem
einfachen ACK-Mechanismus die überlegene Architektur, um die
Kernanforderungen an Latenz, Zuverlässigkeit und Effizienz zu erfüllen.

## 3. Transmitter-Einheit: Firmware-Blueprint

Die Firmware des Transmitters ist auf maximale Reaktionsfähigkeit und
minimale Leistungsaufnahme ausgelegt. Dies wird durch eine
ereignisgesteuerte Architektur mit mehreren FreeRTOS-Tasks erreicht.

### 3.1. Echtzeit-Task-Architektur (FreeRTOS)

Drei Haupttasks arbeiten parallel und kommunizieren über thread-sichere
FreeRTOS-Queues, um eine klare Trennung der Verantwortlichkeiten zu
gewährleisten und blockierende Operationen zu vermeiden.

- **input_task (Hohe Priorität):** Dieser Task wartet blockierungsfrei
  auf GPIO-Interrupts, die durch Tastendrücke ausgelöst werden. Nach dem
  Auslösen entprellt er das Signal softwareseitig, um
  Mehrfacherkennungen zu vermeiden, und sendet eine entsprechende
  Befehlsnachricht (z.B. CMD_NEXT, CMD_PREV_LONG) an eine Queue, die vom
  lora_task gelesen wird. Die hohe Priorität stellt sicher, dass
  Benutzereingaben immer sofort verarbeitet werden.

- **ui_task (Mittlere Priorität):** Dieser Task ist ausschließlich für
  die Verwaltung des OLED-Displays zuständig. Er wartet auf
  Statusnachrichten (z.B. neuer Akkustand, Bestätigungsempfang,
  Pairing-Status) von anderen Tasks über eine dedizierte UI-Queue.
  Dadurch wird die UI-Logik vollständig von der Kernanwendungslogik
  entkoppelt, was die Wartbarkeit verbessert.

- **lora_task (Normale Priorität):** Dies ist der zentrale
  Kommunikationshandler. Er wartet auf Befehlsnachrichten vom
  input_task. Wenn ein Befehl eintrifft, konstruiert, verschlüsselt und
  sendet er das LoRa-Paket. Anschließend wartet er auf eine Bestätigung
  (ACK) vom Empfänger. Dieser Task ist auch für periodische Aufgaben
  zuständig, wie das Senden von Keep-Alive-Pings bei Inaktivität und das
  Anfordern von Batterie-Updates für die UI.

### 3.2. Design des latenzarmen LoRa-Protokolls

Die Anforderungen \"große Reichweite\" und \"niedrige Latenz\" stehen
bei LoRa in einem direkten Zielkonflikt. Große Reichweite wird durch
hohe Spreading Factors (SF) erreicht, die jedoch die Sendezeit (Time on
Air, ToA) und damit die Latenz drastisch erhöhen.^11^ Für einen
Präsentations-Clicker ist eine Latenz von unter 50 ms entscheidend für
eine gute Benutzererfahrung. Ein hoher SF-Wert wie SF12 würde zu einer
Latenz von über einer Sekunde führen und das Produkt unbrauchbar
machen.^12^ Daher muss ein datengesteuerter Kompromiss gefunden werden,
der eine deutlich höhere Reichweite als Bluetooth bietet, aber die
Latenz priorisiert.

#### 3.2.1. Optimierung der physikalischen Schicht

Die folgenden LoRa-Parameter werden gewählt, um die Time on Air zu
minimieren:

- **Spreading Factor (SF):** 7. Dies bietet den besten Kompromiss aus
  Geschwindigkeit und Reichweite für diese Anwendung. SF6 könnte
  ebenfalls in Betracht gezogen werden, erfordert aber oft einen
  stabileren Oszillator (TCXO).^11^

- **Bandbreite (BW):** 500 kHz. Die größtmögliche Bandbreite wird
  verwendet, um die Symboldauer zu verkürzen.

- **Coding Rate (CR):** 4/5. Die geringste Vorwärtsfehlerkorrektur wird
  eingesetzt, um die redundanten Daten und damit die Sendezeit zu
  reduzieren.

Die folgende Tabelle quantifiziert die Auswirkungen dieser Wahl auf die
Latenz.

**Tabelle 2: LoRa-Parameter-Abwägungen für die
Präsentations-Clicker-Anwendung**

  ------------------------------------------------------------------------------------------
  Spreading   Bandbreite   Coding Rate Payload (16 Berechnete   Relative
  Factor                               Bytes)      Time on Air  Reichweite/Empfindlichkeit
                                                   (ToA)
  ----------- ------------ ----------- ----------- ------------ ----------------------------
  12          125 kHz      4/5         16 Bytes    ca. 1483 ms  Sehr hoch
                                                   ^12^

  9           125 kHz      4/5         16 Bytes    ca. 206 ms   Hoch
                                                   ^12^

  **7**       **500 kHz**  **4/5**     **16        **ca. 3.7    **Mittel (optimiert für
                                       Bytes**     ms**         Latenz)**

7           125 kHz      4/5         16 Bytes    ca. 62 ms    Mittel-Hoch
                                                   ^12^
  ------------------------------------------------------------------------------------------

Die gewählte Konfiguration (SF7/BW500kHz) resultiert in einer extrem
niedrigen Time on Air von unter 4 ms, was eine exzellente
Reaktionsfähigkeit gewährleistet und gleichzeitig eine Reichweite
bietet, die die von 2.4-GHz-Technologien bei weitem übertrifft.

#### 3.2.2. Struktur des benutzerdefinierten LoRa-Pakets

Ein einfaches \"Befehl senden\"-Protokoll ist für eine
Unternehmensanwendung unzureichend, da es anfällig für Replay-Angriffe
ist und keine Zuverlässigkeit bietet. Daher wird eine strukturierte
Paketdefinition verwendet, die Sicherheit und Zuverlässigkeit
gewährleistet.

**Tabelle 3: Struktur des benutzerdefinierten LoRa-Pakets**

  ----------------------------------------------------------------------------
  Feld                    Größe (Bytes)           Beschreibung
  ----------------------- ----------------------- ----------------------------
  DeviceID                2                       Eine eindeutige ID, die
                                                  während des Pairings
                                                  zugewiesen wird. Sie ist
                                                  entscheidend, um Nachrichten
                                                  von mehreren STAGE-Sendern
                                                  zu unterscheiden.

  SequenceNum             2                       Eine bei jeder Übertragung
                                                  inkrementierte Sequenznummer
                                                  zur Verhinderung von
                                                  Replay-Angriffen.

  Command                 1                       Der eigentliche Befehl (z.B.
                                                  0x01 für \'Next Slide\').

  Payload                 0-7                     Variable Nutzdaten,
                                                  reserviert für zukünftige
                                                  Erweiterungen (z.B.
                                                  Laserpointer-Koordinaten).

MAC                     4                       Ein Message Authentication
                                                  Code (z.B. ein gekürzter
                                                  HMAC-SHA256-Hash) zur
                                                  Sicherstellung der
                                                  Integrität und Authentizität
                                                  der Nachricht
  ----------------------------------------------------------------------------

Die Gesamtgröße des Pakets wird bewusst klein gehalten, um die Time on
Air weiter zu minimieren.

#### 3.2.3. Verbindungsmanagement

Es wird ein einfacher Stop-and-Wait-ACK-Mechanismus implementiert. Nach
dem Senden eines Befehls lauscht der Sender für eine kurze Zeit (z.B.
100 ms) auf ein ACK-Paket vom Empfänger. Wenn innerhalb dieses
Zeitfensters kein ACK empfangen wird, wird der Befehl bis zu zweimal
erneut gesendet. Dies stellt die zuverlässige Zustellung auch in
Umgebungen mit Funkstörungen sicher.

### 3.3. Sicherheitsimplementierung

Die Sicherheit der Kommunikation ist von größter Bedeutung.

- **Verschlüsselung:** Der Kern des Sicherheitsmodells ist die
  Verschlüsselung der Paketnutzdaten (von SequenceNum bis Payload) mit
  AES-128. Hierfür wird der hardwarebeschleunigte AES-Peripheriechip des
  ESP32-S3 genutzt.^13^ Die Verwendung der Hardwarebeschleunigung über
  die\
  mbedtls- oder hwcrypto-APIs von ESP-IDF stellt sicher, dass die
  Verschlüsselung nur minimale Auswirkungen auf Latenz und
  Stromverbrauch hat.^15^

- **Schlüsselmanagement:** Ein eindeutiger 128-Bit-AES-Sitzungsschlüssel
  wird während eines einmaligen, sicheren Pairing-Prozesses generiert
  und ausgetauscht (siehe Abschnitt 5.1). Jedes gekoppelte STAGE-Gerät
  teilt einen eindeutigen Schlüssel mit dem PC-Gerät.

### 3.4. Benutzeroberfläche und Benutzererfahrung (UI/UX)

Das OLED-Display liefert dem Benutzer klares und unmittelbares Feedback.
Die Implementierung erfolgt durch direkte Manipulation des Framebuffers
unter Verwendung einer schlanken, nativen ESP-IDF-Treiberkomponente wie
tny-robotics/sh1106-esp-idf , um den Ressourcenverbrauch zu minimieren.

#### 3.4.1. UI-Implementierung und Menüstruktur

Die Benutzeroberfläche wird als Zustandsautomat implementiert, um
zwischen verschiedenen Ansichten (z. B. Hauptbildschirm, Menü,
Parametereingabe) zu wechseln.

- **Hauptbildschirme:**

  - **Transmitter (Stage-Modus):** Zeigt den Gerätenamen (z.B.
    \"STAGE-91CD3A\"), eine Signalstärkeanzeige, ein Batteriesymbol und
    einen Hinweis zum Menüaufruf. Wenn das Gerät über USB mit Strom
    versorgt wird, zeigt das Batteriesymbol einen Ladeblitz an.

  - **Receiver (PC-Modus):** Zeigt den Gerätenamen (z.B. \"PC-A45B12\"),
    eine Liste der zuletzt gesehenen Sender mit deren Namen und
    Signalstärke.

- **Menüstruktur:** Das Menü wird durch langes Drücken beider Tasten (\>
  3s) aufgerufen.

  - **Hauptmenü:**

    1. Wireless Settings

    2. Pairing

    3. System

    4. Exit

  - **Untermenü Wireless Settings:**

    - Frequency: Wert bearbeiten

    - Spreading F.: Wert bearbeiten

    - Bandwidth: Wert bearbeiten

    - Back

  - **Untermenü Pairing:**

    - Start Pairing: Startet den Pairing-Prozess

    - Paired Devices: Zeigt eine Liste der gekoppelten Geräte an

    - Back

  - **Untermenü Paired Devices (nur PC-Modus):**

    - Zeigt eine scrollbare Liste der Namen gekoppelter Geräte.

    - Auswahl eines Geräts führt zu: \`\` -\> Delete? -\> Confirm

  - **Untermenü System:**

    - Device Name: Startet den Namens-Editor

    - Device Mode: Umschalten zwischen Stage und PC

    - Info: Zeigt Firmware-Version, Device-ID an

    - Back

#### 3.4.2. Logik der Tasteninteraktion im Menü

Eine konsistente Zwei-Tasten-Navigation wird implementiert:

- **Taste 1 (z.B. oben, GPIO46):**

  - *Kurzer Druck:* Navigiert im Menü nach oben / Dekrementiert einen
    Wert / Wechselt das Zeichen im Namens-Editor.

  - *Langer Druck (\>1.5s):* Geht einen Menüpunkt zurück / Bricht die
    Bearbeitung ab.

- **Taste 2 (z.B. unten, GPIO45):**

  - *Kurzer Druck:* Navigiert im Menü nach unten / Inkrementiert einen
    Wert / Wechselt zur nächsten Zeichenposition im Namens-Editor.

  - *Langer Druck (\>1.5s):* Wählt einen Menüpunkt aus / Bestätigt einen
    Wert / Speichert den Namen.

#### 3.4.3. Elegante Zwei-Tasten-Texteingabe

Die Änderung des Gerätenamens wird durch einen speziellen UI-Zustand
ermöglicht, der eine intuitive Texteingabe mit nur zwei Tasten erlaubt:

1. **Aktivierung:** Der Benutzer wählt System -\> Device Name. Der
    aktuelle Name wird mit einem blinkenden Cursor an der ersten
    Zeichenposition angezeigt.

2. **Zeichenauswahl (Taste 1):** Ein kurzer Druck auf Taste 1 (Up/Prev)
    durchläuft einen vordefinierten Zeichensatz (z.B. A-Z, a-z, 0-9,
    -\_, ) für die aktuelle Cursorposition.

3. **Nächste Position (Taste 2):** Ein kurzer Druck auf Taste 2
    (Down/Next) bestätigt das aktuelle Zeichen und bewegt den Cursor zur
    nächsten Position.

4. **Speichern und Beenden (Langer Druck Taste 2):** Ein langer Druck
    auf Taste 2 speichert den neuen Namen im NVS und kehrt zum
    Systemmenü zurück.

5. **Abbrechen (Langer Druck Taste 1):** Ein langer Druck auf Taste 1
    bricht den Vorgang ab, verwirft die Änderungen und kehrt zum Menü
    zurück.

#### 3.4.4. Branding und visuelles Feedback

- **Boot-Logo:** Beim Einschalten des Geräts wird für 2-3 Sekunden ein
  \"LoRaCue\"-Logo angezeigt. Dies stärkt die Markenidentität und gibt
  dem Benutzer ein unmittelbares visuelles Feedback, dass das Gerät
  startet.

- **Bildschirmschoner:** Nach 30 Sekunden Inaktivität wird das Display
  gedimmt und zeigt ebenfalls das \"LoRaCue\"-Logo an. Dies dient als
  visueller Hinweis auf den Energiesparmodus, bevor das Display nach 5
  Minuten vollständig abgeschaltet wird.

- **Implementierung:** Das Logo wird als 1-Bit-Bitmap-Grafik (128x64
  Pixel) erstellt und mit einem Online-Tool wie image2cpp in ein
  Byte-Array konvertiert.^16^ Dieses Array wird im Flash-Speicher der
  Firmware gespeichert und bei Bedarf direkt in den Framebuffer des
  Displays gezeichnet.

### 3.5. Strategie zur Energieeffizienz

- Die Hauptschleife der Anwendung ist ereignisgesteuert. Zwischen den
  Tastendrücken befindet sich die CPU im **Light-Sleep-Modus**, um den
  Stromverbrauch zu minimieren.

- Das LoRa-Funkmodul wird unmittelbar nach einer erfolgreichen
  Übertragung (Befehl + ACK) in den Standby- oder Schlafmodus versetzt
  und nur bei Bedarf aufgeweckt.

- Das OLED-Display wird nach einer Inaktivitätsdauer von 30 Sekunden
  gedimmt und nach 5 Minuten vollständig abgeschaltet.

- Nachdem das Display abgeschaltet wurde, geht die STAGE-Einheit in den
  **Deep-Sleep-Modus**, um die Batterielebensdauer maximal zu
  verlängern. Jede der beiden Tasten ist als externer Wake-up-Interrupt
  konfiguriert. Ein Tastendruck weckt das Gerät auf, was einem
  vollständigen Neustart entspricht. Alle Konfigurations- und
  Pairing-Daten bleiben erhalten, da sie im NVS gespeichert sind.

## 4. Empfänger-Einheit: Firmware-Blueprint

Die Firmware des Empfängers ist darauf ausgelegt, LoRa-Pakete von
mehreren Sendern zuverlässig zu empfangen, zu validieren und schnell in
USB-HID-Befehle umzusetzen.

### 4.1. Echtzeit-Task-Architektur (FreeRTOS)

- **lora_rx_task (Hohe Priorität):** Dieser Task befindet sich im
  kontinuierlichen Empfangsmodus des LoRa-Moduls. Sobald ein gültiges
  Paket empfangen wird, sendet er sofort ein ACK an den Sender zurück
  und legt den empfangenen Befehl in eine Queue für den usb_hid_task.
  Die hohe Priorität stellt sicher, dass keine eingehenden Pakete
  verpasst werden.

- **usb_hid_task (Normale Priorität):** Dieser Task wartet auf Befehle
  in der Queue. Er übersetzt den Befehl (z.B. CMD_NEXT_SLIDE) in den
  entsprechenden HID-Tastatur-Report (z.B. Senden des Keycodes für
  \"Page Down\") und übergibt diesen an den TinyUSB-Stack zur
  Übertragung an den Host-Computer.

### 4.2. Verwaltung mehrerer Sender

Die PC-Einheit verwaltet eine Liste der gekoppelten STAGE-Geräte im NVS
(Non-Volatile Storage). Jeder Eintrag in dieser Liste enthält die
DeviceID des Senders, den zugehörigen AES-Sitzungsschlüssel, den vom
Benutzer vergebenen Gerätenamen und die unveränderliche Seriennummer
(MAC-Adresse).

Beim Empfang eines Pakets führt der lora_rx_task die folgenden Schritte
aus:

1. **Identifizierung des Senders:** Extrahiert die DeviceID aus dem
    unverschlüsselten Teil des Pakets.

2. **Validierung und Schlüsselabruf:** Überprüft, ob die DeviceID in
    der Liste der gekoppelten Geräte vorhanden ist. Wenn ja, wird der
    entsprechende AES-Schlüssel für die Entschlüsselung abgerufen. Wenn
    nicht, wird das Paket verworfen.

3. **Entschlüsselung und Verifizierung:** Führt die Entschlüsselung und
    die Überprüfung der Sequenznummer durch wie in Abschnitt 4.4
    beschrieben.

Das Löschen eines Geräts über das Menü oder die Weboberfläche entfernt
den entsprechenden Eintrag aus dem NVS des PC-Moduls.

### 4.3. Implementierung des USB-Composite-Geräts (TinyUSB)

Der ESP32-S3 wird als zusammengesetztes USB-Gerät (Composite Device)
konfiguriert, das zwei Schnittstellen gleichzeitig bereitstellt.^17^ Im
normalen Betriebsmodus agiert das Gerät als USB-Device (Slave). Nur im
Pairing-Modus schaltet die PC-Einheit in den USB-Host-Modus.^18^

- **Interface 0: HID-Tastatur:** Dies ist eine
  Standard-Tastaturschnittstelle. Sie gewährleistet einen treiberlosen
  Plug-and-Play-Betrieb unter allen gängigen Betriebssystemen (Windows,
  macOS, Linux).

- **Interface 1: CDC Serial (Virtueller COM-Port):** Diese Schnittstelle
  ist für die Unternehmensbereitstellung von unschätzbarem Wert. Sie
  ermöglicht die Ausgabe von Diagnose- und Debug-Informationen und dient
  als Schnittstelle für die serielle Konfiguration und
  Firmware-Updates.^18^

**Tabelle 4: HID-Befehlszuordnung**

  ------------------------------------------------------------------------
  Empfangener Befehl       Aktion                  USB HID Usage ID
                                                   (Keycode)
  ------------------------ ----------------------- -----------------------
  CMD_NEXT_SLIDE           Nächste Folie           0x4B (Page Down)

  CMD_PREV_SLIDE           Vorherige Folie         0x4E (Page Up)

  CMD_BLACK_SCREEN         Bildschirm schwarz      0x05 (\'b\')

CMD_START_PRESENTATION   Präsentation starten    0x3E (F5)
  ------------------------------------------------------------------------

### 4.4. Kommunikation und Sicherheit

Der Empfänger implementiert die Gegenseite des LoRa-Protokolls mit
identischen SF/BW/CR-Einstellungen. Bei Empfang eines Pakets führt er
die folgenden Schritte durch:

1. **Validierung der DeviceID:** Überprüfung, ob das Paket von einem
    gekoppelten Sender stammt.

2. **Entschlüsselung:** Verwendung des Hardware-AES-Beschleunigers zur
    Entschlüsselung der Nutzdaten mit dem gemeinsamen Sitzungsschlüssel.

3. **Überprüfung der SequenceNum:** Abgleich der Sequenznummer mit dem
    zuletzt empfangenen gültigen Wert, um Replay-Angriffe zu verhindern.
    Pakete mit einer gleichen oder niedrigeren Sequenznummer werden
    verworfen.

## 5. Erweiterte Funktionen und Lebenszyklusmanagement

### 5.1. Sicherer und benutzerfreundlicher Pairing-Mechanismus via USB-C

Der Pairing-Prozess ist so konzipiert, dass er sowohl sicher als auch
intuitiv ist und das Koppeln mehrerer STAGE-Einheiten mit einer
PC-Einheit ermöglicht. Dies wird durch die USB On-The-Go (OTG) Fähigkeit
des ESP32-S3 realisiert, wobei die Rollen klar definiert sind.

#### 5.1.1. Definierte Rollen und UI-Ablauf

- **PC-Einheit:** Fungiert im Pairing-Modus immer als **USB-Host**.

- **STAGE-Einheit:** Fungiert im Pairing-Modus immer als **USB-Device**
  (konkret als CDC Serial Device).

**Pairing-Ablauf mit OLED-Feedback:**

1. **Initiierung:** Der Benutzer wählt Pairing -\> Start Pairing auf
    beiden Geräten.

2. **Aufforderung zur Verbindung:**

    - **PC-Display:** Zeigt \"Connect STAGE via USB-C to pair. Hold
      button to cancel.\"

    - **STAGE-Display:** Zeigt \"Connect to PC via USB-C to pair. Hold
      button to cancel.\"

3. **Abbruch:** Ein langer Druck auf eine beliebige Taste auf einem der
    beiden Geräte bricht den Vorgang ab und kehrt zum Menü zurück.

4. **Verbindung erkannt:** Sobald die PC-Einheit (Host) eine
    STAGE-Einheit (Device) erkennt, wechseln beide Displays zu:

    - **Beide Displays:** \"Device connected. Pairing in progress\...\"

5. **Handshake & Schlüsselaustausch:** Die Geräte führen einen
    automatischen Handshake durch. Die STAGE-Einheit sendet ihren
    Gerätenamen und ihre eindeutige Seriennummer (die werkseitige
    MAC-Adresse, ausgelesen mit esp_efuse_mac_get_default()) an die
    PC-Einheit. Die PC-Einheit generiert daraufhin einen neuen,
    eindeutigen 128-Bit-AES-Sitzungsschlüssel **für diese spezifische
    STAGE-Einheit**.

6. **Speicherung:** Die PC-Einheit speichert die DeviceID, den Namen,
    die Seriennummer und den zugehörigen Schlüssel der neuen
    STAGE-Einheit in ihrer Liste im NVS. Die STAGE-Einheit speichert die
    ID der PC-Einheit und den gemeinsamen Schlüssel.

7. **Erfolgs- oder Fehlermeldung:**

    - **Bei Erfolg:** Beide Displays zeigen für 3 Sekunden \"Pairing
      Successful!\" und kehren dann zum Menü zurück.

    - **Bei Fehler (z.B. Timeout, Kommunikationsfehler):** Beide
      Displays zeigen für 3 Sekunden \"Pairing Failed!\" und kehren dann
      zum Menü zurück.

Dieser Prozess kann für jede weitere STAGE-Einheit wiederholt werden.
Jedes Mal wird ein neuer, eindeutiger Schlüssel generiert und der Liste
der gekoppelten Geräte auf der PC-Einheit hinzugefügt, ohne bestehende
Kopplungen zu beeinträchtigen.

### 5.2. Einheitliche Konfiguration über Wi-Fi Web-Interface

Um die Konfiguration zu vereinfachen und den Code wiederzuverwenden,
wird die Wi-Fi-Schnittstelle nicht nur für OTA-Updates, sondern auch als
umfassendes Konfigurationsportal genutzt. Dies schafft eine einheitliche
Konfigurationslogik mit zwei Frontends: dem OLED-Menü und einer
Weboberfläche.

1. **Aktivierung:** Im \"Update/Config\"-Modus aktiviert das Gerät sein
    Wi-Fi im Access-Point-Modus und startet einen HTTP-Server mit dem
    esp_http_server-Modul von ESP-IDF.^19^

2. **Web-Portal:** Ein Benutzer kann sich mit dem Wi-Fi-Netzwerk des
    Geräts verbinden und über einen Browser auf die IP-Adresse
    (Standard: 192.168.4.1) zugreifen.^19^

3. **Funktionen:** Die Weboberfläche bietet Zugriff auf alle
    Konfigurationsoptionen, die auch über das OLED-Menü verfügbar sind:

    - Ändern des Gerätenamens.

    - Anpassen der Wireless-Einstellungen.

    - Anzeigen und Löschen von gekoppelten Geräten (im PC-Modus).

    - Umschalten des Gerätemodus.

    - Eine separate Seite (/update) für den Firmware-Upload (OTA).

4. **Implementierung:**

    - Ein GET /-Handler liefert die Haupt-HTML-Seite mit einem Formular,
      das die aktuellen Einstellungen aus dem NVS anzeigt.

    - Ein POST /config-Handler empfängt die Formulardaten, validiert sie
      und schreibt die neuen Werte in den NVS.

    - Diese Architektur stellt sicher, dass Änderungen, egal ob über das
      Gerätemenü oder die Weboberfläche vorgenommen, konsistent sind, da
      beide auf dieselbe Konfigurationsdatenbasis im NVS zugreifen.

### 5.3. Firmware-Update-Strategien

#### 5.3.1. Over-the-Air (OTA) Update-Strategie

Die Möglichkeit, die Firmware im Feld zu aktualisieren, ist für die
langfristige Produktunterstützung unerlässlich. Hierfür wird das robuste
OTA-System von ESP-IDF genutzt.^21^

1. Ein \"Update-Modus\" wird implementiert, der durch eine spezielle
    Tastenkombination oder einen seriellen Befehl ausgelöst wird.

2. Im Update-Modus aktiviert das Gerät (sowohl Sender als auch
    Empfänger) sein Wi-Fi im Access-Point-Modus.^22^

3. Das Gerät hostet eine einfache Webseite über den
    ESP-IDF-HTTP-Server, die es einem autorisierten Benutzer ermöglicht,
    eine neue, digital signierte Firmware-Binärdatei (.bin)
    hochzuladen.^23^

4. Die esp_https_ota-Komponente wird verwendet, um die neue Firmware in
    die inaktive App-Partition des Flash-Speichers zu schreiben.^21^

5. Nach dem Hochladen wird die Signatur der Firmware überprüft. Nur
    wenn die Signatur gültig ist, wird die neue Partition als bootfähig
    markiert.

6. Das System startet neu. Der in ESP-IDF integrierte Rollback-Schutz
    stellt sicher, dass das Gerät zur vorherigen funktionierenden
    Firmware zurückkehrt, falls die neue Version nicht erfolgreich
    startet oder sich nicht innerhalb einer bestimmten Zeit als
    \"gültig\" markiert.

#### 5.3.2. Spezifikation für Firmware-Update über USB-CDC

Als robuste Fallback-Option wird ein sicherer
Firmware-Update-Mechanismus über die serielle USB-CDC-Schnittstelle
implementiert. Dies erfordert eine dedizierte \"Loader\"-Anwendung und
ein PC-seitiges Kommandozeilen-Tool.

- **Architektur:** Die Flash-Partitionstabelle wird so konfiguriert,
  dass sie eine kleine, schreibgeschützte \"Factory\"-Partition für den
  Loader und zwei OTA-Partitionen für die Hauptanwendung enthält. Der
  Loader ist eine minimale ESP-IDF-Anwendung, deren einzige Aufgabe es
  ist, die Hauptanwendung sicher zu aktualisieren.

- **Update-Prozess:**

  1. Der Benutzer versetzt das Gerät in den Update-Modus (z.B. durch
      einen speziellen Menüpunkt, der das Gerät in die Loader-Anwendung
      booten lässt).

  2. Das PC-Tool sendet einen UPDATE_START-Befehl mit Metadaten: {
      \"magic\": \"LRCUEUPD\", \"version\": \"1.1.0\", \"size\": 204800,
      \"sha256\": \"\...\" }.

  3. Der Loader prüft das \"magic\"-Feld, um sicherzustellen, dass es
      sich um eine LoRaCue-Firmware handelt. Er prüft, ob die Version
      neuer ist als die bestehende (optional, für Anti-Rollback) und ob
      genügend Speicherplatz vorhanden ist.

  4. Bei Erfolg antwortet der Loader mit CMD_READY und löscht die
      inaktive OTA-Partition.

  5. Das PC-Tool sendet die Firmware-Binärdatei in 1KB-Blöcken.

  6. Der Loader schreibt jeden Block in den Flash und antwortet nach
      jedem Block mit BLOCK_OK.

  7. Nach dem letzten Block sendet das PC-Tool UPDATE_VERIFY.

  8. Der Loader berechnet den SHA256-Hash der geschriebenen Daten und
      vergleicht ihn mit dem in den Metadaten empfangenen Hash.

  9. Bei Übereinstimmung antwortet der Loader mit VERIFY_OK, ruft
      esp_ota_set_boot_partition() auf, um die neue Firmware für den
      nächsten Start zu aktivieren, und sendet REBOOTING. Bei
      Nichtübereinstimmung sendet er VERIFY_FAIL.

### 5.4. Konfiguration über die serielle Konsole (CDC-CLI)

Zusätzlich zur OLED- und Web-UI wird eine Kommandozeilen-Schnittstelle
(CLI) über die USB-CDC-Schnittstelle bereitgestellt. Dies ermöglicht die
Skript-basierte Konfiguration und das Debugging.

- **Implementierung:** Die CLI wird mit der esp_console-Komponente von
  ESP-IDF realisiert. Ein dedizierter FreeRTOS-Task liest die Eingaben
  von der seriellen Schnittstelle und leitet sie an den Konsolen-Handler
  weiter.

- **Befehlssatz:** Die folgenden Befehle werden implementiert, um die
  Konfiguration im NVS zu lesen und zu schreiben:

  - get device_name: Liest den aktuellen Gerätenamen.

  - set device_name \"Neuer Name\": Setzt einen neuen Gerätenamen.

  - get wireless \<param\>: Liest einen Funkparameter (freq, sf, bw).

  - set wireless \<param\> \<value\>: Setzt einen Funkparameter.

  - get paired_devices: Listet alle gekoppelten Geräte auf (nur
    PC-Modus).

  - delete_device \<DeviceID\>: Löscht ein gekoppeltes Gerät (nur
    PC-Modus).

  - get device_mode: Zeigt den aktuellen Modus (PC oder STAGE).

  - set device_mode \<mode\>: Wechselt den Modus und startet neu.

  - reboot: Startet das Gerät neu.

  - help: Zeigt eine Liste aller verfügbaren Befehle an.

## 6. Implementierungs-Roadmap und Best Practices

### 6.1. Phasenweiser Entwicklungsansatz

1. **Phase 1: Hardware-Inbetriebnahme & BSP:** Entwicklung und Test des
    BSP. Verifizierung der grundlegenden Ansteuerung aller
    Peripheriegeräte (SPI, I2C, GPIO, ADC) auf einem einzelnen Board.

2. **Phase 2: Kernkommunikationsverbindung:** Implementierung der
    unverschlüsselten LoRa-Punkt-zu-Punkt-Kommunikation zwischen zwei
    Boards.

3. **Phase 3: USB-HID des Empfängers:** Implementierung der
    USB-HID-Tastaturfunktionalität am Empfänger und Test mit einem
    Host-PC.

4. **Phase 4: Integration & Sicherheit:** Zusammenführung der
    LoRa-Verbindung mit der USB-HID-Funktionalität. Implementierung des
    vollständigen, sicheren und bestätigten Protokolls.

5. **Phase 5: UI & Energiemanagement des Senders:** Entwicklung der
    OLED-Zustandsmaschine, der Tastenlogik und der
    Energiesparfunktionen.

6. **Phase 6: Erweiterte Funktionen:** Implementierung der sicheren
    Pairing-, Namensgebungs- und Update-Mechanismen (OTA & USB).

7. **Phase 7: Web- & CLI-Konfiguration:** Entwicklung der
    Web-Oberfläche und der seriellen Kommandozeile.

### 6.2. Test- und Validierungsstrategie

- **Latenztests:** Messung der Zeit von der steigenden Flanke am
  GPIO-Pin des Tasters bis zum Erschenen des USB-Signals auf den
  D+/D\--Leitungen mit einem Oszilloskop. Ziel: \< 50 ms.

- **Reichweitentests:** Durchführung von Tests in verschiedenen
  Umgebungen (Freifeld, Bürogebäude), um die realistische maximale
  Reichweite mit den gewählten LoRa-Parametern zu ermitteln.

- **Dauertests:** Automatisierung von Tastendrücken und Betrieb des
  Senders im Akkubetrieb über einen längeren Zeitraum, um den
  Stromverbrauch und die Batterielebensdauer zu validieren.

- **Sicherheitstests:** Einsatz eines dritten LoRa-Geräts im
  Promiscuous-Modus, um zu versuchen, Pakete mitzuschneiden und
  wiederholt zu senden (Replay-Angriff), um die Wirksamkeit der
  Verschlüsselung und des Anti-Replay-Mechanismus zu testen.

### 6.3. Produktionsentscheidungen

- **Sicherung der Firmware:** Für Produktionseinheiten werden die
  eFuse-Funktionen des ESP32 genutzt, um JTAG dauerhaft zu deaktivieren
  und Secure Boot v2 zu aktivieren. Dies schützt das geistige Eigentum
  der Firmware vor unbefugtem Auslesen.

- **Hardware-Design:** Während das Heltec-Board für die Entwicklung
  genutzt wird, basiert die Massenproduktion auf einem
  benutzerdefinierten PCB-Design. Dies ermöglicht eine Optimierung des
  Formfaktors, der Kosten und der Komponentenplatzierung, während die
  Kernkomponenten (ESP32-S3 + SX1262) beibehalten werden.

## 7. Development Workflow und Automation (DevOps)

Für ein Enterprise-Grade-Projekt ist ein robuster und automatisierter
Entwicklungs-Workflow unerlässlich. Die folgende Toolchain wird
implementiert, um Codequalität, konsistente Versionierung und
zuverlässige Builds sicherzustellen.

### 7.1. Continuous Integration (CI) mit GitHub Actions

Ein CI-Pipeline wird mittels GitHub Actions eingerichtet, um den Code
bei jedem Push oder Pull Request automatisch zu kompilieren. Dies stellt
sicher, dass der Code auf der main-Branch immer lauffähig ist und
Integrationsfehler frühzeitig erkannt werden.

- **Workflow-Definition:** Ein YAML-Workflow
  (.github/workflows/build.yml) wird erstellt.

- **Umgebungseinrichtung:** Der Workflow verwendet die offizielle
  espressif/install-esp-idf-action, um eine konsistente
  ESP-IDF-Build-Umgebung auf dem Runner zu installieren.^24^

- **Build-Prozess:** Anschließend wird die espressif/esp-idf-ci-action
  genutzt, um den idf.py build-Befehl auszuführen.^25^

- **Vorteile:** Dieser Prozess automatisiert die Überprüfung der
  Code-Integrität und stellt sicher, dass alle Änderungen erfolgreich
  kompiliert werden, bevor sie in die Hauptentwicklungszweige gemerged
  werden.

### 7.2. Automatisierte semantische Versionierung mit GitVersion

Die manuelle Verwaltung von Versionsnummern ist fehleranfällig.
GitVersion wird eingesetzt, um diesen Prozess zu automatisieren.

- **Funktionsweise:** GitVersion analysiert die Git-Historie (Tags und
  Branch-Struktur) und generiert automatisch eine semantische
  Versionsnummer (z.B., 1.2.0-beta.1+5) gemäß dem
  GitFlow-Branching-Modell.^26^

- **Integration:** In der CI-Pipeline wird GitVersion ausgeführt, um die
  Versionsnummer zu ermitteln. Diese Version wird dann verwendet, um:

  1. Releases auf GitHub zu taggen.

  2. Die Firmware-Binärdatei zu benennen (z.B.,
      loracue-firmware-v1.2.0.bin).

  3. Die Versionsnummer direkt in die Firmware zu kompilieren. Dies
      geschieht durch Übergabe der Version als C-Makro (-D
      FIRMWARE_VERSION=\\\"\...\\\") während des Build-Prozesses.^28^

### 7.3. Erzwingung von Commit-Konventionen mit Commitlint und Husky

Um eine konsistente und aussagekräftige Git-Historie zu gewährleisten,
die für die automatische Versionierung und Changelog-Erstellung
unerlässlich ist, werden Commitlint und Husky verwendet.

- **Husky:** Dient als Git-Hook-Manager. Es wird so konfiguriert, dass
  es vor jedem Commit bestimmte Skripte ausführt.^29^

- **Commitlint:** Ist ein Linter für Commit-Nachrichten. Er wird über
  einen von Husky verwalteten commit-msg-Hook aufgerufen.^30^

- **Konfiguration:** Commitlint wird so konfiguriert, dass er die
  \"Conventional Commits\"-Spezifikation erzwingt. Das bedeutet, jede
  Commit-Nachricht muss einem Format wie feat: add new pairing UI oder
  fix: resolve battery display bug folgen.

- **Vorteil:** Commits, die diesem Format nicht entsprechen, werden
  automatisch abgelehnt. Dies stellt sicher, dass GitVersion die Art der
  Änderungen (Feature, Fix, Breaking Change) korrekt interpretieren und
  die Versionsnummer entsprechend erhöhen kann. Zusätzlich können
  pre-commit-Hooks für automatisches Code-Formatting (z.B. mit
  clang-format) eingerichtet werden.

## 8. Implementierungsreife und KI-gestützte Entwicklung

Dieser Blueprint hat einen Reifegrad erreicht, der eine direkte
Implementierung ermöglicht. Alle Aspekte des Systems, von der
Low-Level-Hardware-Interaktion über die Echtzeit-Softwarearchitektur bis
hin zu den DevOps-Prozessen, sind nach Enterprise-Standards und Best
Practices spezifiziert.

Die detaillierte, modulare und spezifische Natur dieses Dokuments macht
es **außergewöhnlich gut geeignet für moderne KI-gestützte
Entwicklungsworkflows**. Jeder nummerierte Abschnitt und jede Tabelle
kann als präziser, kontextreicher Prompt für KI-Codierungsassistenten
wie GitHub Copilot, Amazon Q oder ähnliche LLMs dienen. Dies kann die
Entwicklungsgeschwindigkeit erheblich beschleunigen, indem die
Generierung von Boilerplate-Code, die Implementierung definierter
Protokolle und die Erstellung von Konfigurationsdateien automatisiert
wird, während sich die menschlichen Entwickler auf die Architektur, das
Testen und die Integration konzentrieren.

### Referenzen

1. nopnop2002/esp-idf-sx126x: SX1262/SX1268/LLCC68 Low Power Long Range
    Transceiver driver for esp-idf - GitHub, Zugriff am September 26,
    2025,
    [[https://github.com/nopnop2002/esp-idf-sx126x]{.underline}](https://github.com/nopnop2002/esp-idf-sx126x)

2. jgromes/radiolib • v6.6.0 - ESP Component Registry, Zugriff am
    September 26, 2025,
    [[https://components.espressif.com/components/jgromes/radiolib/versions/6.6.0?language=en]{.underline}](https://components.espressif.com/components/jgromes/radiolib/versions/6.6.0?language=en)

3. sx1262 · GitHub Topics, Zugriff am September 26, 2025,
    [[https://github.com/topics/sx1262]{.underline}](https://github.com/topics/sx1262)

4. WiFi LoRa 32 --- esp32 latest documentation - Heltec Docs, Zugriff
    am September 26, 2025,
    [[https://docs.heltec.org/en/node/esp32/wifi_lora_32/index.html]{.underline}](https://docs.heltec.org/en/node/esp32/wifi_lora_32/index.html)

5. ShotokuTech/HeltecLoRa32v3_I2C: Pinout diagram for the Heltec LoRa
    32 V3 development board - GitHub, Zugriff am September 26, 2025,
    [[https://github.com/ShotokuTech/HeltecLoRa32v3_I2C]{.underline}](https://github.com/ShotokuTech/HeltecLoRa32v3_I2C)

6. WiFi LoRa 32 V3 Documentation repository - Heltec Automation
    Technical Community, Zugriff am September 26, 2025,
    [[http://community.heltec.cn/t/wifi-lora-32-v3-documentation-repository/14475]{.underline}](http://community.heltec.cn/t/wifi-lora-32-v3-documentation-repository/14475)

7. WiFi LoRa 32 V3 Voltage read(use GPIO1\[VBAT_READ\]) question,
    Zugriff am September 26, 2025,
    [[http://community.heltec.cn/t/wifi-lora-32-v3-voltage-read-use-gpio1-vbat-read-question/12646]{.underline}](http://community.heltec.cn/t/wifi-lora-32-v3-voltage-read-use-gpio1-vbat-read-question/12646)

8. Battery voltage reading on WiFi LoRa32 v3.1 board · Issue #234 ·
    Heltec-Aaron-Lee/WiFi_Kit_series - GitHub, Zugriff am September 26,
    2025,
    [[https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues/234]{.underline}](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues/234)

9. Heltec Wifi Lora 32 GPIO 37 : r/esp32 - Reddit, Zugriff am September
    26, 2025,
    [[https://www.reddit.com/r/esp32/comments/htazgb/heltec_wifi_lora_32_gpio_37/]{.underline}](https://www.reddit.com/r/esp32/comments/htazgb/heltec_wifi_lora_32_gpio_37/)

10. Battery Management - Arduino - Digital Concepts, Zugriff am
    September 26, 2025,
    [[http://digitalconcepts.net.au/arduino/index.php?op=Battery]{.underline}](http://digitalconcepts.net.au/arduino/index.php?op=Battery)

11. LoRa bitrate calculator and understanding LoRa parameters -
    unsigned.io, Zugriff am September 26, 2025,
    [[https://unsigned.io/understanding-lora-parameters/]{.underline}](https://unsigned.io/understanding-lora-parameters/)

12. Airtime calculator for LoRaWAN, Zugriff am September 26, 2025,
    [[https://avbentem.github.io/airtime-calculator/]{.underline}](https://avbentem.github.io/airtime-calculator/)

13. ESP32 Hardware Encryption with wolfSSL - gojimmypi - GitHub Pages,
    Zugriff am September 26, 2025,
    [[https://gojimmypi.github.io/ESP32-hardware-encryption-wolfSSL/]{.underline}](https://gojimmypi.github.io/ESP32-hardware-encryption-wolfSSL/)

14. components/esp32/hwcrypto/aes.c ·
    94ec3c8e53e7f48133f17ae6c6e905fa5a856fd2 · Felix Brüning / esp-idf -
    GitLab, Zugriff am September 26, 2025,
    [[https://gitlab.informatik.uni-bremen.de/fbrning/esp-idf/-/blob/94ec3c8e53e7f48133f17ae6c6e905fa5a856fd2/components/esp32/hwcrypto/aes.c]{.underline}](https://gitlab.informatik.uni-bremen.de/fbrning/esp-idf/-/blob/94ec3c8e53e7f48133f17ae6c6e905fa5a856fd2/components/esp32/hwcrypto/aes.c)

15. Espressif RISC-V Hardware Accelerated Cryptographic Functions Up to
    1000% Faster than Software - wolfSSL, Zugriff am September 26, 2025,
    [[https://www.wolfssl.com/espressif-risc-v-hardware-accelerated-cryptographic-functions-up-to-1000-faster-than-software/]{.underline}](https://www.wolfssl.com/espressif-risc-v-hardware-accelerated-cryptographic-functions-up-to-1000-faster-than-software/)

16. Bitmap Image Display on OLED Screen - Hackaday.io, Zugriff am
    September 27, 2025,
    [[https://hackaday.io/project/202272-bitmap-image-display-on-oled-screen]{.underline}](https://hackaday.io/project/202272-bitmap-image-display-on-oled-screen)

17. ESP-IDF vs Arduino IDE: Which is Better for ESP32? - YouTube,
    Zugriff am September 26, 2025,
    [[https://www.youtube.com/watch?v=yvWbvnj3_ss]{.underline}](https://www.youtube.com/watch?v=yvWbvnj3_ss)

18. What are the benefits of using ESP-IDF over Arduino IDE for
    developing on ESP32? - Reddit, Zugriff am September 26, 2025,
    [[https://www.reddit.com/r/esp32/comments/1bocl8m/what_are_the_benefits_of_using_espidf_over/]{.underline}](https://www.reddit.com/r/esp32/comments/1bocl8m/what_are_the_benefits_of_using_espidf_over/)

19. ESP-IDF Tutorials: Basic HTTP server - Espressif Developer Portal,
    Zugriff am September 26, 2025,
    [[https://developer.espressif.com/blog/2025/06/basic_http_server/]{.underline}](https://developer.espressif.com/blog/2025/06/basic_http_server/)

20. HTTP Server - ESP32 - --- ESP-IDF Programming Guide v5.5.1
    documentation, Zugriff am September 26, 2025,
    [[https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html]{.underline}](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html)

21. ESP HTTPS OTA - ESP32-S3 - --- ESP-IDF Programming Guide v5.5.1
    documentation - Espressif Systems, Zugriff am September 26, 2025,
    [[https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/esp_https_ota.html]{.underline}](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/esp_https_ota.html)

22. ota via http and using ESP32 as AP instead of as STA, Zugriff am
    September 26, 2025,
    [[https://esp32.com/viewtopic.php?t=31623]{.underline}](https://esp32.com/viewtopic.php?t=31623)

23. ESP32 OTA (Over-the-Air) Updates - AsyncElegantOTA Arduino \| Random
    Nerd Tutorials, Zugriff am September 26, 2025,
    [[https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/]{.underline}](https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/)

24. espressif/install-esp-idf-action - GitHub, Zugriff am September 27,
    2025,
    [[https://github.com/espressif/install-esp-idf-action]{.underline}](https://github.com/espressif/install-esp-idf-action)

25. espressif/esp-idf-ci-action: GitHub Action for ESP32 CI, Zugriff am
    September 27, 2025,
    [[https://github.com/espressif/esp-idf-ci-action]{.underline}](https://github.com/espressif/esp-idf-ci-action)

26. How it works - GitVersion, Zugriff am September 27, 2025,
    [[https://gitversion.net/docs/learn/how-it-works]{.underline}](https://gitversion.net/docs/learn/how-it-works)

27. Documentation - GitVersion, Zugriff am September 27, 2025,
    [[https://gitversion.net/docs/]{.underline}](https://gitversion.net/docs/)

28. How can I get my C code to automatically print out its Git version
    hash? - Stack Overflow, Zugriff am September 27, 2025,
    [[https://stackoverflow.com/questions/1704907/how-can-i-get-my-c-code-to-automatically-print-out-its-git-version-hash]{.underline}](https://stackoverflow.com/questions/1704907/how-can-i-get-my-c-code-to-automatically-print-out-its-git-version-hash)

29. Setup Husky and Commitlint to our repo - Arcadio Quintero, Zugriff
    am September 27, 2025,
    [[https://arcadioquintero.com/setup-husky-and-commitlint-to-our-repo]{.underline}](https://arcadioquintero.com/setup-husky-and-commitlint-to-our-repo)

30. Guide: Local setup - commitlint, Zugriff am September 27, 2025,
    [[https://commitlint.js.org/guides/local-setup.html]{.underline}](https://commitlint.js.org/guides/local-setup.html)

// platformio run --target upload && platformio device monitor

#include <Arduino.h>
#include <SPI.h>
#include <SSD1306.h>

// OLED pins to ESP32 GPIOs via this connecthin:
#define OLED_ADDRESS 0x3c
#define OLED_SDA 4  // GPIO4
#define OLED_SCL 15 // GPIO15
#define OLED_RST 16 // GPIO16

SSD1306 display(OLED_ADDRESS, OLED_SDA, OLED_SCL);

#define max_lines 5
String log_array[max_lines];
int log_length = 0;
int new_logs = false;

void oled_log(String text)
{   // only increment till screen is full
    if (log_length < max_lines)
        log_length++;
    else {
        // move up one line
        for (int i = 0; i < log_length-1; i++)
            log_array[i] = log_array[i+1];
    }
    log_array[log_length-1] = text;
	new_logs = true;
}
void print_oled_log()
{   if (new_logs) {
		Serial.println(log_length - 1);
    	int font_height = 10;
		for (int i = 0; i < log_length; i++)
			display.drawString(0, i * font_height, String(log_array[i]));
		display.display();
		new_logs = false;
	}
}

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

static const PROGMEM u1_t NWKSKEY[16] = { 0xFA, 0xD5, 0xEC, 0xF3, 0x92, 0xDB, 0x56, 0x38, 0x13, 0xCB, 0x00, 0xC0, 0xD7, 0x86, 0xA5, 0x46 };
static const u1_t PROGMEM APPSKEY[16] = { 0x37, 0xC1, 0x4A, 0x86, 0x87, 0xEA, 0x55, 0xFE, 0xC8, 0xA3, 0x05, 0x48, 0x36, 0xBF, 0x0B, 0x52 };
static const u4_t DEVADDR = 0x26011895;

void os_getArtEui(u1_t *buf) {}
void os_getDevEui(u1_t *buf) {}
void os_getDevKey(u1_t *buf) {}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;

// Pin mapping
//LoRa module (using the LMIC-Arduino library)
const lmic_pinmap lmic_pins = {
    .nss = 18,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,
    .dio = {/*dio0*/ 26, /*dio1*/ 33, /*dio2*/ 32}};

void do_send(osjob_t *j)
{
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND)
    {
        oled_log("OP_TXRXPEND, not sending");
    }
    else
    {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata) - 1, 0);
        oled_log("Packet queued");
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent(ev_t ev)
{
    Serial.print(os_getTime());
    Serial.print(": ");
    switch (ev)
    {
    case EV_SCAN_TIMEOUT:
        oled_log("EV_SCAN_TIMEOUT");
        break;
    case EV_BEACON_FOUND:
        oled_log("EV_BEACON_FOUND");
        break;
    case EV_BEACON_MISSED:
        oled_log("EV_BEACON_MISSED");
        break;
    case EV_BEACON_TRACKED:
        oled_log("EV_BEACON_TRACKED");
        break;
    case EV_JOINING:
        oled_log("EV_JOINING");
        break;
    case EV_JOINED:
        oled_log("EV_JOINED");
        break;
    case EV_RFU1:
        oled_log("EV_RFU1");
        break;
    case EV_JOIN_FAILED:
        oled_log("EV_JOIN_FAILED");
        break;
    case EV_REJOIN_FAILED:
        oled_log("EV_REJOIN_FAILED");
        break;
    case EV_TXCOMPLETE:
        oled_log("EV_TXCOMPLETE (includes waiting for RX windows)");
        if (LMIC.txrxFlags & TXRX_ACK)
            oled_log("Received ack");
        if (LMIC.dataLen)
        {
            oled_log("Received ");
            oled_log(F(LMIC.dataLen));
            oled_log(" bytes of payload");
        }
        // Schedule next transmission
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
        break;
    case EV_LOST_TSYNC:
        oled_log("EV_LOST_TSYNC");
        break;
    case EV_RESET:
        oled_log("EV_RESET");
        break;
    case EV_RXCOMPLETE:
        // data received in ping slot
        oled_log("EV_RXCOMPLETE");
        break;
    case EV_LINK_DEAD:
        oled_log("EV_LINK_DEAD");
        break;
    case EV_LINK_ALIVE:
        oled_log("EV_LINK_ALIVE");
        break;
    default:
        oled_log("Unknown event");
        break;
    }
}

void setup()
{
    Serial.begin(921600);
    while (!Serial)
        ; //if just the the basic function, must connect to a computer
    delay(1000);
    oled_log("Starting");

    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW); // low to reset OLED
    delay(50);
    digitalWrite(OLED_RST, HIGH); // must be high to turn on OLED

    display.init();
    display.flipScreenVertically();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10); // 10, 16

    // LMIC init
	pinMode(26, INPUT);
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

// Set static session parameters. Instead of dynamically establishing a session
// by joining the network, precomputed session parameters are be provided.
#ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);
#else
    // If not running an AVR with PROGMEM, just use the arrays directly
    LMIC_setSession(0x1, DEVADDR, NWKSKEY, APPSKEY);
#endif

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7, 14);

    // Start job
    do_send(&sendjob);
}

// const int  btn_pin = 5;
// int btn_state = -1;
// int last_btn_state = 0;
void loop()
{	// button test
	// if (btn_state == -1)
	// 	pinMode(btn_pin, INPUT);
	// btn_state = digitalRead(btn_pin);
	// if (last_btn_state != btn_state)
	// 	oled_log(last_btn_state + " !=" + btn_state);
	// last_btn_state = btn_state;

	// lora
    os_runloop_once();

    print_oled_log();
	
}

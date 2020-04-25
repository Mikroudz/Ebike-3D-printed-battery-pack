#include <Adafruit_NeoPixel.h>

#include <Encoder.h>

#define BUTTON_PIN 2
#define ENCODER_A 3
#define ENCODER_B 4
#define PIXEL_PIN 5

#define WELD_PIN 12
#define BUTTON_DEBOUNCE 200
#define CLOCKSPEED 16000000
#define PRESCALE 8

unsigned long last_button_int = 0;
volatile bool button_press, PIN_TOGGLE = false;
volatile bool weld_active = false;

long encoder_val = 0;
int us_encoder_time = 10000;

Encoder encoder(ENCODER_A, ENCODER_B);
Adafruit_NeoPixel strip(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void buttonISR(){
  if (millis() - last_button_int > 1000 && button_press != true && !digitalRead(BUTTON_PIN)){
    button_press = true;
    weld_active = true;
    last_button_int = millis();
    TCCR1B &= ~_BV(CS11); // Kello pois päältä
    TCNT1H = 0; // nollaa kellon yläbitit
    TCNT1L = 0; // nollaa kellon alabitit
    TIFR1 |= _BV(OCF1A); // Nollaa keskeytykset kellosta
    TIMSK1 |= _BV(OCIE0A); // Käynnistä keskeytykset OCR1A rekisterin overflow
    PORTB |= (1 << 4); // käynnistetään hitsaus
    TCCR1B |= _BV(CS11); // käynnistetään kello uudestaan

  }
}

void setTime(uint16_t p_time_us){
  if(weld_active)
    return;
  if(p_time_us < 100)
    p_time_us = 100;
  uint16_t int_cycles = p_time_us * 2;
  //Serial.println(int_cycles);
  OCR1A = int_cycles;
}

ISR(TIMER1_COMPA_vect){
  weld_active = false;
  TIMSK1 = 0;
  // sammuta hitsi
  PORTB ^= (1 << 4);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(WELD_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  Serial.begin(115200);
  Serial.println("start");
  strip.begin(); // Initialize NeoPixel strip object (REQUIRED)
  strip.show();  // Initialize all pixels to 'off'
  strip.setBrightness(20);
  // timer 1
  TCCR1A = 0;
  TCCR1B = 0;
  
  //TCCR1A |= _BV(WGM10) | _BV(WGM11); 
  TCCR1B |= _BV(WGM12);// | _BV(WGM13);
  TCCR1B |= _BV(CS11);
  setTime(us_encoder_time);

}

void set_led_color(int time_val){
  if(time_val <= 2000){
    strip.setPixelColor(0, strip.Color(0,255,0));
 
  }else if(time_val > 2000 && time_val < 4000){
    strip.setPixelColor(0, strip.Color(0,0,255));
  }else if(time_val >= 4000 && time_val <= 6000){
    strip.setPixelColor(0, strip.Color(0,255,255));
  }else if(time_val > 6000 && time_val < 8000){
    strip.setPixelColor(0, strip.Color(255,255,0));
  }else if(time_val >= 8000 && time_val <= 10000){
    strip.setPixelColor(0, strip.Color(255,0,255));
  }else if(time_val > 10000){
    strip.setPixelColor(0, strip.Color(255,0,0));
  }
  strip.show();
}


void update_weld_time(){
  int cur_val = encoder.read();
  int diff = encoder_val - cur_val;
  if(diff == 0)
    return;
  if(diff > 0){
    us_encoder_time += 100;
  }else if(diff < 0){
    us_encoder_time -= 100;
  }
  if(us_encoder_time < 0)
    us_encoder_time = 0;
  set_led_color(us_encoder_time);
  encoder_val = cur_val;
  setTime((uint16_t)us_encoder_time);
}

void loop() {
  delay(50);
  update_weld_time();
  //Serial.println(us_encoder_time);
  if(button_press)
    button_press = false;
}

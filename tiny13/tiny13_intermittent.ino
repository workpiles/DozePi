#define PW_PIN 3
#define FC_PIN 4
#define SW_PIN 1
#define DIP0_PIN 0
#define DIP1_PIN 2

#define NOP __asm__ volatile ("nop\n\t")

// 実測　232ms per ovf
#define ONE_MINUTE_COUNT 258 //1min

#define OUTPUT_TIME   1 //１分

class PowerControl {
  public:
    void periodic();
    void force(bool on);
    void output();
    void setPeriodicTime(int time);

  private:
    bool isRun = false;
    int count = 0;
    bool periodic_on = false;
    bool force_on = false;
    int periodic_time = 0;

};

void PowerControl::setPeriodicTime(int time) {
  periodic_time = time;
}

void PowerControl::periodic() {
  count++;
  
  if (isRun) {
    if (count >= OUTPUT_TIME) {
      periodic_on = false;
      count = 0;
      isRun = false;
    }
  } else {
    if (count >= periodic_time) {
      periodic_on = true;
      count = 0;
      isRun = true;
    }
  }  
}

void PowerControl::force(bool on) {
  force_on = on;
}

void PowerControl::output() {
  if (force_on ) {
    digitalWrite(FC_PIN, HIGH);
    digitalWrite(PW_PIN, HIGH);
  } else if ( periodic_on) {
    digitalWrite(FC_PIN, LOW);
    digitalWrite(PW_PIN, HIGH);    
  } else {
    digitalWrite(FC_PIN, LOW);
    digitalWrite(PW_PIN, LOW);
  }
}


volatile unsigned long timer_count = 0; 
PowerControl pw;

ISR(TIM0_OVF_vect) {
  timer_count += 1;

  if (timer_count >= ONE_MINUTE_COUNT) {
    pw.periodic();
    timer_count = 0;
  } 


}

bool check_sw_state(uint8_t sw) {
  // chattering guard
  static bool sw_state = false;
  static unsigned int sw_buf = 0;
  
  sw_buf = (sw_buf << 1) | sw;
  if (sw_buf==0) {
    sw_state = true;
  } else if (sw_buf==0xffff) {
    sw_state = false;
  }
  return sw_state;
}

void setup() {
  uint8_t oldSREG = SREG;
  cli();

  //タイマースタート
  TCCR0A |= _BV(WGM01)|_BV(WGM00);
  TCCR0B |= _BV(CS02)|_BV(CS00);
  TIMSK0 |= _BV(TOIE0);
  
  SREG = oldSREG;
  
  pinMode(PW_PIN, OUTPUT);
  pinMode(FC_PIN, OUTPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(DIP0_PIN, INPUT_PULLUP);
  pinMode(DIP1_PIN, INPUT_PULLUP);

  delay(1);

  uint8_t dip = (digitalRead(DIP1_PIN) << 1) | digitalRead(DIP0_PIN);

   if (dip==0x00) {
    pw.setPeriodicTime(29); //29分ごと
   } else if (dip==0x01) {
    pw.setPeriodicTime(14);
   } else if (dip==0x02) {
    pw.setPeriodicTime(5);
   } else {
    pw.setPeriodicTime(2);
   }

}

void loop() {
  int value = digitalRead(SW_PIN);

  pw.force(check_sw_state(value));
  pw.output();
  
}

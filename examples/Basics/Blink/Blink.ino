
DeclareTaskLoop(SerialTask);

void setup() {
  Serial.begin(19200);

  pinMode(13, OUTPUT);

  CreateTaskLoop(SerialTask, HIGH_PRIORITY);
}

void loop() {
  digitalWrite(13, HIGH);
  delay(1000);
  
  digitalWrite(13, LOW);
  delay(1000);
}

TaskLoop(SerialTask){

  Serial.println(millis());
  DelayWithBlocked(1000);
  
}

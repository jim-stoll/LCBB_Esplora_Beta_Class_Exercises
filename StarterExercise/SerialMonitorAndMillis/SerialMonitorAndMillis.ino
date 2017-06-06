void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("start");
}

void loop() {
  Serial.print("time: ");
  Serial.println(millis());
  delay(100);
}

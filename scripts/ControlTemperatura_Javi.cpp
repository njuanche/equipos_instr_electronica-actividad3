#include <Arduino.h>
#include "ControlTemperatura.h"
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Fuzzy.h> // Biblioteca eFLL

extern LiquidCrystal_I2C lcd;

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

float tempDeseada = 25.0;
float tempReal = 0.0;
float tempInterna = 25.0;

const float gananciaProceso = 0.5;

int pwmCalor = 0;
int pwmFrio = 0;

unsigned long lastSensorRead = 0;
unsigned long lastProcesoUpdate = 0;

const unsigned long sensorInterval = 1000;
const unsigned long procesoInterval = 500;

Fuzzy* fuzzy;

void setupFuzzy() {
  fuzzy = new Fuzzy();

  // Entradas
  FuzzyInput* error = new FuzzyInput(1);
  FuzzySet* errorNeg = new FuzzySet(-10, -10, -5, 0);
  FuzzySet* errorCero = new FuzzySet(-5, 0, 0, 5);
  FuzzySet* errorPos = new FuzzySet(0, 5, 10, 10);
  error->addFuzzySet(errorNeg);
  error->addFuzzySet(errorCero);
  error->addFuzzySet(errorPos);
  fuzzy->addFuzzyInput(error);

  FuzzyInput* delta = new FuzzyInput(2);
  FuzzySet* deltaNeg = new FuzzySet(-10, -10, -5, 0);
  FuzzySet* deltaCero = new FuzzySet(-5, 0, 0, 5);
  FuzzySet* deltaPos = new FuzzySet(0, 5, 10, 10);
  delta->addFuzzySet(deltaNeg);
  delta->addFuzzySet(deltaCero);
  delta->addFuzzySet(deltaPos);
  fuzzy->addFuzzyInput(delta);

  // Salidas
  FuzzyOutput* calor = new FuzzyOutput(1);
  FuzzySet* calorBajo = new FuzzySet(0, 0, 64, 128);
  FuzzySet* calorAlto = new FuzzySet(64, 128, 192, 255);
  calor->addFuzzySet(calorBajo);
  calor->addFuzzySet(calorAlto);
  fuzzy->addFuzzyOutput(calor);

  FuzzyOutput* frio = new FuzzyOutput(2);
  FuzzySet* frioBajo = new FuzzySet(0, 0, 64, 128);
  FuzzySet* frioAlto = new FuzzySet(64, 128, 192, 255);
  frio->addFuzzySet(frioBajo);
  frio->addFuzzySet(frioAlto);
  fuzzy->addFuzzyOutput(frio);

  // Reglas básicas
  FuzzyRuleAntecedent* rule1If = new FuzzyRuleAntecedent();
  rule1If->joinWithAND(errorNeg, deltaNeg);
  FuzzyRuleConsequent* rule1Then = new FuzzyRuleConsequent();
  rule1Then->addOutput(frioAlto);
  FuzzyRule* rule1 = new FuzzyRule(1, rule1If, rule1Then);
  fuzzy->addFuzzyRule(rule1);
}



void setupControlTemperatura() {
  dht.begin();
  setupFuzzy();
}

void loopControlTemperatura() {
  static float lastError = 0.0;
  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorRead >= sensorInterval) {
    lastSensorRead = currentMillis;
    tempReal = dht.readTemperature();
    if (isnan(tempReal)) tempReal = tempInterna;
  }

  float error = tempDeseada - tempInterna;
  float deltaError = error - lastError;
  lastError = error;

  fuzzy->setInput(1, error);
  fuzzy->setInput(2, deltaError);
  fuzzy->fuzzify();

  pwmCalor = fuzzy->defuzzify(1);
  pwmFrio = fuzzy->defuzzify(2);

  pwmCalor = constrain(pwmCalor, 0, 255);
  pwmFrio = constrain(pwmFrio, 0, 255);

  if (currentMillis - lastProcesoUpdate >= procesoInterval) {
    lastProcesoUpdate = currentMillis;

    float cambioCalor = gananciaProceso * (pwmCalor / 255.0);
    float cambioFrio = gananciaProceso * (pwmFrio / 255.0);

    tempInterna += cambioCalor;
    tempInterna -= cambioFrio;

    float diffAmbiente = tempReal - tempInterna;
    tempInterna += diffAmbiente * 0.005;

    Serial.print("TempReal: "); Serial.print(tempReal);
    Serial.print(" | TempSist: "); Serial.print(tempInterna);
    Serial.print(" | Calor: "); Serial.print(pwmCalor);
    Serial.print(" | Frio: "); Serial.println(pwmFrio);
  }
}

int getPWMCalor() { return pwmCalor; }
int getPWMFrio() { return pwmFrio; }
float getTemperaturaAmbiente() { return tempReal; }
float getTemperaturaInterna() { return tempInterna; }
float getTemperaturaDeseada() { return tempDeseada; }

void mostrarDatosTemperatura() {
  float t = getTemperaturaAmbiente();
  float tint = getTemperaturaInterna();
  lcd.setCursor(0, 0);
  lcd.print("TAmb: ");
  lcd.print(t, 1);
  lcd.print(" C    ");

  lcd.setCursor(0, 1);
  lcd.print("TSist: ");
  lcd.print(tint, 1);
  lcd.print(" C    ");
}
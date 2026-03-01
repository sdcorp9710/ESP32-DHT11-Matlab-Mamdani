/**************************************************************
  ESP32 + DHT11 + FIS Mamdani (exportado MATLAB) + 3 LEDs

  - Entradas reales: Temp (°C), Hum (%RH)
  - Normalización a [0..10] (igual que tu práctica anterior):
      Temp: 15..40°C -> TempN: 0..10
      Hum : 20..90%  -> HumN : 0..10

  - El FIS exportado (Karthik Nadig converter) usa:
      g_fisInput[0] = TempN
      g_fisInput[1] = HumN
      fis_evaluate();
      g_fisOutput[0] = ConfortScore 0..30

  - Semáforo (misma lógica que ya probaste):
      0..10  -> ROJO   (GPIO25)
     10..20  -> BLANCO (GPIO26)
     20..30  -> VERDE  (GPIO27)

  Requisitos:
  - Librería DHT (Adafruit) + Adafruit Unified Sensor
  - Archivo fis_header.h en la misma carpeta del sketch
**************************************************************/

#include <Arduino.h>
#include <DHT.h>
#include "fis_header.h"   // <-- tu archivo .h exportado

// ====================== DHT11 ======================
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ====================== LEDs ======================
const int LED_ROJO   = 25;
const int LED_BLANCO = 26;
const int LED_VERDE  = 27;

// ====================== Utilidades ======================
FIS_TYPE scale_to_0_10(FIS_TYPE x, FIS_TYPE xmin, FIS_TYPE xmax) {
  if (x <= xmin) return 0.0f;
  if (x >= xmax) return 10.0f;
  return (x - xmin) * 10.0f / (xmax - xmin);
}

void setTrafficLight(FIS_TYPE y) {
  bool red   = (y > 0.0f  && y < 10.0f);
  bool white = (y >= 10.0f && y < 20.0f);
  bool green = (y >= 20.0f && y <= 30.0f);

  digitalWrite(LED_ROJO,   red   ? HIGH : LOW);
  digitalWrite(LED_BLANCO, white ? HIGH : LOW);
  digitalWrite(LED_VERDE,  green ? HIGH : LOW);
}

const char* labelFromOutput(FIS_TYPE y) {
  if (y > 0.0f  && y < 10.0f)   return "ROJO (inconfortable/fuera)";
  if (y >= 10.0f && y < 20.0f)  return "BLANCO (aceptable/medio)";
  if (y >= 20.0f && y <= 30.0f) return "VERDE (ideal)";
  return "SIN CLASE (0 o >30)";
}

/**************************************************************
  ======== BLOQUE FIS EXPORTADO (MATLAB .fis -> Arduino) ========
  NOTA: Se retoma el FIS generado por tu exportación fisfisConfor.ino,
        PERO eliminamos su setup()/loop() original (analogRead/analogWrite)
        y dejamos solo:
        - variables globales del FIS
        - funciones MF + fis_evaluate()
        - datos (coeficientes, reglas, rangos)
**************************************************************/

// Number of inputs to the fuzzy inference system
const int fis_gcI = 2;
// Number of outputs to the fuzzy inference system
const int fis_gcO = 1;
// Number of rules to the fuzzy inference system
const int fis_gcR = 9;

FIS_TYPE g_fisInput[fis_gcI];
FIS_TYPE g_fisOutput[fis_gcO];

//***********************************************************************
// Support functions for Fuzzy Inference System
//***********************************************************************
// Triangular Member Function
FIS_TYPE fis_trimf(FIS_TYPE x, FIS_TYPE* p)
{
  FIS_TYPE a = p[0], b = p[1], c = p[2];
  FIS_TYPE t1 = (x - a) / (b - a);
  FIS_TYPE t2 = (c - x) / (c - b);
  if ((a == b) && (b == c)) return (FIS_TYPE) (x == a);
  if (a == b) return (FIS_TYPE) (t2 * (b <= x) * (x <= c));
  if (b == c) return (FIS_TYPE) (t1 * (a <= x) * (x <= b));
  t1 = min(t1, t2);
  return (FIS_TYPE) max(t1, 0.0f);
}

FIS_TYPE fis_min(FIS_TYPE a, FIS_TYPE b) { return min(a, b); }
FIS_TYPE fis_max(FIS_TYPE a, FIS_TYPE b) { return max(a, b); }

FIS_TYPE fis_array_operation(FIS_TYPE *array, int size, _FIS_ARR_OP pfnOp)
{
  int i;
  FIS_TYPE ret = 0;

  if (size == 0) return ret;
  if (size == 1) return array[0];

  ret = array[0];
  for (i = 1; i < size; i++) ret = (*pfnOp)(ret, array[i]);
  return ret;
}

//***********************************************************************
// Data for Fuzzy Inference System
//***********************************************************************
_FIS_MF fis_gMF[] = { fis_trimf };

int fis_gIMFCount[] = { 3, 3 };
int fis_gOMFCount[] = { 3 };

// Coefficients for the Input Member Functions
FIS_TYPE fis_gMFI0Coeff1[] = { 0, 0, 4 };
FIS_TYPE fis_gMFI0Coeff2[] = { 3, 5, 7 };
FIS_TYPE fis_gMFI0Coeff3[] = { 6, 10, 10 };
FIS_TYPE* fis_gMFI0Coeff[] = { fis_gMFI0Coeff1, fis_gMFI0Coeff2, fis_gMFI0Coeff3 };

FIS_TYPE fis_gMFI1Coeff1[] = { 0, 0, 4 };
FIS_TYPE fis_gMFI1Coeff2[] = { 3, 5, 7 };
FIS_TYPE fis_gMFI1Coeff3[] = { 6, 10, 10 };
FIS_TYPE* fis_gMFI1Coeff[] = { fis_gMFI1Coeff1, fis_gMFI1Coeff2, fis_gMFI1Coeff3 };

FIS_TYPE** fis_gMFICoeff[] = { fis_gMFI0Coeff, fis_gMFI1Coeff };

// Coefficients for the Output Member Functions
FIS_TYPE fis_gMFO0Coeff1[] = { 0, 5, 10 };
FIS_TYPE fis_gMFO0Coeff2[] = { 10, 15, 20 };
FIS_TYPE fis_gMFO0Coeff3[] = { 20, 25, 30 };
FIS_TYPE* fis_gMFO0Coeff[] = { fis_gMFO0Coeff1, fis_gMFO0Coeff2, fis_gMFO0Coeff3 };
FIS_TYPE** fis_gMFOCoeff[] = { fis_gMFO0Coeff };

// Input membership function set (usa fis_gMF[0]=trimf)
int fis_gMFI0[] = { 0, 0, 0 };
int fis_gMFI1[] = { 0, 0, 0 };
int* fis_gMFI[] = { fis_gMFI0, fis_gMFI1 };

// Output membership function set
int fis_gMFO0[] = { 0, 0, 0 };
int* fis_gMFO[] = { fis_gMFO0 };

// Rule Weights
FIS_TYPE fis_gRWeight[] = { 1,1,1,1,1,1,1,1,1 };

// Rule Type (1 = AND)
int fis_gRType[] = { 1,1,1,1,1,1,1,1,1 };

// Rule Inputs
int fis_gRI0[] = { 1, 1 };
int fis_gRI1[] = { 1, 2 };
int fis_gRI2[] = { 1, 3 };
int fis_gRI3[] = { 2, 1 };
int fis_gRI4[] = { 2, 2 };
int fis_gRI5[] = { 2, 3 };
int fis_gRI6[] = { 3, 1 };
int fis_gRI7[] = { 3, 2 };
int fis_gRI8[] = { 3, 3 };
int* fis_gRI[] = { fis_gRI0,fis_gRI1,fis_gRI2,fis_gRI3,fis_gRI4,fis_gRI5,fis_gRI6,fis_gRI7,fis_gRI8 };

// Rule Outputs
int fis_gRO0[] = { 1 };
int fis_gRO1[] = { 2 };
int fis_gRO2[] = { 2 };
int fis_gRO3[] = { 2 };
int fis_gRO4[] = { 3 };
int fis_gRO5[] = { 2 };
int fis_gRO6[] = { 2 };
int fis_gRO7[] = { 2 };
int fis_gRO8[] = { 1 };
int* fis_gRO[] = { fis_gRO0,fis_gRO1,fis_gRO2,fis_gRO3,fis_gRO4,fis_gRO5,fis_gRO6,fis_gRO7,fis_gRO8 };

// Input range Min/Max
FIS_TYPE fis_gIMin[] = { 0, 0 };
FIS_TYPE fis_gIMax[] = { 10, 10 };

// Output range Min/Max
FIS_TYPE fis_gOMin[] = { 0 };
FIS_TYPE fis_gOMax[] = { 30 };

//***********************************************************************
// Data dependent support functions
//***********************************************************************
FIS_TYPE fis_MF_out(FIS_TYPE** fuzzyRuleSet, FIS_TYPE x, int o)
{
  FIS_TYPE mfOut;
  int r;

  for (r = 0; r < fis_gcR; ++r)
  {
    int index = fis_gRO[r][o];
    if (index > 0)
    {
      index = index - 1;
      mfOut = (fis_gMF[fis_gMFO[o][index]])(x, fis_gMFOCoeff[o][index]);
    }
    else if (index < 0)
    {
      index = -index - 1;
      mfOut = 1 - (fis_gMF[fis_gMFO[o][index]])(x, fis_gMFOCoeff[o][index]);
    }
    else
    {
      mfOut = 0;
    }
    fuzzyRuleSet[0][r] = fis_min(mfOut, fuzzyRuleSet[1][r]);
  }
  return fis_array_operation(fuzzyRuleSet[0], fis_gcR, fis_max);
}

FIS_TYPE fis_defuzz_centroid(FIS_TYPE** fuzzyRuleSet, int o)
{
  FIS_TYPE step = (fis_gOMax[o] - fis_gOMin[o]) / (FIS_RESOLUSION - 1);
  FIS_TYPE area = 0;
  FIS_TYPE momentum = 0;
  FIS_TYPE dist, slice;

  for (int i = 0; i < FIS_RESOLUSION; ++i) {
    dist = fis_gOMin[o] + (step * i);
    slice = step * fis_MF_out(fuzzyRuleSet, dist, o);
    area += slice;
    momentum += slice * dist;
  }
  return ((area == 0) ? ((fis_gOMax[o] + fis_gOMin[o]) / 2) : (momentum / area));
}

//***********************************************************************
// Fuzzy Inference System (exportado)
//***********************************************************************
void fis_evaluate()
{
  FIS_TYPE fuzzyInput0[] = { 0, 0, 0 };
  FIS_TYPE fuzzyInput1[] = { 0, 0, 0 };
  FIS_TYPE* fuzzyInput[fis_gcI] = { fuzzyInput0, fuzzyInput1 };

  FIS_TYPE fuzzyRules[fis_gcR] = { 0 };
  FIS_TYPE fuzzyFires[fis_gcR] = { 0 };
  FIS_TYPE* fuzzyRuleSet[] = { fuzzyRules, fuzzyFires };

  FIS_TYPE sW = 0;

  // 1) Fuzzificación
  for (int i = 0; i < fis_gcI; ++i) {
    for (int j = 0; j < fis_gIMFCount[i]; ++j) {
      fuzzyInput[i][j] = (fis_gMF[fis_gMFI[i][j]])(g_fisInput[i], fis_gMFICoeff[i][j]);
    }
  }

  // 2) Evaluación reglas
  for (int r = 0; r < fis_gcR; ++r) {
    int index = 0;

    if (fis_gRType[r] == 1) { // AND
      fuzzyFires[r] = FIS_MAX;
      for (int i = 0; i < fis_gcI; ++i) {
        index = fis_gRI[r][i];
        if (index > 0)      fuzzyFires[r] = fis_min(fuzzyFires[r], fuzzyInput[i][index - 1]);
        else if (index < 0) fuzzyFires[r] = fis_min(fuzzyFires[r], 1 - fuzzyInput[i][-index - 1]);
        else                fuzzyFires[r] = fis_min(fuzzyFires[r], 1);
      }
    } else { // OR (no se usa en este FIS)
      fuzzyFires[r] = FIS_MIN;
      for (int i = 0; i < fis_gcI; ++i) {
        index = fis_gRI[r][i];
        if (index > 0)      fuzzyFires[r] = fis_max(fuzzyFires[r], fuzzyInput[i][index - 1]);
        else if (index < 0) fuzzyFires[r] = fis_max(fuzzyFires[r], 1 - fuzzyInput[i][-index - 1]);
        else                fuzzyFires[r] = fis_max(fuzzyFires[r], 0);
      }
    }

    fuzzyFires[r] = fis_gRWeight[r] * fuzzyFires[r];
    sW += fuzzyFires[r];
  }

  // 3) Defuzzificación
  if (sW == 0) {
    for (int o = 0; o < fis_gcO; ++o) g_fisOutput[o] = ((fis_gOMax[o] + fis_gOMin[o]) / 2);
  } else {
    for (int o = 0; o < fis_gcO; ++o) g_fisOutput[o] = fis_defuzz_centroid(fuzzyRuleSet, o);
  }
}

/**************************************************************
  ====================== TU APP (ESP32) ======================
**************************************************************/
void setup() {
  Serial.begin(9600);
  delay(1200);

  dht.begin();

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_BLANCO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  Serial.println("=== ESP32 + DHT11 + FIS Confort (export MATLAB) ===");
  Serial.println("Normalizacion: Temp 15..40C -> [0..10], Hum 20..90 -> [0..10]");
}

void loop() {
  float hum = dht.readHumidity();
  float temp = dht.readTemperature(); // Celsius

  if (isnan(hum) || isnan(temp)) {
    Serial.println("[ERROR] DHT11 NaN. Revisa cableado/pull-up 10k/VCC 3V3.");
    delay(2000);
    return;
  }

  // Normalización exactamente como tu práctica
  FIS_TYPE tN = scale_to_0_10((FIS_TYPE)temp, 15.0f, 40.0f);
  FIS_TYPE hN = scale_to_0_10((FIS_TYPE)hum, 20.0f, 90.0f);

  // Cargar entradas al FIS exportado y evaluar
  g_fisInput[0] = tN;  // TempN
  g_fisInput[1] = hN;  // HumN
  g_fisOutput[0] = 0;

  fis_evaluate();

  // Salida del FIS: ConfortScore 0..30
  FIS_TYPE y = g_fisOutput[0];

  // Semáforo
  setTrafficLight(y);

  // Evidencia en Serial (igual que el sketch anterior)
  Serial.println("-------------------------------------------");
  Serial.print("Temp(C): "); Serial.print(temp, 1);
  Serial.print(" | Hum(%): "); Serial.print(hum, 1);
  Serial.print(" || TempN: "); Serial.print(tN, 2);
  Serial.print(" | HumN: "); Serial.print(hN, 2);
  Serial.println();

  Serial.print("ConfortScore (FIS exportado) = "); Serial.print(y, 2);
  Serial.print(" => "); Serial.println(labelFromOutput(y));

  delay(2000); // DHT11: no leer demasiado rápido
}
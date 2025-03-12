#include <SPI.h>

// --- Настройки пинов ---
// Задний мотор
#define REAR_MOTOR_PWM 6   // ШИМ заднего мотора
#define REAR_MOTOR_DIR 7   // Направление заднего мотора

// Пины для энкодера
#define ENCODER_CS_PIN 10 // CS энкодера
#define ENCODER_CLK_PIN 13 // CLK энкодера (SCK SPI)
#define ENCODER_DATA_PIN 12 // MISO энкодера (данные)

// Пины для мотора
#define FRONT_MOTOR_LEFT 5 // ШИМ переднего мотора
#define FRONT_MOTOR_RIGHT 4 // Направление переднего мотора

// Диапазон углов колес
#define WHEEL_ANGLE_MIN -30 // Минимальный угол поворота колес
#define WHEEL_ANGLE_MAX 30  // Максимальный угол поворота колес

// Диапазон углов энкодера (градусы)
int ENCODER_ANGLE_RIGHT=0;
int ENCODER_ANGLE_LEFT=0;

// Параметр П-контроллера
float Kp = 0.5;

// Целевой угол поворота колес
int targetWheelAngle = 0;

int readEncoder(int* position=NULL);


void setup() {
  // Настройка пинов энкодера
  pinMode(ENCODER_CS_PIN, OUTPUT);
  digitalWrite(ENCODER_CS_PIN, HIGH); // CS по умолчанию высокий

  // Настройка SPI
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);          // Работаем в режиме SPI MODE0
  SPI.setClockDivider(SPI_CLOCK_DIV16); // Делитель частоты (подходит для большинства энкодеров)

  // Настройка пинов моторов
  pinMode(REAR_MOTOR_PWM, OUTPUT);
  pinMode(REAR_MOTOR_DIR, OUTPUT);
  pinMode(FRONT_MOTOR_RIGHT, OUTPUT);
  pinMode(FRONT_MOTOR_LEFT, OUTPUT);



  // Отладка
  Serial.begin(9600);
  Serial.println("Driver ready");

  test_encoder_max(&ENCODER_ANGLE_RIGHT, &ENCODER_ANGLE_LEFT);
  test_encoder_max(&ENCODER_ANGLE_RIGHT, &ENCODER_ANGLE_LEFT);
  controlSteering(30);

}

void loop() {
  delay(50);

  


  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("SPEED")) { // Установка скорости заднего мотора
      int speed = command.substring(6).toInt();
      setRearMotorSpeed(speed);
    } else if (command.startsWith("ANGLE")) { // Установка угла переднего мотора
      int angle = command.substring(6).toInt();
      // Ограничиваем целевой угол в пределах допустимого диапазона
      targetWheelAngle = constrain(angle, WHEEL_ANGLE_MIN, WHEEL_ANGLE_MAX);
      controlSteering(angle+30);

    } else {
      Serial.println("Invalid Co  mmand");
    }
  }

  // Отладка
  /*
  Serial.print("Current Angle: ");
  Serial.print(readEncoder());
  Serial.print(", Target Angle: ");
  Serial.print(targetWheelAngle);
  Serial.println();*/
}

void test_encoder_max(int* right, int* left){
  {
    digitalWrite(FRONT_MOTOR_LEFT, HIGH); // Вправо
    analogWrite(FRONT_MOTOR_RIGHT, 0);
    delay(800);
  }

  // Объединяем байты
  readEncoder(right);
   delay(100);

  {
    digitalWrite(FRONT_MOTOR_RIGHT, HIGH); // влево
    analogWrite(FRONT_MOTOR_LEFT, 0);
  
    delay(800);
  }

  // Объединяем байты
  readEncoder(left);
  delay(100);

  analogWrite(FRONT_MOTOR_RIGHT, 255);
  analogWrite(FRONT_MOTOR_LEFT, 255);
  Serial.println("ENCODER COLLIBRATION END: LEFT - " + String(*left) + " , RIGHT - " + String(*right) );

}

// Функция управления поворотом колес
void controlSteering(int targetAngle) {
  int currentAngle;

  do {
    // Считываем текущий угол с энкодера

    // Вычисляем ошибку
    int error = targetAngle - currentAngle;

    // Пропорциональное управление
    int motorSpeed = Kp * abs(error);

    // Ограничиваем скорость
    motorSpeed = constrain(motorSpeed, 0, 255);

    // Определяем направление вращения
    if (error < 0) {
      digitalWrite(FRONT_MOTOR_RIGHT, HIGH); // Вправо
      analogWrite(FRONT_MOTOR_LEFT, 50);

    } else {
      digitalWrite(FRONT_MOTOR_LEFT, HIGH); // Влево
      analogWrite(FRONT_MOTOR_RIGHT, 50);

    }

    delay(50); // Задержка для стабильности
    currentAngle = readEncoder();
  } while (abs(targetAngle - currentAngle) > 1); // Условие выхода, если ошибка мала
  
  analogWrite(FRONT_MOTOR_RIGHT, 255);
  analogWrite(FRONT_MOTOR_LEFT, 255);

}

// Функция для чтения данных с энкодера
int readEncoder(int* position) {
  digitalWrite(ENCODER_CS_PIN, LOW); // Активируем энкодер
  delayMicroseconds(1);

  // Считываем данные энкодера (например, 16-битное значение)
  byte highByte = SPI.transfer(0x00); // Высший байт
  byte lowByte = SPI.transfer(0x00);  // Низший байт

  digitalWrite(ENCODER_CS_PIN, HIGH); // Деактивируем энкодер
  // Объединяем байты
  int pos;
  if(!position){
    position=&pos;
  }
  *position = (highByte << 8) | lowByte;
  Serial.print(*position);
  // Преобразуем в угол (если энкодер выдает значения от 0 до 4096)
  int angle = double(double(*position - ENCODER_ANGLE_LEFT) / double(ENCODER_ANGLE_RIGHT - ENCODER_ANGLE_LEFT)) * 60;
  Serial.print(" : ");
  Serial.println(angle);
  
  return angle;
}

// --- Управление скоростью заднего мотора ---
void setRearMotorSpeed(int speed) {
  if (speed > 255) speed = 255;
  if (speed < -255) speed = -255;
  
  speed=-speed;
  if (speed >= 0) {
    digitalWrite(REAR_MOTOR_DIR, HIGH);
    analogWrite(REAR_MOTOR_PWM, 255-speed);
  } else {
    speed=-speed;
    digitalWrite(REAR_MOTOR_DIR, LOW);
    analogWrite(REAR_MOTOR_PWM, speed);
  }
  /*
    speed=-speed;
  if (speed >= 0) {
    digitalWrite(REAR_MOTOR_DIR, HIGH);
    analogWrite(REAR_MOTOR_PWM, 255-speed);
  } else {
    digitalWrite(REAR_MOTOR_PWM, HIGH);
    analogWrite(REAR_MOTOR_DIR, 255+speed);
  }
  */

  Serial.print("Rear motor speed set to: ");
  Serial.println(speed);
}

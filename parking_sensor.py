import serial
import time


class ParkingSensor:
    def __init__(self, port, baud_rate=21520, timeout=1):
        """
        Инициализация подключения к парктроникам.
        :param port: COM-порт, к которому подключено устройство.
        :param baud_rate: Скорость передачи данных.
        :param timeout: Таймаут для чтения данных.
        """
        try:
            self.arduino = serial.Serial(port, baud_rate, timeout=timeout)
            print(f"Подключение к {port} установлено.")
            time.sleep(2)  # Задержка для установления соединения
            self.arduino.flush()  # Очистка буфера
        except serial.SerialException as e:
            raise ConnectionError(f"Ошибка подключения к порту {port}: {e}")

    def read_data(self):
        """
        Считывание данных с парктроников.
        :return: Словарь с данными парктроников или None, если данные некорректны.
        """
        try:
            line = self.arduino.readline()
            decimal_values = [str(byte) for byte in line][1:]
            dec="\t".join(decimal_values)
            if dec:
                # Ожидаем строку вида "val1 val2 val3 val4 mode"
                parts = dec.split()
                if len(parts) > 7:
                    #print(dec)
                    try:
                        data = {
                            "sensors": list(map(int, parts[:4])),
                            "mode": int(parts[6])  # 0 - передние, 1 - задние
                        }
                        return data
                    except ValueError:
                        print(f"Ошибка преобразования данных: {line}")
        except Exception as e:
            print(f"Ошибка чтения данных: {e}")
        return None

    def close(self):
        """
        Закрыть соединение с устройством.
        """
        if self.arduino.is_open:
            self.arduino.close()
            print("Соединение закрыто.")
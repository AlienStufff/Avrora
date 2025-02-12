import tkinter as tk
from tkinter import Scale, Button
from PIL import Image, ImageTk
import numpy as np
from motor_controller import MotorController
from parking_sensor import ParkingSensor


import tkinter as tk
from tkinter import Scale, Button
from PIL import Image, ImageTk
import numpy as np


class CarControlApp:
    def __init__(self, root, motor_controller, parking_sensor):
        self.root = root
        self.motor_controller = motor_controller
        self.parking_sensor = parking_sensor
        
        self.front_sensors = []
        self.rear_sensors = []

        self.root.title("Car Control")
        
        # Load car image (top view)
        self.car_image = Image.open("car_top_view.png")
        self.car_image = self.car_image.resize((210, 440))  # Resize to fit canvas
        self.car_photo = ImageTk.PhotoImage(self.car_image)

        # Creating canvas for car drawing
        self.canvas = tk.Canvas(root, width=500, height=500)
        self.canvas.grid(row=0, column=0, padx=10, pady=10)

        # Display the car image on canvas
        self.car_image_on_canvas = self.canvas.create_image(110, 240, image=self.car_photo)

        # Creating control sliders for speed and angle
        self.speed_slider = Scale(root, from_=-255, to=255, orient="horizontal", label="Speed")
        self.speed_slider.grid(row=1, column=0, padx=10, pady=5)

        self.angle_slider = Scale(root, from_=-30, to=30, orient="horizontal", label="Steering Angle")
        self.angle_slider.grid(row=2, column=0, padx=10, pady=5)

        # Create button to apply controls
        self.apply_button = Button(root, text="Apply", command=self.apply_controls)
        self.apply_button.grid(row=3, column=0, padx=10, pady=5)

        # Update telemetry
        self.update_telemetry()

    def apply_controls(self):
        speed = self.speed_slider.get()
        angle = self.angle_slider.get()
        self.motor_controller.set_speed(speed)
        self.motor_controller.set_angle(angle)

    def update_telemetry(self):
        # Update telemetry with current sensor values and angle/speed
        telemetry_data = self.motor_controller.read_telemetry()
        sensor_data = self.parking_sensor.read_data()

        # Clear previous drawing
        self.canvas.delete("all")
        
        # Redraw the car image
        self.canvas.create_image(250, 250, image=self.car_photo)

        # Draw wheels (rotating based on angle)
        wheel_angle = telemetry_data["angle"] if telemetry_data else 0
        self.draw_wheels(wheel_angle)

        # Parking sensors (showing lines)
        self.draw_parking_sensors(sensor_data)

        # Call again to update telemetry
        self.root.after(100, self.update_telemetry)

    def draw_wheels(self, angle):
        # Wheel sizes and positions
        wheel_width = 40
        wheel_length = 20
        car_x, car_y = 245, 50
        wheel_offset_x = 65
        wheel_offset_y = 60

        # Front left and right wheels
        wheel_1_x = car_x - wheel_offset_x
        wheel_1_y = car_y + wheel_offset_y
        wheel_2_x = car_x + wheel_offset_x
        wheel_2_y = car_y + wheel_offset_y

        # Draw the wheels as rectangles and rotate them based on angle
        self.draw_rotated_wheel(wheel_1_x, wheel_1_y, angle)
        self.draw_rotated_wheel(wheel_2_x, wheel_2_y, angle)

    def draw_rotated_wheel(self, x, y, angle):
        # Wheel dimensions
        wheel_width = 40
        wheel_length = 20

        # Convert angle to radians
        angle_rad = np.deg2rad(angle)

        # Calculate new coordinates for the rotated wheel
        wheel_coords = [
            (-wheel_length / 2, -wheel_width / 2),  # top-left corner
            (wheel_length / 2, -wheel_width / 2),   # top-right corner
            (wheel_length / 2, wheel_width / 2),    # bottom-right corner
            (-wheel_length / 2, wheel_width / 2),   # bottom-left corner
        ]

        # Apply rotation matrix to each point
        rotated_coords = []
        for (dx, dy) in wheel_coords:
            new_x = x + dx * np.cos(angle_rad) - dy * np.sin(angle_rad)
            new_y = y + dx * np.sin(angle_rad) + dy * np.cos(angle_rad)
            rotated_coords.append((new_x, new_y))

        # Draw the rotated wheel as a polygon using the calculated coordinates
        self.canvas.create_polygon(rotated_coords, fill="black")

    def draw_parking_sensors(self, sensor_data):
        # Разделение данных на передние и задние датчики
        if sensor_data!= None:
            if sensor_data['mode'] == 0:  # Передние датчики
                self.front_sensors.clear()
            else:
                self.rear_sensors.clear()
            # Обрабатываем данные с парктроников
            for i, sensor in enumerate(sensor_data['sensors']):
                if sensor_data['mode'] == 0:  # Передние датчики
                    self.front_sensors.append(sensor)
                else:  # Задние датчики
                    self.rear_sensors.append(sensor)

        # Отображение передних датчиков (4 датчика)
        if len(self.front_sensors)==4:
            self.draw_sensor_line(245-55, 50, 120, 35-self.front_sensors[0]/255*35)
            self.draw_sensor_line(245-15, 50, 90, 35-self.front_sensors[1]/255*35)
            self.draw_sensor_line(245+15, 50, 90, 35-self.front_sensors[2]/255*35)
            self.draw_sensor_line(245+45, 50, 60, 35-self.front_sensors[3]/255*35)


        # Отображение задних датчиков (4 датчика)
        if len(self.rear_sensors)==4:
            self.draw_sensor_line(245-55, 450, -120, 35-self.rear_sensors[0]/255*35)
            self.draw_sensor_line(245-15, 450, -90, 35-self.rear_sensors[1]/255*35)
            self.draw_sensor_line(245+15, 450, -90, 35-self.rear_sensors[2]/255*35)
            self.draw_sensor_line(245+45, 450, -60, 35-self.rear_sensors[3]/255*35)



    def draw_sensor_line(self, x, y, angle, distance):
        # Draw the sensor line representing distance
        rad = np.deg2rad(angle)
        x2 = x + np.cos(rad) * distance
        y2 = y - np.sin(rad) * distance
        self.canvas.create_line(x, y, x2, y2, fill="black", width=6)
        self.canvas.create_line(x, y, x2, y2, fill="green", width=3)


if __name__ == "__main__":
    root = tk.Tk()

    # Initialize motor and parking sensor controllers
    motor_controller = MotorController(port='COM53')
    parking_sensor = ParkingSensor(port='COM40')

    app = CarControlApp(root, motor_controller, parking_sensor)

    root.mainloop()

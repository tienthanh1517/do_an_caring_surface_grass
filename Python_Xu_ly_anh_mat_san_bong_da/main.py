import cv2
import numpy as np
import serial
import time

# Khởi tạo các giá trị ngưỡng màu
lower_co_xanh = np.array([45, 50, 20])
upper_co_xanh = np.array([75, 255, 255])
lower_co_xam = np.array([15, 10, 20])
upper_co_xam = np.array([45, 175, 255])

esp32Data = serial.Serial(port='COM4', baudrate=115200, timeout=.1)


def write_read(gia_tri):
    setdata = str(gia_tri)
    esp32Data.write(bytes(setdata, 'utf-8'))
    time.sleep(0.005)
    data = esp32Data.readline()
    return data
# Mở hình ảnh
img = cv2.imread('image_xau.jpg')
# Chuyển đổi không gian màu từ BGR sang HSV
img_hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

# Tạo các lớp phủ bề mặt
mask_co_xanh = cv2.inRange(img_hsv, lower_co_xanh, upper_co_xanh)
mask_co_xam = cv2.inRange(img_hsv, lower_co_xam, upper_co_xam)

# Tính diện tích lớp phủ bề mặt
area_co_xanh = cv2.countNonZero(mask_co_xanh)
area_co_xam = cv2.countNonZero(mask_co_xam)


print(area_co_xanh)
print(area_co_xam)

while True:
# Tính tỷ lệ phần trăm vật thể có màu vàng
    percent_co_vang = (area_co_xam / (area_co_xanh + area_co_xam)) * 100
    #Gửi tỷ lệ phần trăm vật thể có màu vàng đến Esp32
    value = write_read(percent_co_vang)
    print(value)
    # Hiển thị hình ảnh
    cv2.imshow("mask image green grass", mask_co_xanh)
    cv2.imshow("mask image yellow grass", mask_co_xam)
    cv2.imshow("window", img)

    key = cv2.waitKey(1)
    if key == ord("q"):
        break
cv2.destroyAllWindows()

# import cv2
# import numpy as np
# import serial
# import time
#
# # Khởi tạo các giá trị ngưỡng màu
# lower_co_xanh = np.array([40, 50, 20])
# upper_co_xanh = np.array([78, 255, 255])
# lower_co_xam = np.array([15, 45, 45])
# upper_co_xam = np.array([33, 175, 255])
#
#
# # Mở hình ảnh
# img = cv2.imread('image_xau.jpg')
# # img = cv2.resize(img, (1080, 720))
# img_hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
#
# # Tạo các lớp phủ bề mặt
# mask_co_xanh = cv2.inRange(img_hsv, lower_co_xanh, upper_co_xanh)
# mask_co_xam = cv2.inRange(img_hsv, lower_co_xam, upper_co_xam)
#
# # Tính diện tích lớp phủ bề mặt
# area_co_xanh = cv2.countNonZero(mask_co_xanh)
# area_co_xam = cv2.countNonZero(mask_co_xam)
#
#
# print(area_co_xanh)
# print(area_co_xam)
#
# # Tính tỷ lệ phần trăm vật thể có màu vàng
# percent_co_vang = (area_co_xam / (area_co_xanh + area_co_xam)) * 100
# print(percent_co_vang)
# #Gửi tỷ lệ phần trăm vật thể có màu vàng đến Esp32
# # Hiển thị hình ảnh
# cv2.imshow("mask image green grass", mask_co_xanh)
# cv2.imshow("mask image yellow grass", mask_co_xam)
# cv2.imshow("window", img)
#
# key = cv2.waitKey(0)
# cv2.destroyAllWindows()

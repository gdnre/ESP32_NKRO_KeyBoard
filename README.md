# 前言
* 本键盘库包括USB键盘以及蓝牙键盘功能，请根据设备硬件选择需要的版本，USB键盘只有ESP32-S2/S3支持，ESP32-S2不支持蓝牙功能；
  
* 本键盘库基于官方键盘库和T-vK的ESP32-BLE-Keyboard键盘库修改报告描述符实现，从6键无冲支持到104按键全键无冲（and more）；

# 使用说明
使用请参考官方USB键盘案例以及T-vK的蓝牙键盘库：
```
https://github.com/espressif/arduino-esp32/tree/master/libraries/USB/examples/Keyboard
https://github.com/T-vK/ESP32-BLE-Keyboard
```

# 功能
* 本键盘库修改了报告描述符，原版本只支持修饰键以外的最多6键同时按下，修改后使用17Byte的每1bit表示1个按键，映射至HID Usage Tables文档中Keyboard usage page的0-135号useage id，可支持这136个按键同时按下，但由于官方键盘库映射至ascii表的处理以及保留按键的存在，实际可同时按下的键要少于136；
* 蓝牙键盘库包括媒体按键报告描述符，USB键盘不支持媒体按键，但可以直接使用官方的USBHIDConsumerControl库，但蓝牙键盘库和官方ConsumerControl库实现媒体按键的描述符不同，数据类型不相互兼容，有需要的可参照官方库自行修改蓝牙键盘库的描述符，官方库实现的功能范围更广；
```
  https://github.com/espressif/arduino-esp32/tree/master/libraries/USB/src
```

# 可能的问题
* 修改后的键盘报告与boot键盘报告不兼容，不保证比较老的设备在boot模式下可以使用（我在b550m主板的bios主板上测试是可用的，当然蓝牙我没法测试boot模式兼容性）；
* 官方键盘库和蓝牙键盘库有部分命名冲突，同时使用时如有报错请考虑注释掉其中一部分或重命名；

# 感谢
感谢b站up主hoop0对hid设备及报告描述符的详细解释，有需要的可以去看他的视频：
https://space.bilibili.com/287090028
